#include <backend/ssh/channel.hpp>

Channel::Channel(std::unique_ptr<ssh::Channel> channel, bool isPty)
    : channel_{std::move(channel)}
    , isPty_{isPty}
{}

Channel::~Channel()
{
    close();
}

void Channel::close()
{
    // Do not fire onExit if stop was issued, this is only for when the channel itself issues that its no longer
    // useable.
    onExit_ = {};
    isProcessingReady_ = false;

    if (channel_)
    {
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

bool Channel::processOnce(std::chrono::milliseconds pollTimeout)
{
    if (!isProcessingReady_)
        return true;

    constexpr static int bufferSize = 1024;
    char buffer[bufferSize];

    auto readOne = [this, &buffer, pollTimeout](bool stdout_) {
        auto rdy = ssh_channel_poll_timeout(channel_->getCChannel(), pollTimeout.count(), stdout_ ? 0 : 1);
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

    if (readOne(true) < 0)
    {
        if (onExit_)
            onExit_();
        return false;
    }
    if (readOne(false) < 0)
    {
        if (onExit_)
            onExit_();
        return false;
    }

    if (!queuedWrites_.empty())
    {
        for (auto const& data : queuedWrites_)
        {
            channel_->write(data.c_str(), data.size());
        }
        queuedWrites_.clear();
    }

    return true;
}

void Channel::write(std::string const& data)
{
    queuedWrites_.push_back(data);
}

void Channel::startReading(
    std::function<void(std::string const&)> onStdout,
    std::function<void(std::string const&)> onStderr,
    std::function<void()> onExit)
{
    isProcessingReady_ = true;
    onStdout_ = onStdout;
    onStderr_ = onStderr;
    onExit_ = onExit;
}