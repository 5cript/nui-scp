#include <backend/sftp/operation_queue.hpp>
#include <shared_data/file_operations/download_progress.hpp>
#include <shared_data/file_operations/operation_added.hpp>
#include <shared_data/file_operations/operation_completed.hpp>

#include <log/log.hpp>
#include <utility/overloaded.hpp>

namespace
{
    OperationQueue::OperationCompleted makeCompletedOperation(
        OperationQueue::CompletionReason reason,
        Ids::OperationId operationId,
        Operation const& operation,
        std::optional<Operation::Error> error = std::nullopt)
    {
        return operation.visit(
            Utility::overloaded(
                [reason, operationId, error](DownloadOperation const& op) {
                    return OperationQueue::OperationCompleted{
                        .reason = reason,
                        .operationId = operationId,
                        .completionTime = std::chrono::system_clock::now(),
                        .localPath = op.localPath(),
                        .remotePath = op.remotePath(),
                        .error = error,
                    };
                },
                [reason, operationId, error](ScanOperation const& op) {
                    return OperationQueue::OperationCompleted{
                        .reason = reason,
                        .operationId = operationId,
                        .completionTime = std::chrono::system_clock::now(),
                        .remotePath = op.remotePath(),
                        .error = error,
                    };
                },
                [reason, operationId, error](BulkDownloadOperation const&) {
                    return OperationQueue::OperationCompleted{
                        .reason = reason,
                        .operationId = operationId,
                        .completionTime = std::chrono::system_clock::now(),
                        .error = error,
                    };
                },
                [reason, operationId](std::nullopt_t) {
                    return OperationQueue::OperationCompleted{
                        .reason = reason,
                        .operationId = operationId,
                        .completionTime = std::chrono::system_clock::now(),
                    };
                }));
    }
}

OperationQueue::OperationQueue(
    boost::asio::any_io_executor executor,
    std::shared_ptr<boost::asio::strand<boost::asio::any_io_executor>> strand,
    Nui::Window& wnd,
    Nui::RpcHub& hub,
    Persistence::SftpOptions sftpOpts,
    Ids::SessionId sessionId,
    int parallelism)
    : RpcHelper::StrandRpc{executor, strand, wnd, hub}
    , sftpOpts_{std::move(sftpOpts)}
    , sessionId_{std::move(sessionId)}
    , parallelism_{parallelism}
{}

void OperationQueue::cancelAll()
{
    within_strand_do([weak = weak_from_this()]() {
        auto self = weak.lock();
        if (!self)
            return;

        self->operations_.clear();
        Log::info("All operations in the queue have been canceled.");
    });
}
void OperationQueue::cancel(Ids::OperationId id)
{
    within_strand_do([weak = weak_from_this(), id = std::move(id)]() {
        auto self = weak.lock();
        if (!self)
            return;

        std::erase_if(self->operations_, [id](const auto& op) {
            return op.first == id;
        });
    });
}

void OperationQueue::completeOperation(SharedData::OperationCompleted&& operationCompleted)
{
    within_strand_do([weak = weak_from_this(), operationCompleted = std::move(operationCompleted)]() {
        auto self = weak.lock();
        if (!self)
            return;

        if (operationCompleted.error)
            Log::error("Operation failed: {}", operationCompleted.error->toString());

        self->hub_->callRemote(
            fmt::format("OperationQueue::{}::onOperationCompleted", self->sessionId_.value()),
            SharedData::OperationCompleted{
                .reason = operationCompleted.reason,
                .operationId = operationCompleted.operationId,
                .completionTime = operationCompleted.completionTime,
                .localPath = operationCompleted.localPath,
                .remotePath = operationCompleted.remotePath,
                .error = operationCompleted.error,
            });
    });
}

