#include <backend/sftp/operation_queue.hpp>

#include <log/log.hpp>
#include <utility/overloaded.hpp>

namespace
{
    OperationQueue::CompletedOperation makeCompletedOperation(
        OperationQueue::CompletionReason reason,
        Ids::OperationId id,
        Operation const& operation,
        std::optional<Operation::Error> error = std::nullopt)
    {
        return operation.visit(
            Utility::overloaded(
                [reason, id, error](DownloadOperation const& op) {
                    return OperationQueue::CompletedOperation{
                        .reason = reason,
                        .id = id,
                        .completionTime = std::chrono::system_clock::now(),
                        .localPath = op.localPath(),
                        .remotePath = op.remotePath(),
                        .error = error,
                    };
                },
                [reason, id](std::nullopt_t) {
                    return OperationQueue::CompletedOperation{
                        .reason = reason,
                        .id = id,
                        .completionTime = std::chrono::system_clock::now(),
                    };
                }));
    }
}

OperationQueue::OperationQueue(Persistence::SftpOptions sftpOpts, int parallelism)
    : sftpOpts_{std::move(sftpOpts)}
    , parallelism_{parallelism}
{}

void OperationQueue::cancelAll()
{
    std::scoped_lock lock{mutex_};
    operations_.clear();
    Log::info("All operations in the queue have been canceled.");
}
void OperationQueue::cancel(Ids::OperationId id)
{
    std::scoped_lock lock{mutex_};
    std::erase_if(operations_, [id](const auto& op) {
        return op.first == id;
    });
}

bool OperationQueue::update()
{
    std::scoped_lock lock{mutex_};
    const auto updateCount = std::min(operations_.size(), static_cast<std::size_t>(parallelism_));

    bool moreWork = false;
    if (updateCount == 0)
    {
        return false;
    }

    for (std::size_t i = 0; i < updateCount; ++i)
    {
        auto& [id, operation] = operations_[i];
        const auto workResult = operation->work();
        if (!workResult.has_value())
        {
            Log::error("Operation failed: {}", workResult.error().toString());
            completedOperations_.push_back(
                makeCompletedOperation(OperationQueue::CompletionReason::Failed, id, *operation, workResult.error()));
            continue;
        }

        const auto workStatus = workResult.value();
        if (workStatus == Operation::WorkStatus::Complete)
        {
            Log::info("Operation completed successfully: {}", id.value());
            completedOperations_.push_back(
                makeCompletedOperation(OperationQueue::CompletionReason::Completed, id, *operation));
            operations_.pop_front();
            // Exit loop and avoid any offset math. Just do another update cycle.
            return true;
        }
        else if (workStatus == Operation::WorkStatus::MoreWork)
        {
            moreWork = true;
            continue;
        }
    }
    return moreWork;
}

std::expected<void, Operation::Error> OperationQueue::addDownloadOperation(
    SecureShell::SftpSession& sftp,
    Ids::OperationId id,
    std::filesystem::path const& localPath,
    std::filesystem::path const& remotePath)
{
    auto fut = sftp.openFile(remotePath, SecureShell::SftpSession::OpenType::Read, std::filesystem::perms::unknown);
    if (fut.wait_for(std::chrono::seconds{5}) != std::future_status::ready)
    {
        Log::error("Failed to open remote sftp file: timeout");
        return std::unexpected(Operation::Error{.type = Operation::ErrorType::OpenFailure});
    }

    const auto result = fut.get();
    if (!result.has_value())
    {
        Log::error("Failed to open remote sftp file: {}", result.error().message);
        return std::unexpected(Operation::Error{.type = Operation::ErrorType::SftpError, .sftpError = result.error()});
    }

    const auto transferOptions = sftpOpts_.downloadOptions.value_or(Persistence::TransferOptions{});
    const auto defaultOptions = DownloadOperation::DownloadOperationOptions{};

    auto operation = std::make_unique<DownloadOperation>(
        std::move(result).value(),
        DownloadOperation::DownloadOperationOptions{
            .remotePath = remotePath,
            .localPath = localPath,
            .tempFileSuffix = transferOptions.tempFileSuffix.value_or(defaultOptions.tempFileSuffix),
            .mayOverwrite = transferOptions.mayOverwrite.value_or(defaultOptions.mayOverwrite),
            .reserveSpace = transferOptions.reserveSpace.value_or(defaultOptions.reserveSpace),
            .tryContinue = transferOptions.tryContinue.value_or(defaultOptions.tryContinue),
            .inheritPermissions = transferOptions.inheritPermissions.value_or(defaultOptions.inheritPermissions),
            .doCleanup = transferOptions.doCleanup.value_or(defaultOptions.doCleanup),
            .permissions =
                transferOptions.customPermissions ? transferOptions.customPermissions : defaultOptions.permissions,
        });

    std::scoped_lock lock{mutex_};
    operations_.emplace_back(id, std::move(operation));
    return {};
}