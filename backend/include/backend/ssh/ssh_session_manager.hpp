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

    void registerRpc(Nui::Window& wnd, Nui::RpcHub& rpcHub);

    void addPasswordProvider(int priority, PasswordProvider* provider);
    std::optional<std::string> addSession(Persistence::SshTerminalEngine const& engine);

    friend int askPassDefault(char const* prompt, char* buf, std::size_t length, int echo, int verify, void* userdata);

  private:
    std::unordered_map<std::string, std::unique_ptr<Session>> sessions_;
    std::mutex passwordProvidersMutex_;
    std::map<int, PasswordProvider*> passwordProviders_;
};

int askPassDefault(char const* prompt, char* buf, std::size_t length, int echo, int verify, void* userdata);