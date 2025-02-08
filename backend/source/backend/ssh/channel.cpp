#include <backend/ssh/channel.hpp>

Channel::Channel(std::unique_ptr<ssh::Channel> channel, bool isPty)
    : stopIssued_{false}
    , channel_{std::move(channel)}
    , isPty_{isPty}
{}

Channel::~Channel()
{
    stop();
}

void Channel::stop()
{
    std::scoped_lock guard{channelMutex_};

    // Do not fire onExit if stop was issued, this is only for when the channel itself issues that its no longer
    // useable.
    onExit_ = {};

    if (channel_)
    {
        stopIssued_ = true;
        if (processingThread_.joinable())
            processingThread_.join();
        if (channel_->isOpen())
        {
            channel_->sendEof();
            channel_->close();
        }
        channel_.reset();
    }
}

int Channel::resizePty(int cols, int rows)
{
    return channel_->changePtySize(cols, rows);
}

void Channel::doProcessing()
{
    constexpr static int bufferSize = 1024;
    char buffer[bufferSize];

    auto readOne = [this, &buffer](bool stdout_) {
        auto rdy = ssh_channel_poll_timeout(channel_->getCChannel(), 100, stdout_ ? 0 : 1);
        if (rdy < 0)
            return -1;

        if (rdy == 0)
            return 0;

        while (rdy > 0)
        {
            const auto toRead = std::min(bufferSize, rdy);
            const auto bytesRead = ssh_channel_read(channel_->getCChannel(), buffer, toRead, stdout_ ? 0 : 1);
            if (bytesRead <= 0)
                return -1;

            std::string data{buffer, static_cast<std::size_t>(bytesRead)};
            if (stdout_)
                onStdout_(data);
            else
                onStderr_(data);

            rdy -= bytesRead;
        }
        return rdy;
    };

    while (!stopIssued_)
    {
        if (readOne(true) < 0)
            break;
        // Lets not read stderr for now and see what happens:
        // readOne(false);

        std::unique_lock tryGuard{channelMutex_, std::try_to_lock};
        if (tryGuard.owns_lock())
        {
            if (!queuedWrites_.empty())
            {
                for (auto const& data : queuedWrites_)
                {
                    channel_->write(data.c_str(), data.size());
                }
                queuedWrites_.clear();
            }
        }
    }

    {
        if (stopIssued_)
        {
            if (onExit_)
                onExit_();
        }
        else
        {
            std::scoped_lock guard{channelMutex_};
            if (onExit_)
                onExit_();
        }
    }
}

void Channel::write(std::string const& data)
{
    std::scoped_lock guard{channelMutex_};
    queuedWrites_.push_back(data);
}

void Channel::startReading(
    std::function<void(std::string const&)> onStdout,
    std::function<void(std::string const&)> onStderr,
    std::function<void()> onExit)
{
    onStdout_ = onStdout;
    onStderr_ = onStderr;
    onExit_ = onExit;

    processingThread_ = std::thread([this]() {
        doProcessing();
    });
}