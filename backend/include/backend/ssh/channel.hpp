#pragma once

#include <libssh/libsshpp.hpp>

#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <thread>

class Session;

class Channel
{
  public:
    friend Session;

    Channel(Channel const&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(Channel const&) = delete;
    Channel& operator=(Channel&&) = delete;
    ~Channel();

    operator ssh::Channel&()
    {
        return *channel_;
    }

    void stop();

    /**
     * @brief Writes data to the channel.
     *
     * @param data The data to write
     */
    void write(std::string const& data);

    /**
     * @brief Resizes the pty.
     *
     * @param cols The new number of columns.
     * @param rows The new number of rows.
     * @return int 0 on success, otherwise an error code.
     */
    int resizePty(int cols, int rows);

    /**
     * @brief Starts reading and processing the channel.
     *
     * @param onStdout
     * @param onStderr
     * @param onExit
     */
    void startReading(
        std::function<void(std::string const&)> onStdout,
        std::function<void(std::string const&)> onStderr,
        std::function<void()> onExit);

  private:
    Channel(std::unique_ptr<ssh::Channel> channel, bool isPty);
    void doProcessing();

  private:
    std::atomic_bool stopIssued_;
    std::mutex channelMutex_{};
    std::unique_ptr<ssh::Channel> channel_;
    bool isPty_{false};
    std::vector<std::string> queuedWrites_{};
    std::thread processingThread_{};
    std::function<void(std::string const&)> onStdout_{};
    std::function<void(std::string const&)> onStderr_{};
    std::function<void()> onExit_{};
};