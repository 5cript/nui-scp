#include <backend/ssh/ssh_session_manager.hpp>
#include <log/log.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <libssh/libssh.h>
#include <roar/utility/base64.hpp>
#include <roar/filesystem/special_paths.hpp>

#include <optional>
#include <thread>
#include <future>
#include <mutex>
#include <cstring>
#include <sstream>

namespace
{
    struct SequenceResult
    {
        int result{0};
        int index{0};
        int total{0};

        bool reachedEnd() const
        {
            return index == total;
        }
        bool success() const
        {
            return result == 0;
        }
    };

    template <typename... T>
    SequenceResult sequential(T&&... functions)
    {
        int result = 0;
        int index = 0;

        const auto single = [&](auto&& fn) {
            ++index;
            result = fn();
            return result;
        };

        [[maybe_unused]] const bool allRan = ((single(functions) == 0) && ...);

        return {result, index, sizeof...(functions)};
    }
}

int askPassDefault(char const* prompt, char* buf, std::size_t length, int, int, void* userdata)
{
    std::pair<SshSessionManager*, std::string> const* data =
        static_cast<std::pair<SshSessionManager*, std::string>*>(userdata);
    auto* manager = data->first;
    std::string whatFor = data->second;

    std::promise<std::optional<std::string>> pwPromise{};
    std::lock_guard<std::mutex> lock{manager->passwordProvidersMutex_};
    std::function<void(decltype(manager->passwordProviders_)::const_iterator)> askNextProvider;

    askNextProvider = [end = manager->passwordProviders_.end(), &askNextProvider, prompt, &pwPromise, &whatFor](
                          decltype(manager->passwordProviders_)::const_iterator iter) {
        if (iter == end)
        {
            pwPromise.set_value(std::nullopt);
            return;
        }

        iter->second->getPassword(
            whatFor, prompt, [iter, &pwPromise, &askNextProvider](std::optional<std::string> pw) mutable {
                if (pw.has_value())
                    pwPromise.set_value(pw);
                else
                {
                    ++iter;
                    askNextProvider(iter);
                }
            });
    };
    askNextProvider(manager->passwordProviders_.begin());

    const auto pw = pwPromise.get_future().get();

    if (pw.has_value())
    {
        std::memset(buf, 0, length);
        std::strncpy(buf, pw.value().c_str(), length - 1);
        return 0;
    }
    return -1;
}

SshSessionManager::Session::Session()
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
SshSessionManager::Session::~Session()
{
    stop();
}

void SshSessionManager::Session::stop()
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

