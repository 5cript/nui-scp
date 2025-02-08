#include <backend/ssh/session.hpp>
#include <backend/ssh/sftp_session.hpp>
#include <backend/ssh/sequential.hpp>
#include <log/log.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace Detail;

Session::Session()
    : sessionMutex_{}
    , session_{}
    , channels_{}
    , sftpSessions_{}
{}
Session::~Session()
{
    stop();
}

void Session::stop()
{
    for (auto const& sftpSession : sftpSessions_)
    {
        sftpSession->disconnect();
    }

    std::scoped_lock guard{sessionMutex_};
    // This might be possible to do in parallel, but lets be pessimistic and safe.
    for (auto const& [uuid, channel] : channels_)
    {
        channel->stop();
    }

    session_.disconnect();
}

std::shared_ptr<SftpSession> Session::createSftpSession()
{
    auto perhapsSession = makeSftpSession(*this);
    if (!perhapsSession.has_value())
    {
        Log::error(
            "Failed to create sftp session ({}, {}): {}",
            perhapsSession.error().sshError,
            perhapsSession.error().sftpError,
            perhapsSession.error().message);
        return nullptr;
    }
    auto sftpSession = std::make_shared<SftpSession>(std::move(perhapsSession).value());
    sftpSessions_.push_back(sftpSession);
    return sftpSession;
}

std::shared_ptr<SftpSession> Session::getSftpSession()
{
    if (sftpSessions_.empty())
        return createSftpSession();
    return sftpSessions_.back();
}

Channel* Session::getChannel(Ids::ChannelId const& id)
{
    std::scoped_lock guard{sessionMutex_};
    auto it = channels_.find(id);
    if (it == channels_.end())
        return nullptr;
    return it->second.get();
}

bool Session::closeChannel(Ids::ChannelId const& id)
{
    std::scoped_lock guard{sessionMutex_};
    auto it = channels_.find(id);
    if (it == channels_.end())
        return false;
    it->second->stop();
    channels_.erase(it);
    return true;
}

std::expected<Ids::ChannelId, int>
Session::createPtyChannel(std::optional<std::unordered_map<std::string, std::string>> const& environment)
{
    auto ptyChannel = std::make_unique<ssh::Channel>(session_);
    auto& channel = *ptyChannel;
    auto result = sequential(
        [&channel]() {
            if (!channel.isOpen())
                return channel.openSession();
            return 0;
        },
        [&channel, &environment]() {
            if (!environment.has_value())
                return 0;
            for (auto const& [key, value] : *environment)
            {
                if (channel.requestEnv(key.c_str(), value.c_str()) != 0)
                    return -1;
            }
            return 0;
        },
        [&channel]() {
            return channel.requestPty("xterm", 80, 24);
        },
        [&channel]() {
            return channel.requestShell();
        });

    if (result.result != SSH_OK)
        return std::unexpected(result.result);

    const auto id = Ids::generateChannelId();

    // make unique impossible due to private constructor
    std::scoped_lock guard{sessionMutex_};
    channels_[id] = std::unique_ptr<Channel>{new Channel(std::move(ptyChannel), true)};
    return id;
}