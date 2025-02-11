#pragma once

#include <backend/ssh/channel.hpp>
#include <ids/ids.hpp>

#include <libssh/libsshpp.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <expected>
#include <thread>
#include <condition_variable>
#include <atomic>

class SftpSession;

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

    /**
     * @brief Create the single SFTP session.
     *
     * @return std::shared_ptr<SftpSession> The sftp session.
     */
    std::shared_ptr<SftpSession> createSftpSession();

    /**
     * @brief Get the sftp session. If there is no session, a new one will be created.
     *
     * @return std::shared_ptr<SftpSession> The sftp session.
     */
    std::shared_ptr<SftpSession> getSftpSession();

    /**
     * @brief Stops all channels and closes the sftp sessions.
     * Do not reuse the session afterwards.
     */
    void stop();

    /**
     * @brief Closes and removes a channel.
     *
     * @param id
     */
    bool closeChannel(Ids::ChannelId const& id);

    /**
     * @brief Creates a new channel as a pty
     *
     * @return std::expected<ChannelId, int> The channel id or an error code
     */
    std::expected<Ids::ChannelId, int>
    createPtyChannel(std::optional<std::unordered_map<std::string, std::string>> const& environment);

    /**
     * @brief Does something with a channel locked.
     *
     * @param id
     * @return Channel*
     */
    void withChannelDo(Ids::ChannelId const& id, std::function<void(Channel*)> const& handler);

    /**
     * @brief Pause reading to create new channels.
     */
    void pauseProcessing(bool paused);

    /**
     * @brief Start reading of all channels
     */
    void startReading();
    void stopReading();

  private:
    void doChannelProcessing();

  private:
    std::mutex sessionMutex_;

    // Channel processing
    std::thread processingThread_{};
    std::atomic_bool stopIssued_{false};

    // Processing pause
    std::atomic_bool pauseProcessing_{false};
    std::mutex processingPauseMutex_;
    std::condition_variable processingCondition_;
    bool pauseReached_{false};

    ssh::Session session_;
    std::vector<std::shared_ptr<SftpSession>> sftpSessions_;
    std::unordered_map<Ids::ChannelId, std::unique_ptr<Channel>, Ids::IdHash> channels_;
};