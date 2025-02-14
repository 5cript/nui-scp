#include <ssh/channel.hpp>
#include <ssh/session.hpp>

namespace SecureShell
{
    Channel::Channel(Session* owner, std::unique_ptr<ssh::Channel> channel)
        : owner_{owner}
        , channel_{std::move(channel)}
    {}
    Channel::~Channel()
    {
        /* close makes no sense, since this lives in a shared_ptr and will only ever end here when it was already
         * removed */
    }
    Channel::Channel(Channel&& other)
        : owner_{std::exchange(other.owner_, nullptr)}
    {}
    Channel& Channel::operator=(Channel&& other)
    {
        if (this != &other)
        {
            owner_ = std::exchange(other.owner_, nullptr);
        }
        return *this;
    }
    void Channel::close()
    {
        if (owner_)
        {
            if (readTaskId_)
            {
                owner_->processingThread_.removePermanentTask(*readTaskId_);
            }

            owner_->channelRemoveItself(this);
            owner_ = nullptr;
        }
    }
    void Channel::write(std::string data)
    {
        owner_->processingThread_.pushTask([this, data = std::move(data)]() {
            if (channel_)
                channel_->write(data.data(), data.size());
        });
    }
    std::future<int> Channel::resizePty(int cols, int rows)
    {
        auto promise = std::make_shared<std::promise<int>>();
        owner_->processingThread_.pushTask([this, cols, rows, promise]() {
            if (channel_)
                channel_->changePtySize(cols, rows);
        });
        return promise->get_future();
    }
    void Channel::readTask(std::chrono::milliseconds pollTimeout)
    {
        if (!onStdout_ || !onStderr_ || !onExit_)
            return;

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
            return;
        }
        if (readOne(false) < 0)
        {
            if (onExit_)
                onExit_();
            return;
        }
    }
    void Channel::startReading(
        std::function<void(std::string const&)> onStdout,
        std::function<void(std::string const&)> onStderr,
        std::function<void()> onExit)
    {
        onStdout_ = std::move(onStdout);
        onStderr_ = std::move(onStderr);
        onExit_ = std::move(onExit);

        auto [success, id] = owner_->processingThread_.pushPermanentTask([this]() {
            readTask();
        });
        if (!success)
        {
            // TODO: ???
        }
        else
        {
            readTaskId_ = id;
        }
    }
}