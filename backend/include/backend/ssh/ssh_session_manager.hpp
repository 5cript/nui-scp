#pragma once

#include <persistence/state/terminal_engine.hpp>
#include <backend/password/password_provider.hpp>
#include <backend/ssh/session.hpp>
#include <backend/ssh/sftp_session.hpp>
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
    SshSessionManager();
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
    std::unordered_map<Ids::SessionId, std::unique_ptr<Session>, Ids::IdHash> sessions_;
    std::mutex passwordProvidersMutex_;
    std::map<int, PasswordProvider*> passwordProviders_;
    std::mutex addSessionMutex_;
    std::unique_ptr<std::thread> addSessionThread_;
    struct PwCache
    {
        std::optional<std::string> user;
        std::string host;
        std::optional<int> port;
        std::optional<std::string> password;
    };
    std::vector<PwCache> pwCache_;
};

int askPassDefault(char const* prompt, char* buf, std::size_t length, int echo, int verify, void* userdata);