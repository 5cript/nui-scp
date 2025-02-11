#include <backend/ssh/session.hpp>
#include <backend/ssh/sftp_session.hpp>
#include <backend/ssh/sequential.hpp>
#include <log/log.hpp>
#include <nui/utility/scope_exit.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace Detail;

Session::Session()
    : sessionMutex_{}
    , session_{}
    , sftpSessions_{}
    , channels_{}
{}
Session::~Session()
{
    stop();
}

void Session::stopReading()
{
    {
        std::scoped_lock guard{sessionMutex_};
        stopIssued_ = true;
    }

    if (processingThread_.joinable())
        processingThread_.join();

    {
        std::scoped_lock guard{sessionMutex_};
        stopIssued_ = false;
    }
}

void Session::stop()
{
    for (auto const& sftpSession : sftpSessions_)
    {
        sftpSession->disconnect();
    }

    stopReading();

    std::scoped_lock guard{sessionMutex_};
    session_.disconnect();
}

void Session::startReading()
{
    processingThread_ = std::thread([this]() {
        doChannelProcessing();
    });
}

void Session::pauseProcessing(bool paused)
{
    pauseProcessing_ = paused;
    {
        std::unique_lock guard{processingPauseMutex_};
        auto result =
            processingCondition_.wait_for(guard, std::chrono::milliseconds{250ull * channels_.size()}, [this]() {
                return pauseReached_;
            });
        if (result)
        {
            Log::info("Pause reached");
        }
        else if (!pauseReached_)
        {
            Log::error("Pause not reached");
        }
    }
}

void Session::withChannelDo(Ids::ChannelId const& id, std::function<void(Channel*)> const& handler)
{
    std::unique_lock guard{sessionMutex_};
    auto it = channels_.find(id);
    if (it == channels_.end())
    {
        guard.unlock();
        handler(nullptr);
    }
    handler(it->second.get());
}

void Session::doChannelProcessing()
{
    while (!stopIssued_)
    {
        if (pauseProcessing_)
        {
            {
                std::scoped_lock guard{processingPauseMutex_};
                pauseReached_ = true;
            }
            processingCondition_.notify_all();
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
            continue;
        }

        bool empty = false;
        {
            std::scoped_lock guard{sessionMutex_};
            empty = channels_.empty();
        }

        if (empty)
        {
            // TODO: A condition variable here for stop would make this better.
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
            continue;
        }

        std::chrono::milliseconds pollTimeout{std::max(10ull, 100ull / channels_.size())};
        for (auto& [id, channel] : channels_)
        {
            if (stopIssued_)
                break;

            if (!channel->processOnce(pollTimeout))
            {
                Log::error("Failed to process channel: {}", id.value());
                closeChannel(id);
            }
        }
    }
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

bool Session::closeChannel(Ids::ChannelId const& id)
{
    std::scoped_lock guard{sessionMutex_};
    auto it = channels_.find(id);
    if (it == channels_.end())
        return false;
    it->second->close();
    channels_.erase(it);
    return true;
}

std::expected<Ids::ChannelId, int>
Session::createPtyChannel(std::optional<std::unordered_map<std::string, std::string>> const& environment)
{
    const auto unpauseReading = Nui::ScopeExit{[this]() noexcept {
        // pauseProcessing(false);
        startReading();
    }};
    // pauseProcessing(true);
    stopReading();

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
    {
        Log::error("Failed to create pty channel: {}", session_.getError());
        return std::unexpected(session_.getErrorCode());
    }

    const auto id = Ids::generateChannelId();

    // make unique impossible due to private constructor
    std::scoped_lock guard{sessionMutex_};
    channels_[id] = std::unique_ptr<Channel>{new Channel(std::move(ptyChannel), true)};
    return id;
}