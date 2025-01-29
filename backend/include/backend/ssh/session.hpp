#pragma once

#include <libssh/libsshpp.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>
#include <optional>

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