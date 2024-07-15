#pragma once

#include <persistence/state/terminal_engine.hpp>
#include <backend/password/password_provider.hpp>

#include <nui/rpc.hpp>
#include <libssh/libsshpp.hpp>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <mutex>
#include <thread>

class SshSessionManager
{
  public:
    struct Session
    {
        ssh::Session session;
        operator ssh::Session&()
        {
            return session;
        }
    };

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
        std::function<void(std::optional<std::string> const&)> onComplete);

    friend int askPassDefault(char const* prompt, char* buf, std::size_t length, int echo, int verify, void* userdata);

  private:
    std::unordered_map<std::string, std::unique_ptr<Session>> sessions_;
    std::mutex passwordProvidersMutex_;
    std::map<int, PasswordProvider*> passwordProviders_;
    std::mutex addSessionMutex_;
    std::unique_ptr<std::thread> addSessionThread_;
};

int askPassDefault(char const* prompt, char* buf, std::size_t length, int echo, int verify, void* userdata);