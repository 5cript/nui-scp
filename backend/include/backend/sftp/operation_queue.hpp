#pragma once

#include <backend/sftp/all_operations.hpp>
#include <persistence/state/state.hpp>
#include <ssh/sftp_session.hpp>
#include <ids/ids.hpp>

#include <vector>
#include <deque>
#include <filesystem>
#include <memory>
#include <utility>

class OperationQueue
{
  public:
    enum class CompletionReason
    {
        Completed,
        Canceled,
        Failed
    };

    struct CompletedOperation
    {
        CompletionReason reason;
        Ids::OperationId id;
        std::chrono::system_clock::time_point completionTime;
        std::optional<std::filesystem::path> localPath{std::nullopt};
        std::optional<std::filesystem::path> remotePath{std::nullopt};
        std::optional<Operation::Error> error{std::nullopt};
    };

    OperationQueue(Persistence::SftpOptions sftpOpts, int parallelism = 1);

    using Error = OperationErrorType;

    void cancelAll();
    void cancel(Ids::OperationId id);

    /**
     * @brief Returns true if it should be called without delay again.
     *
     * @return true
     * @return false
     */
    bool update();

    std::expected<void, Operation::Error> addDownloadOperation(
        SecureShell::SftpSession& sftp,
        Ids::OperationId id,
        std::filesystem::path const& localPath,
        std::filesystem::path const& remotePath);

  private:
    std::mutex mutex_{};
    Persistence::SftpOptions sftpOpts_{};
    std::deque<std::pair<Ids::OperationId, std::unique_ptr<Operation>>> operations_{};
    std::vector<CompletedOperation> completedOperations_{};
    int parallelism_{1};
};
