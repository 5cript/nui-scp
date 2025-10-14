#include <backend/sftp/scan_operation.hpp>
#include <log/log.hpp>

#include <ssh/sftp_session.hpp>

ScanOperation::ScanOperation(SecureShell::SftpSession& sftp, ScanOperationOptions options)
    : sftp_(&sftp)
    , remotePath_{std::move(options.remotePath)}
    , progressCallback_{std::move(options.progressCallback)}
    , futureTimeout_{options.futureTimeout}
{}

ScanOperation::~ScanOperation() = default;

std::expected<void, ScanOperation::Error> ScanOperation::scanOnce(std::filesystem::path const& path)
{
    auto fut = sftp_->listDirectory(path);
    fut.wait_for(futureTimeout_);
    if (fut.wait_for(futureTimeout_) != std::future_status::ready)
        return enterErrorState<void>({.type = ErrorType::FutureTimeout});

    const auto result = fut.get();
    if (!result.has_value())
        return enterErrorState<void>({.type = ErrorType::SftpError, .sftpError = result.error()});

    entries_.insert(entries_.end(), result->begin(), result->end());
    return {};
}

std::expected<ScanOperation::WorkStatus, ScanOperation::Error> ScanOperation::work()
{
    using enum OperationState;

    switch (state_)
    {
        case (NotStarted):
        {
            state_ = Running;
            totalBytes_ = 0;
            currentIndex_ = 0;
            Log::info("ScanOperation: Starting scan of '{}'.", remotePath_.generic_string());
            const auto result = scanOnce(remotePath_);
            if (!result.has_value())
            {
                Log::error("ScanOperation: Failed to scan directory: {}", result.error().toString());
                return enterErrorState<WorkStatus>(result.error());
            }
            progressCallback_(totalBytes_, currentIndex_, static_cast<std::uint64_t>(entries_.size()));
            break;
        }
        case (Running):
        {
            if (currentIndex_ >= static_cast<std::uint64_t>(entries_.size()))
            {
                Log::info("ScanOperation: Scan of '{}' completed.", remotePath_.generic_string());
                state_ = Completed;
                return WorkStatus::Complete;
            }

            if (entries_[currentIndex_].isDirectory())
            {
                const auto result = scanOnce(entries_[currentIndex_].path);
                if (!result.has_value())
                {
                    Log::error(
                        "ScanOperation: Failed to scan directory '{}': {}",
                        entries_[currentIndex_].path.generic_string(),
                        result.error().toString());
                    return enterErrorState<WorkStatus>(result.error());
                }
                ++currentIndex_;
            }

            for (; currentIndex_ < static_cast<std::uint64_t>(entries_.size()); ++currentIndex_)
            {
                auto& entry = entries_[currentIndex_];
                if (entry.isRegularFile())
                    totalBytes_ += entry.size;
                else if (entry.isDirectory())
                    return WorkStatus::MoreWork;
            }

            progressCallback_(totalBytes_, currentIndex_, static_cast<std::uint64_t>(entries_.size()));
            return WorkStatus::MoreWork;
        }
        case (Prepared):
        case (Preparing):
        case (Finalizing):
            Log::error("ScanOperation: Invalid state: {}", static_cast<int>(state_));
            return enterErrorState<WorkStatus>({.type = ErrorType::InvalidOperationState});
        case (Completed):
        {
            Log::warn("ScanOperation: Operation already completed.");
            // Dont enter error state here, it would overwrite the success state.
            return std::unexpected(Error{.type = ErrorType::CannotWorkCompletedOperation});
        }
        case (Failed):
        {
            Log::warn("ScanOperation: Operation already failed.");
            // Do not enter error state here, it would overwrite the error state.
            return std::unexpected(Error{.type = ErrorType::CannotWorkFailedOperation});
        }
        case (Canceled):
        {
            Log::warn("ScanOperation: Cannot work on canceled operation.");
            return std::unexpected(Error{.type = ErrorType::CannotWorkCanceledOperation});
        }
    }
    return enterErrorState<WorkStatus>({.type = ErrorType::UnknownWorkState});
}

std::expected<void, ScanOperation::Error> ScanOperation::cancel(bool adoptCancelState)
{
    if (adoptCancelState)
    {
        Log::info("ScanOperation: Scan of '{}' canceled.", remotePath_.generic_string());
        enterState(OperationState::Canceled);
    }
    return {};
}

SecureShell::ProcessingStrand* ScanOperation::strand() const
{
    return sftp_->strand();
}