bool OperationQueue::work()
{
    // Assumed in strand

    const auto updateCount = std::min(operations_.size(), static_cast<std::size_t>(parallelism_));

    bool moreWork = false;
    if (updateCount == 0)
        return false;

    for (std::size_t i = 0; i < updateCount; ++i)
    {
        auto& [id, operation] = operations_[i];
        const auto workResult = operation->work();
        if (!workResult.has_value())
        {
            completeOperation(
                makeCompletedOperation(OperationQueue::CompletionReason::Failed, id, *operation, workResult.error()));
            operations_.erase(operations_.begin() + static_cast<std::ptrdiff_t>(i));
            // Exit loop and avoid any offset math. Just do another update cycle.
            return true;
        }

        const auto workStatus = workResult.value();
        if (workStatus == Operation::WorkStatus::Complete)
        {
            Log::info("Operation completed successfully: {}", id.value());
            completeOperation(makeCompletedOperation(OperationQueue::CompletionReason::Completed, id, *operation));
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
    Ids::OperationId operationId,
    std::filesystem::path const& localPath,
    std::filesystem::path const& remotePath)
{
    // Assumed in strand

    auto fut = sftp.stat(remotePath);
    if (fut.wait_for(sftpOpts_.operationTimeout) != std::future_status::ready)
    {
        Log::error("Failed to stat remote sftp file: timeout");
        return std::unexpected(Operation::Error{.type = Operation::ErrorType::FutureTimeout});
    }

    const auto result = fut.get();
    if (!result.has_value())
    {
        Log::error("Failed to stat remote sftp file: {}", result.error().message);
        return std::unexpected(Operation::Error{.type = Operation::ErrorType::SftpError, .sftpError = result.error()});
    }

    if (result->isRegularFile())
    {
        const auto fileSize = result->size;

        auto fut = sftp.openFile(remotePath, SecureShell::SftpSession::OpenType::Read, std::filesystem::perms::unknown);
        if (fut.wait_for(std::chrono::seconds{5}) != std::future_status::ready)
        {
            Log::error("Failed to open remote sftp file: timeout");
            return std::unexpected(Operation::Error{.type = Operation::ErrorType::OpenFailure});
        }

        const auto openResult = fut.get();
        if (!openResult.has_value())
        {
            Log::error("Failed to open remote sftp file: {}", openResult.error().message);
            return std::unexpected(
                Operation::Error{.type = Operation::ErrorType::SftpError, .sftpError = openResult.error()});
        }

        const auto transferOptions = sftpOpts_.downloadOptions.value_or(Persistence::TransferOptions{});
        const auto defaultOptions = DownloadOperation::DownloadOperationOptions{};

        auto operation = std::make_unique<DownloadOperation>(
            std::move(openResult).value(),
            DownloadOperation::DownloadOperationOptions{
                .progressCallback =
                    [weak = weak_from_this(), operationId](auto min, auto max, auto current) {
                        // TODO:
                        auto self = weak.lock();
                        if (!self)
                            return;

                        self->hub_->callRemote(
                            fmt::format("OperationQueue::{}::onDownloadProgress", self->sessionId_.value()),
                            SharedData::DownloadProgress{
                                .operationId = operationId,
                                .min = min,
                                .max = max,
                                .current = current,
                            });

                        Log::info(
                            "Downloaded {} / {} bytes ({}%)",
                            current - min,
                            max - min,
                            (current - min) * 100 / (max - min));
                    },
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

        operations_.emplace_back(operationId, std::move(operation));

        Log::info("Calling OperationQueue::{}::onOperationAdded", sessionId_.value());
        hub_->callRemote(
            fmt::format("OperationQueue::{}::onOperationAdded", sessionId_.value()),
            SharedData::OperationAdded{
                .operationId = operationId, .type = SharedData::OperationType::Download, .totalBytes = fileSize});

        return {};
    }
    else if (result->isDirectory())
    {
        auto scan = std::make_unique<ScanOperation>(
            sftp,
            ScanOperation::ScanOperationOptions{
                .progressCallback =
                    [](auto, auto, auto) {
                        // TODO:
                    },
                .remotePath = remotePath,
                .futureTimeout = std::chrono::seconds{5},
            });

        auto bulk = std::make_unique<BulkDownloadOperation>(
            sftp,
            BulkDownloadOperation::BulkDownloadOperationOptions{
                .overallProgressCallback =
                    [](auto const&, auto, auto, auto, auto) {
                        // TODO:
                    },
                .individualOptions =
                    DownloadOperation::DownloadOperationOptions{
                        .progressCallback =
                            [](auto, auto, auto) {
                                // TODO:
                            },
                        .remotePath = remotePath,
                        .localPath = localPath,
                    },
            });

        operations_.emplace_back(operationId, std::move(scan));
        operations_.emplace_back(operationId, std::move(bulk));

        hub_->callRemote(
            fmt::format("OperationQueue::{}::{}", sessionId_.value(), "onOperationAdded"),
            SharedData::OperationAdded{
                .operationId = operationId,
                .type = SharedData::OperationType::Scan,
            });
        hub_->callRemote(
            fmt::format("OperationQueue::{}::{}", sessionId_.value(), "onOperationAdded"),
            SharedData::OperationAdded{
                .operationId = operationId,
                .type = SharedData::OperationType::BulkDownload,
            });

        return {};
    }
    else
    {
        Log::error("Remote path is neither a file nor a directory: {}.", result->type);
        return std::unexpected(Operation::Error{.type = Operation::ErrorType::OperationNotPossibleOnFileType});
    }
}