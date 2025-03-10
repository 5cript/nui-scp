#pragma once

#include <persistence/state/terminal_engine.hpp>
#include <persistence/state_holder.hpp>
#include <backend/password/password_provider.hpp>
#include <backend/sftp/operation_queue.hpp>
#include <ssh/session.hpp>
#include <ssh/sftp_session.hpp>
#include <ids/ids.hpp>

#include <nui/rpc.hpp>
#include <libssh/libsshpp.hpp>
#include <libssh/sftp.h>

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <thread>

class SshSessionManager
{
  public:
    constexpr static auto futureTimeout = std::chrono::seconds{10};

    SshSessionManager(Persistence::StateHolder* stateHolder);
    ~SshSessionManager();
    SshSessionManager(SshSessionManager const&) = delete;
    SshSessionManager& operator=(SshSessionManager const&) = delete;
    SshSessionManager(SshSessionManager&&) = delete;
    SshSessionManager& operator=(SshSessionManager&&) = delete;

    void registerRpc(Nui::Window& wnd, Nui::RpcHub& rpcHub);

    void addPasswordProvider(int priority, PasswordProvider* provider);
    void joinSessionAdder();
    void addSession(
        Persistence::SshTerminalEngine const& engine,
        std::function<void(std::optional<Ids::SessionId> const&)> onComplete);

    friend int askPassDefault(char const* prompt, char* buf, std::size_t length, int echo, int verify, void* userdata);

  private:
    void registerRpcConnect(Nui::Window& wnd, Nui::RpcHub& hub);
    void registerRpcCreateChannel(Nui::Window& wnd, Nui::RpcHub& hub);
    void registerRpcStartChannelRead(Nui::Window& wnd, Nui::RpcHub& hub);
    void registerRpcChannelClose(Nui::Window& wnd, Nui::RpcHub& hub);
    void registerRpcEndSession(Nui::Window&, Nui::RpcHub& hub);
    void registerRpcChannelWrite(Nui::Window&, Nui::RpcHub& hub);
    void registerRpcChannelPtyResize(Nui::Window&, Nui::RpcHub& hub);

    // Sftp:
    void registerRpcSftpListDirectory(Nui::Window&, Nui::RpcHub& hub);
    void registerRpcSftpCreateDirectory(Nui::Window&, Nui::RpcHub& hub);
    void registerRpcSftpCreateFile(Nui::Window&, Nui::RpcHub& hub);

    // Sftp Files:
    void registerRpcSftpAddDownloadOperation(Nui::Window&, Nui::RpcHub& hub);

  private:
    std::mutex passwordProvidersMutex_{};
    std::mutex addSessionMutex_{};
    Persistence::StateHolder* stateHolder_{};
    std::unordered_map<Ids::SessionId, std::unique_ptr<SecureShell::Session>, Ids::IdHash> sessions_{};
    std::unordered_map<Ids::ChannelId, std::weak_ptr<SecureShell::Channel>, Ids::IdHash> channels_{};
    std::unordered_map<Ids::ChannelId, std::weak_ptr<SecureShell::SftpSession>, Ids::IdHash> sftpChannels_{};
    OperationQueue operationQueue_{};
    std::map<int, PasswordProvider*> passwordProviders_{};
    std::unique_ptr<std::thread> addSessionThread_{};
    std::vector<SecureShell::PasswordCacheEntry> pwCache_{};
};

int askPassDefault(char const* prompt, char* buf, std::size_t length, int echo, int verify, void* userdata);