void SshSessionManager::Session::doProcessing()
{
    if (!ptyChannel_)
        return;

    constexpr static int bufferSize = 1024;
    char buffer[bufferSize];

    auto readOne = [this, &buffer](bool stdout) {
        auto rdy = ssh_channel_poll_timeout(ptyChannel_->getCChannel(), 100, stdout ? 0 : 1);
        if (rdy < 0)
            return -1;

        if (rdy == 0)
            return 0;

        while (rdy > 0)
        {
            const auto toRead = std::min(bufferSize, rdy);
            const auto bytesRead = ssh_channel_read(ptyChannel_->getCChannel(), buffer, toRead, stdout ? 0 : 1);
            if (bytesRead <= 0)
                return -1;

            std::string data{buffer, static_cast<std::size_t>(bytesRead)};
            if (stdout)
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

void SshSessionManager::Session::write(std::string const& data)
{
    if (!ptyChannel_)
        return;

    std::scoped_lock guard{sessionMutex_};
    queuedWrites_.push_back(data);
}

int SshSessionManager::Session::createPtyChannel()
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

void SshSessionManager::Session::setPtyEnvironment(std::unordered_map<std::string, std::string> const& env)
{
    environment_ = env;
}
int SshSessionManager::Session::resizePty(int cols, int rows)
{
    if (!ptyChannel_)
        return 0;

    return ptyChannel_->changePtySize(cols, rows);
}

void SshSessionManager::Session::startReading(
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

SshSessionManager::SshSessionManager()
    : sessions_{}
{}

SshSessionManager::~SshSessionManager()
{
    joinSessionAdder();
}

void SshSessionManager::registerRpc(Nui::Window& wnd, Nui::RpcHub& hub)
{
    hub.registerFunction(
        "SshSessionManager::connect",
        [this, hub = &hub, wnd = &wnd](std::string const& responseId, nlohmann::json const& parameters) {
            try
            {
                Log::info("Connecting to ssh server with parameters: {}", parameters.dump(4));
                if (!parameters.contains("engine"))
                {
                    Log::error("No engine specified for ssh connection");
                    hub->callRemote(responseId, nlohmann::json{{"error", "No engine specified for ssh connection"}});
                    return;
                }

                const auto sessionOptions =
                    parameters["engine"]["sshSessionOptions"].get<Persistence::SshSessionOptions>();
                // const auto sshOptions = sessionOptions.sshOptions.value();

                auto env = sessionOptions.environment;

                if (!parameters.contains("stdout"))
                {
                    Log::error("No stdout receptacle specified for ssh connection");
                    hub->callRemote(
                        responseId, nlohmann::json{{"error", "No stdout receptacle specified for ssh connection"}});
                    return;
                }

                if (!parameters.contains("stderr"))
                {
                    Log::error("No stderr receptacle specified for ssh connection");
                    hub->callRemote(
                        responseId, nlohmann::json{{"error", "No stderr receptacle specified for ssh connection"}});
                    return;
                }

                const auto stdoutReceptacle = parameters.at("stdout").get<std::string>();
                const auto stderrReceptacle = parameters.at("stderr").get<std::string>();
                const auto exitReceptacle = parameters.at("onExit").get<std::string>();

                const auto engine = parameters["engine"].get<Persistence::SshTerminalEngine>();
                joinSessionAdder();
                addSession(
                    engine,
                    [this,
                     wnd,
                     hub,
                     responseId,
                     stdoutReceptacle,
                     stderrReceptacle,
                     exitReceptacle,
                     env = std::move(env)](auto const& maybeId) {
                        if (!maybeId)
                        {
                            Log::error("Failed to create ssh session.");
                            hub->callRemote(responseId, nlohmann::json{{"error", "Failed to connect to ssh server"}});
                            return;
                        }

                        const auto uuid = maybeId.value();

                        if (env)
                            sessions_[uuid]->setPtyEnvironment(env.value());

                        Log::info("Connected to ssh server with id: {}", uuid);
                        const auto result = sessions_[uuid]->createPtyChannel();

                        if (result != 0)
                        {
                            Log::error("Failed to create pty channel: {}", result);
                            sessions_.erase(uuid);
                            hub->callRemote(responseId, nlohmann::json{{"error", "Failed to create pty channel"}});
                            return;
                        }

                        hub->callRemote(responseId, nlohmann::json{{"uuid", uuid}});

                        sessions_[uuid]->startReading(
                            [wnd, hub, uuid, stdoutReceptacle](std::string const& msg) {
                                wnd->runInJavascriptThread([hub, stdoutReceptacle, uuid, msg]() {
                                    hub->callRemote(
                                        stdoutReceptacle,
                                        nlohmann::json{{"uuid", uuid}, {"data", Roar::base64Encode(msg)}});
                                });
                            },
                            [wnd, hub, uuid, stderrReceptacle](std::string const& data) {
                                wnd->runInJavascriptThread([hub, stderrReceptacle, uuid, data]() {
                                    hub->callRemote(
                                        stderrReceptacle,
                                        nlohmann::json{{"uuid", uuid}, {"data", Roar::base64Encode(data)}});
                                });
                            },
                            [wnd, hub, uuid, exitReceptacle]() {
                                wnd->runInJavascriptThread([hub, uuid, exitReceptacle]() {
                                    Log::info("Ssh connection lost with id: {}", uuid);
                                    hub->callRemote(exitReceptacle, nlohmann::json{{"uuid", uuid}});
                                });
                            });
                    });
            }
            catch (std::exception const& e)
            {
                Log::error("Error connecting to ssh server: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "SshSessionManager::disconnect", [this, hub = &hub](std::string const& responseId, std::string const& uuid) {
            try
            {
                if (sessions_.find(uuid) == sessions_.end())
                {
                    // Do not log this, because multi delete is not an error
                    hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                    return;
                }
                Log::info("Disconnecting from ssh server with id: {}", uuid);
                sessions_.erase(uuid);
                hub->callRemote(responseId, nlohmann::json{{"success", true}});
            }
            catch (std::exception const& e)
            {
                Log::error("Error disconnecting to ssh server: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "SshSessionManager::write",
        [this, hub = &hub](std::string const& responseId, std::string const& uuid, std::string const& data) {
            try
            {
                if (sessions_.find(uuid) == sessions_.end())
                {
                    Log::error("No session found with id: {}", uuid);
                    hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                    return;
                }

                auto& session = sessions_[uuid];
                session->write(Roar::base64Decode(data));
            }
            catch (std::exception const& e)
            {
                Log::error("Error writing to pty: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "SshSessionManager::ptyResize",
        [this, hub = &hub](std::string const& responseId, std::string const& uuid, int cols, int rows) {
            try
            {
                if (sessions_.find(uuid) == sessions_.end())
                {
                    Log::error("No session found with id: {}", uuid);
                    hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                    return;
                }

                auto& session = sessions_[uuid];

                const auto result = session->resizePty(cols, rows);
                if (result != 0)
                {
                    Log::error("Failed to resize pty: {}", result);
                    hub->callRemote(responseId, nlohmann::json{{"error", "Failed to resize pty"}});
                    return;
                }
                hub->callRemote(responseId, nlohmann::json{{"success", true}});
            }
            catch (std::exception const& e)
            {
                Log::error("Error resizing pty: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });
}

void SshSessionManager::addPasswordProvider(int priority, PasswordProvider* provider)
{
    std::lock_guard lock{passwordProvidersMutex_};
    passwordProviders_.emplace(priority, provider);
}

void SshSessionManager::joinSessionAdder()
{
    std::scoped_lock guard{addSessionMutex_};
    if (addSessionThread_ && addSessionThread_->joinable())
    {
        addSessionThread_->join();
        addSessionThread_.reset();
    }
}

void SshSessionManager::addSession(
    Persistence::SshTerminalEngine const& engine,
    std::function<void(std::optional<std::string> const&)> onComplete)
{
    std::scoped_lock guard{addSessionMutex_};
    if (addSessionThread_ && addSessionThread_->joinable())
    {
        Log::error("Session creation already in progress");
        onComplete(std::nullopt);
    }

    addSessionThread_ = std::make_unique<std::thread>([this, engine, onComplete = std::move(onComplete)]() {
        auto session = std::make_unique<Session>();

        const auto sessionOptions = engine.sshSessionOptions.value();
        const auto sshOptions = sessionOptions.sshOptions.value();

        const bool tryAgent =
            sshOptions.tryAgentForAuthentication ? sshOptions.tryAgentForAuthentication.value() : false;

        auto result = sequential(
            [&] {
                if (sshOptions.logVerbosity)
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_LOG_VERBOSITY_STR, sshOptions.logVerbosity.value().c_str());
                return 0;
            },
            [&] {
                int port = 22;
                if (sessionOptions.port)
                    port = sessionOptions.port.value();
                return static_cast<ssh::Session&>(*session).setOption(SSH_OPTIONS_PORT, &port);
            },
            [&] {
                return static_cast<ssh::Session&>(*session).setOption(SSH_OPTIONS_HOST, sessionOptions.host.c_str());
            },
            [&] {
                if (sessionOptions.user.has_value())
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_USER, sessionOptions.user.value().c_str());
                return 0;
            },
            [&] {
                if (sshOptions.sshDirectory.has_value())
                    static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_SSH_DIR, sshOptions.sshDirectory.value().c_str());
                return 0;
            },
            [&] {
                if (sshOptions.knownHostsFile.has_value())
                    static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_KNOWNHOSTS, sshOptions.knownHostsFile.value().c_str());
                return 0;
            },
            [&] {
                long timeout = 0;
                if (sshOptions.connectTimeoutSeconds.has_value())
                {
                    timeout = sshOptions.connectTimeoutSeconds.value();
                    return static_cast<ssh::Session&>(*session).setOption(SSH_OPTIONS_TIMEOUT, &timeout);
                }
                return 0;
            },
            [&] {
                long timeout = 0;
                if (sshOptions.connectTimeoutUSeconds.has_value())
                {
                    timeout = sshOptions.connectTimeoutUSeconds.value();
                    return static_cast<ssh::Session&>(*session).setOption(SSH_OPTIONS_TIMEOUT_USEC, &timeout);
                }
                return 0;
            },
            [&] {
                if (sshOptions.keyExchangeAlgorithms.has_value())
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_KEY_EXCHANGE, sshOptions.keyExchangeAlgorithms.value().c_str());
                return 0;
            },
            [&] {
                if (sshOptions.compressionClientToServer.has_value())
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_COMPRESSION_C_S, sshOptions.compressionClientToServer.value().c_str());
                return 0;
            },
            [&] {
                if (sshOptions.compressionServerToClient.has_value())
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_COMPRESSION_S_C, sshOptions.compressionServerToClient.value().c_str());
                return 0;
            },
            [&] {
                if (sshOptions.compressionLevel.has_value())
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_COMPRESSION_LEVEL, sshOptions.compressionLevel.value());
                return 0;
            },
            [&] {
                if (sshOptions.strictHostKeyCheck)
                {
                    int strict = sshOptions.strictHostKeyCheck.value() ? 1 : 0;
                    return static_cast<ssh::Session&>(*session).setOption(SSH_OPTIONS_STRICTHOSTKEYCHECK, &strict);
                }
                return 0;
            },
            [&] {
                if (sshOptions.proxyCommand)
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_PROXYCOMMAND, sshOptions.proxyCommand.value().c_str());
                return 0;
            },
            [&] {
                if (sshOptions.gssapiServerIdentity)
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_GSSAPI_SERVER_IDENTITY, sshOptions.gssapiServerIdentity.value().c_str());
                return 0;
            },
            [&] {
                if (sshOptions.gssapiDelegateCredentials)
                {
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_GSSAPI_DELEGATE_CREDENTIALS, sshOptions.gssapiDelegateCredentials.value() ? 1 : 0);
                }
                return 0;
            },
            [&] {
                if (sshOptions.gssapiClientIdentity)
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_GSSAPI_CLIENT_IDENTITY, sshOptions.gssapiClientIdentity.value().c_str());
                return 0;
            },
            [&] {
                if (sshOptions.noDelay)
                {
                    int noDelay = sshOptions.noDelay.value() ? 1 : 0;
                    return static_cast<ssh::Session&>(*session).setOption(SSH_OPTIONS_NODELAY, &noDelay);
                }
                return 0;
            },
            [&] {
                if (sessionOptions.sshKey)
                {
                    int pubkeyAuth = 1;
                    return static_cast<ssh::Session&>(*session).setOption(SSH_OPTIONS_PUBKEY_AUTH, &pubkeyAuth);
                }
                return 0;
            },
            [&] {
                if (sshOptions.bypassConfig)
                {
                    int processConfig = sshOptions.bypassConfig.value() ? 0 : 1;
                    return static_cast<ssh::Session&>(*session).setOption(SSH_OPTIONS_PROCESS_CONFIG, &processConfig);
                }
                return 0;
            },
            [&] {
                if (sshOptions.identityAgent)
                    return static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_IDENTITY_AGENT, sshOptions.identityAgent.value().c_str());
                return 0;
            },
            [&] {
                return static_cast<ssh::Session&>(*session).connect();
            },
#ifdef _WIN32
            []()
#else
            [tryAgent, &session]()
#endif
            {
#ifdef _WIN32
                return static_cast<int>(SSH_AUTH_AGAIN);
#else
                if (!tryAgent)
                    return static_cast<int>(SSH_AUTH_AGAIN);

                const auto result = ssh_userauth_agent(static_cast<ssh::Session&>(*session).getCSession(), nullptr);
                switch (result)
                {
                    case SSH_AUTH_SUCCESS:
                        return static_cast<int>(SSH_AUTH_SUCCESS);
                    case SSH_AUTH_DENIED:
                        Log::error("Authentication denied");
                        return static_cast<int>(SSH_AUTH_DENIED);
                    case SSH_AUTH_ERROR:
                        Log::error("Authentication error");
                        return static_cast<int>(SSH_AUTH_ERROR);
                    case SSH_AUTH_PARTIAL:
                        Log::error("Partial authentication");
                        return static_cast<int>(SSH_AUTH_PARTIAL);
                    case SSH_AUTH_AGAIN:
                        Log::error("Authentication again");
                        return static_cast<int>(SSH_AUTH_AGAIN);
                    default:
                        Log::error("Unknown authentication result");
                        return result;
                }
#endif
            });

        if (result.result != SSH_AUTH_SUCCESS && sshOptions.usePublicKeyAutoAuth &&
            sshOptions.usePublicKeyAutoAuth.value())
        {
            result = sequential([&] {
                return static_cast<ssh::Session&>(*session).userauthPublickeyAuto();
            });
        }

        if (result.result != SSH_AUTH_SUCCESS && sessionOptions.sshKey)
        {
            const auto sshKey = sessionOptions.sshKey.value();

            ssh_key key{nullptr};
            result = sequential(
                [this, &sshKey, &key]() {
                    std::pair<SshSessionManager*, std::string> data{this, "KEY_PASSPHRASE"};
                    return ssh_pki_import_privkey_file(sshKey.c_str(), nullptr, &askPassDefault, &data, &key);
                },
                [&key, &session]() {
                    return static_cast<ssh::Session&>(*session).userauthPublickey(key);
                });
        }

        if (result.result != SSH_AUTH_SUCCESS)
        {
            std::pair<SshSessionManager*, std::string> data{this, "PASSWORD"};
            std::string buf(1024, '\0');
            result = sequential([&] {
                const auto r = askPassDefault("Password: ", buf.data(), buf.size(), 0, 0, &data);
                if (r == 0)
                    return static_cast<ssh::Session&>(*session).userauthPassword(buf.data());
                else
                    return (int)SSH_AUTH_DENIED;
            });
        }

        if (result.result != SSH_AUTH_SUCCESS)
        {
            std::string authResult = "";
            switch (result.result)
            {
                case SSH_AUTH_DENIED:
                    authResult = "Authentication denied";
                    break;
                case SSH_AUTH_ERROR:
                    authResult = "Authentication error";
                    break;
                case SSH_AUTH_PARTIAL:
                    authResult = "Partial authentication";
                    break;
                case SSH_AUTH_AGAIN:
                    authResult = "Authentication again";
                    break;
                default:
                    authResult = "Unknown authentication result";
                    break;
            }
            Log::error("Failed to authenticate: {}", authResult);
            onComplete(std::nullopt);
        }

        std::stringstream sstr;
        sstr << boost::uuids::random_generator()();
        const auto uuid = sstr.str();
        if (sessions_.find(uuid) != sessions_.end())
        {
            Log::error("Failed to generate unique session id?!");
            // intentional std::terminate
            throw std::runtime_error("Failed to generate unique session id");
        }
        sessions_[uuid] = std::move(session);
        Log::info("Ssh session created with id: {}", uuid);
        onComplete(uuid);
    });
}