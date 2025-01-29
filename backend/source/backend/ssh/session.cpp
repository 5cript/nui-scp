#include <backend/ssh/session.hpp>
#include <backend/ssh/sequential.hpp>

using namespace Detail;

Session::Session()
    : stopIssued_{}
    , sessionMutex_{}
    , processingThread_{}
    , session_{}
    , ptyChannel_{}
    , environment_{}
    , onStdout_{}
    , onStderr_{}
    , onExit_{}
    , queuedWrites_{}
{}
Session::~Session()
{
    stop();
}

void Session::stop()
{
    std::scoped_lock guard{sessionMutex_};
    onExit_ = {};
    if (ptyChannel_)
    {
        stopIssued_ = true;
        if (processingThread_.joinable())
            processingThread_.join();
        if (ptyChannel_->isOpen())
        {
            ptyChannel_->sendEof();
            ptyChannel_->close();
        }
        ptyChannel_.reset();
    }
    session_.disconnect();
}

void Session::doProcessing()
{
    if (!ptyChannel_)
        return;

    constexpr static int bufferSize = 1024;
    char buffer[bufferSize];

    auto readOne = [this, &buffer](bool stdout_) {
        auto rdy = ssh_channel_poll_timeout(ptyChannel_->getCChannel(), 100, stdout_ ? 0 : 1);
        if (rdy < 0)
            return -1;

        if (rdy == 0)
            return 0;

        while (rdy > 0)
        {
            const auto toRead = std::min(bufferSize, rdy);
            const auto bytesRead = ssh_channel_read(ptyChannel_->getCChannel(), buffer, toRead, stdout_ ? 0 : 1);
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

        std::unique_lock tryGuard{sessionMutex_, std::try_to_lock};
        if (tryGuard.owns_lock())
        {
            if (!queuedWrites_.empty())
            {
                for (auto const& data : queuedWrites_)
                {
                    ptyChannel_->write(data.c_str(), data.size());
                }
                queuedWrites_.clear();
            }
        }
    }

    {
        std::scoped_lock guard{sessionMutex_};
        if (onExit_)
            onExit_();
    }
}

void Session::write(std::string const& data)
{
    if (!ptyChannel_)
        return;

    std::scoped_lock guard{sessionMutex_};
    queuedWrites_.push_back(data);
}

int Session::createPtyChannel()
{
    ptyChannel_ = std::make_unique<ssh::Channel>(session_);
    auto& channel = *ptyChannel_;
    auto result = sequential(
        [&channel]() {
            if (!channel.isOpen())
                return channel.openSession();
            return 0;
        },
        [this, &channel]() {
            if (!environment_.has_value())
                return 0;
            for (auto const& [key, value] : *environment_)
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
    return result.result;
}

void Session::setPtyEnvironment(std::unordered_map<std::string, std::string> const& env)
{
    environment_ = env;
}
int Session::resizePty(int cols, int rows)
{
    if (!ptyChannel_)
        return 0;

    return ptyChannel_->changePtySize(cols, rows);
}

void Session::startReading(
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