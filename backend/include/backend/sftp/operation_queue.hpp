#pragma once

#include <backend/sftp/all_operations.hpp>
#include <persistence/state/state.hpp>
#include <ssh/sftp_session.hpp>
#include <nui/rpc.hpp>
#include <ids/ids.hpp>
#include <backend/rpc_helper.hpp>

#include <vector>
#include <deque>
#include <filesystem>
#include <memory>
#include <utility>

class OperationQueue
    : public RpcHelper::StrandRpc
    , public std::enable_shared_from_this<OperationQueue>
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

    OperationQueue(
        boost::asio::any_io_executor executor,
        std::shared_ptr<boost::asio::strand<boost::asio::any_io_executor>> strand,
        Nui::Window& wnd,
        Nui::RpcHub& hub,
        Persistence::SftpOptions sftpOpts,
        Ids::SessionId sessionId,
        int parallelism = 1);

    using Error = OperationErrorType;

    void cancelAll();
    void cancel(Ids::OperationId id);

    /**
     * @brief Returns true if it should be called without delay again.
     *
     * @return true
     * @return false
     */
    bool work();

    std::expected<void, Operation::Error> addDownloadOperation(
        SecureShell::SftpSession& sftp,
        Ids::OperationId operationId,
        std::filesystem::path const& localPath,
        std::filesystem::path const& remotePath);

  private:
    template <typename T>
    void call(std::string event, Ids::OperationId id, T&& data)
    {
        hub_->callRemote(
            fmt::format("OperationQueue::{}::{}", sessionId_.value(), event),
            nlohmann::json{
                {"operationId", id.value()},
                {"payload", std::forward<T>(data)},
            });
    }

  private:
    Persistence::SftpOptions sftpOpts_{};
    Ids::SessionId sessionId_{};
    std::deque<std::pair<Ids::OperationId, std::unique_ptr<Operation>>> operations_{};
    std::vector<CompletedOperation> completedOperations_{};
    int parallelism_{1};
};
