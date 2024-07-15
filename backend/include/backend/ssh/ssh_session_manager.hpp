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
    class Session
    {
      public:
        Session();
        ~Session();
        Session(Session const&) = delete;
        Session& operator=(Session const&) = delete;
        Session(Session&&) = delete;
        Session& operator=(Session&&) = delete;

        operator ssh::Session&()
        {
            return session_;
        }

        void startReading(
            std::function<void(std::string const&)> onStdout,
            std::function<void(std::string const&)> onStderr,
            std::function<void()> onExit);

        void stop();
        int createPtyChannel();
        void setPtyEnvironment(std::unordered_map<std::string, std::string> const& env);
        void write(std::string const& data);
        int resizePty(int cols, int rows);

      private:
        void doProcessing();

      private:
        std::atomic_bool stopIssued_;
        std::mutex sessionMutex_;
        std::thread processingThread_;
        ssh::Session session_;
        std::unique_ptr<ssh::Channel> ptyChannel_;
        std::optional<std::unordered_map<std::string, std::string>> environment_;
        std::function<void(std::string const&)> onStdout_;
        std::function<void(std::string const&)> onStderr_;
        std::function<void()> onExit_;
        std::vector<std::string> queuedWrites_;
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