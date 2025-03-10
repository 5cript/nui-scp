#include <backend/sftp/operation_queue.hpp>

#include <log/log.hpp>

OperationQueue::OperationQueue()
    : operations_{}
{}
void OperationQueue::start()
{}
void OperationQueue::pause()
{}
void OperationQueue::cancelAll()
{}
void OperationQueue::cancel(Ids::OperationId id)
{}
std::expected<void, Operation::Error> OperationQueue::addDownloadOperation(
    Persistence::State const& state,
    std::string const& sshSessionOptionsKey,
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

    auto sessionOptsIter = state.sshSessionOptions.find(sshSessionOptionsKey);
    if (sessionOptsIter == state.sshSessionOptions.end())
    {
        Log::error("No ssh session options found with key: {}", sshSessionOptionsKey);
        return std::unexpected(Operation::Error{.type = Operation::ErrorType::InvalidOptionsKey});
    }

    auto sessionOpts = sessionOptsIter->second;
    sessionOpts.sftpOptions.resolveWith(state.sftpOptions);
    const auto& sftpOpts = sessionOpts.sftpOptions;
    const auto transferOptions = sftpOpts->downloadOptions.value_or(Persistence::TransferOptions{});
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

    operations_.emplace_back(id, std::move(operation));
    return {};
}