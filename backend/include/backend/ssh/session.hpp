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
     * @brief Retrieves a channel by id.
     *
     * @param id
     * @return Channel*
     */
    Channel* getChannel(Ids::ChannelId const& id);

  private:
    std::mutex sessionMutex_;
    ssh::Session session_;
    std::unordered_map<Ids::ChannelId, std::unique_ptr<Channel>, Ids::IdHash> channels_;
    std::vector<std::shared_ptr<SftpSession>> sftpSessions_;
};