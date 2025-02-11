#include <backend/ssh/ssh_session_manager.hpp>
#include <backend/ssh/sequential.hpp>
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

using namespace Detail;

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

SshSessionManager::SshSessionManager()
    : sessions_{}
{}

SshSessionManager::~SshSessionManager()
{
    joinSessionAdder();
}

void SshSessionManager::registerRpc(Nui::Window& wnd, Nui::RpcHub& hub)
{
    /*
    {
        engine: {
            sshSessionOptions: Persistence::SshSessionOptions,
            environment: std::unordered_map<std::string, std::string>
        },
    }
    */
    hub.registerFunction(
        "SshSessionManager::connect",
        [this, hub = &hub](std::string const& responseId, nlohmann::json const& parameters) {
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

                const auto engine = parameters["engine"].get<Persistence::SshTerminalEngine>();
                joinSessionAdder();
                addSession(engine, [responseId, hub](auto const& maybeId) {
                    if (!maybeId)
                    {
                        Log::error("Failed to connect to ssh server");
                        hub->callRemote(responseId, nlohmann::json{{"error", "Failed to connect to ssh server"}});
                        return;
                    }

                    Log::info("Connected to ssh server with id: {}", maybeId->value());
                    hub->callRemote(responseId, nlohmann::json{{"id", maybeId->value()}});
                });
            }
            catch (std::exception const& e)
            {
                Log::error("Error connecting to ssh server: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    /*
    {
        sessionId: string,
        engine: {
            sshSessionOptions: Persistence::SshSessionOptions,
            environment: std::unordered_map<std::string, std::string>
        }
    }
    */
    hub.registerFunction(
        "SshSessionManager::Session::createChannel",
        [this, hub = &hub](std::string const& responseId, nlohmann::json const& parameters) {
            if (!parameters.contains("sessionId"))
            {
                Log::error("No session id specified for channel creation");
                hub->callRemote(responseId, nlohmann::json{{"error", "No session id specified for channel creation"}});
                return;
            }

            const auto sessionId = Ids::makeSessionId(parameters.at("sessionId").get<std::string>());

            if (sessions_.find(sessionId) == sessions_.end())
            {
                Log::error("No session found with id: {}", sessionId.value());
                hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                return;
            }

            if (!parameters.contains("engine"))
            {
                Log::error("No engine specified for ssh channel");
                hub->callRemote(responseId, nlohmann::json{{"error", "No engine specified for ssh connection"}});
                return;
            }

            const auto sessionOptions = parameters["engine"]["sshSessionOptions"].get<Persistence::SshSessionOptions>();
            auto env = sessionOptions.environment;

            const auto channelId = sessions_[sessionId]->createPtyChannel(env);
            if (!channelId.has_value())
            {
                Log::error("Failed to create pty channel: {}", channelId.error());
                hub->callRemote(responseId, nlohmann::json{{"error", "Failed to create pty channel"}});
                return;
            }

            hub->callRemote(responseId, nlohmann::json{{"id", channelId.value().value()}});
        });

    hub.registerFunction(
        "SshSessionManager::Channel::startReading",
        [this, hub = &hub, wnd = &wnd](
            std::string const& responseId, std::string const& sessionIdString, std::string const& channelIdString) {
            const auto sessionId = Ids::makeSessionId(sessionIdString);
            const auto channelId = Ids::makeChannelId(channelIdString);

            if (sessions_.find(sessionId) == sessions_.end())
            {
                Log::error("No session found with id: {}", sessionId.value());
                hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                return;
            }

            auto& session = sessions_[sessionId];

            session->withChannelDo(channelId, [&, hub, wnd](auto* channel) {
                if (!channel)
                {
                    Log::error("No channel found with id: {}", channelId.value());
                    hub->callRemote(responseId, nlohmann::json{{"error", "No channel found with id"}});
                    return;
                }

                const std::string stdoutReceptacle{"sshTerminalStdout_" + channelId.value()};
                const std::string stderrReceptacle{"sshTerminalStderr_" + channelId.value()};
                const std::string exitReceptacle{"sshTerminalOnExit_" + channelId.value()};

                channel->startReading(
                    [wnd, hub, sessionId, channelId, stdoutReceptacle](std::string const& msg) {
                        wnd->runInJavascriptThread([hub, stdoutReceptacle, sessionId, channelId, msg]() {
                            hub->callRemote(
                                stdoutReceptacle,
                                nlohmann::json{
                                    {"sessionId", sessionId.value()},
                                    {"channelId", channelId.value()},
                                    {"data", Roar::base64Encode(msg)}});
                        });
                    },
                    [wnd, hub, sessionId, channelId, stderrReceptacle](std::string const& data) {
                        wnd->runInJavascriptThread([hub, stderrReceptacle, sessionId, channelId, data]() {
                            hub->callRemote(
                                stderrReceptacle,
                                nlohmann::json{
                                    {"sessionId", sessionId.value()},
                                    {"channelId", channelId.value()},
                                    {"data", Roar::base64Encode(data)}});
                        });
                    },
                    [this, wnd, hub, sessionId, channelId, exitReceptacle]() {
                        wnd->runInJavascriptThread([this, hub, sessionId, channelId, exitReceptacle]() {
                            Log::info(
                                "Channel for session '{}' lost with id: {}", sessionId.value(), channelId.value());
                            sessions_[sessionId]->closeChannel(channelId);
                            hub->callRemote(
                                exitReceptacle,
                                nlohmann::json{{"sessionId", sessionId.value()}, {"channelId", channelId.value()}});
                        });
                    });
            });

            hub->callRemote(responseId, nlohmann::json{{"success", true}});
        });

    hub.registerFunction(
        "SshSessionManager::Session::closeChannel",
        [this, hub = &hub](
            std::string const& responseId, std::string const& sessionIdString, std::string const& channelIdString) {
            try
            {
                const auto sessionId = Ids::makeSessionId(sessionIdString);
                const auto channelId = Ids::makeChannelId(channelIdString);

                if (sessions_.find(sessionId) == sessions_.end())
                {
                    Log::error("No session found with id: {}", sessionId.value());
                    hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                    return;
                }

                auto& session = sessions_[sessionId];
                if (!session->closeChannel(channelId))
                {
                    Log::error("Failed to close channel with id: {}", channelId.value());
                    hub->callRemote(responseId, nlohmann::json{{"error", "Failed to close channel"}});
                    return;
                }

                hub->callRemote(responseId, nlohmann::json{{"success", true}});
            }
            catch (std::exception const& e)
            {
                Log::error("Error closing channel: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "SshSessionManager::disconnect",
        [this, hub = &hub](std::string const& responseId, std::string const& sessionIdString) {
            try
            {
                const auto sessionId = Ids::makeSessionId(sessionIdString);

                if (sessions_.find(sessionId) == sessions_.end())
                {
                    // Do not log this, because multi delete is not an error
                    hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                    return;
                }
                Log::info("Disconnecting from ssh server with id: {}", sessionId.value());
                sessions_.erase(sessionId);
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
        "SshSessionManager::sftp::listDirectory",
        [this, hub = &hub](std::string const& responseId, std::string const& sessionIdString, std::string const& path) {
            try
            {
                const auto sessionId = Ids::makeSessionId(sessionIdString);

                if (sessions_.find(sessionId) == sessions_.end())
                {
                    Log::error("No session found with id: {}", sessionId.value());
                    hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                    return;
                }

                auto& session = sessions_[sessionId];
                auto sftpSession = session->getSftpSession();
                if (!sftpSession)
                {
                    Log::error("Failed to create sftp session");
                    hub->callRemote(responseId, nlohmann::json{{"error", "Failed to create sftp session"}});
                    return;
                }

                auto result = sftpSession->listDirectory(path);
                if (!result.has_value())
                {
                    Log::error("Failed to list directory: {}", result.error().message);
                    hub->callRemote(responseId, nlohmann::json{{"error", result.error().message}});
                    return;
                }

                hub->callRemote(responseId, nlohmann::json{{"entries", *result}});
            }
            catch (std::exception const& e)
            {
                Log::error("Error listing directory: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "SshSessionManager::sftp::createDirectory",
        [this, hub = &hub](std::string const& responseId, std::string const& sessionIdString, std::string const& path) {
            try
            {
                const auto sessionId = Ids::makeSessionId(sessionIdString);

                if (sessions_.find(sessionId) == sessions_.end())
                {
                    Log::error("No session found with id: {}", sessionId.value());
                    hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                    return;
                }

                auto& session = sessions_[sessionId];
                auto sftpSession = session->getSftpSession();
                if (!sftpSession)
                {
                    Log::error("Failed to create sftp session");
                    hub->callRemote(responseId, nlohmann::json{{"error", "Failed to create sftp session"}});
                    return;
                }

                auto result = sftpSession->createDirectory(path);
                if (result)
                {
                    Log::error("Failed to create directory: {}", result->message);
                    hub->callRemote(responseId, nlohmann::json{{"error", result->message}});
                    return;
                }

                hub->callRemote(responseId, nlohmann::json{{"success", true}});
            }
            catch (std::exception const& e)
            {
                Log::error("Error creating directory: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "SshSessionManager::Channel::write",
        [this, hub = &hub](
            std::string const& responseId,
            std::string const& sessionIdString,
            std::string const& channelIdString,
            std::string const& data) {
            try
            {
                const auto sessionId = Ids::makeSessionId(sessionIdString);
                const auto channelId = Ids::makeChannelId(channelIdString);

                if (sessions_.find(sessionId) == sessions_.end())
                {
                    Log::error("No session found with id: {}", sessionId.value());
                    hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                    return;
                }

                auto& session = sessions_[sessionId];

                session->withChannelDo(channelId, [hub, &responseId, &channelId, &data](auto* channel) {
                    if (!channel)
                    {
                        Log::error("Failed to get channel: {}", channelId.value());
                        hub->callRemote(responseId, nlohmann::json{{"error", "Failed to get channel"}});
                        return;
                    }
                    channel->write(Roar::base64Decode(data));
                });
            }
            catch (std::exception const& e)
            {
                Log::error("Error writing to pty: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "SshSessionManager::Channel::ptyResize",
        [this, hub = &hub](
            std::string const& responseId,
            std::string const& sessionIdString,
            std::string const& channelIdString,
            int cols,
            int rows) {
            try
            {
                const auto sessionId = Ids::makeSessionId(sessionIdString);
                const auto channelId = Ids::makeChannelId(channelIdString);

                if (sessions_.find(sessionId) == sessions_.end())
                {
                    Log::error("No session found with id: {}", sessionId.value());
                    hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                    return;
                }

                auto& session = sessions_[sessionId];

                session->withChannelDo(channelId, [hub, &responseId, &channelId, &cols, &rows](auto* channel) {
                    if (!channel)
                    {
                        Log::error("Failed to get channel: {}", channelId.value());
                        hub->callRemote(responseId, nlohmann::json{{"error", "Failed to get channel"}});
                        return;
                    }

                    const auto result = channel->resizePty(cols, rows);
                    if (result != 0)
                    {
                        Log::error("Failed to resize pty: {}", result);
                        hub->callRemote(responseId, nlohmann::json{{"error", "Failed to resize pty"}});
                        return;
                    }
                    hub->callRemote(responseId, nlohmann::json{{"success", true}});
                });
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
    std::function<void(std::optional<Ids::SessionId> const&)> onComplete)
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

#ifndef _WIN32
        // TODO: Implement this for Windows (its complicated, because there is no one single agent)
        const bool tryAgent =
            sshOptions.tryAgentForAuthentication ? sshOptions.tryAgentForAuthentication.value() : false;
#endif

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
                        SSH_OPTIONS_SSH_DIR, sshOptions.sshDirectory.value().generic_string().c_str());
                return 0;
            },
            [&] {
                if (sshOptions.knownHostsFile.has_value())
                    static_cast<ssh::Session&>(*session).setOption(
                        SSH_OPTIONS_KNOWNHOSTS, sshOptions.knownHostsFile.value().generic_string().c_str());
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
                std::optional<std::string> pwFromCache{};
                for (const auto& cache : pwCache_)
                {
                    if (cache.user == sessionOptions.user && cache.host == sessionOptions.host &&
                        cache.port == sessionOptions.port)
                    {
                        pwFromCache = cache.password;
                        break;
                    }
                }

                if (pwFromCache.has_value())
                {
                    buf = pwFromCache.value();
                    const auto result = static_cast<ssh::Session&>(*session).userauthPassword(buf.data());
                    if (result == SSH_AUTH_SUCCESS)
                        return (int)SSH_AUTH_SUCCESS;
                }

                const auto r = askPassDefault("Password: ", buf.data(), buf.size(), 0, 0, &data);
                if (r == 0)
                {
                    const auto result = static_cast<ssh::Session&>(*session).userauthPassword(buf.data());
                    if (result == SSH_AUTH_SUCCESS)
                    {
                        pwCache_.push_back(PwCache{sessionOptions.user, sessionOptions.host, sessionOptions.port, buf});
                        return (int)SSH_AUTH_SUCCESS;
                    }
                }
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
        const auto sessionId = Ids::makeSessionId(sstr.str());

        if (sessions_.find(sessionId) != sessions_.end())
        {
            Log::error("Failed to generate unique session id?!");
            // intentional std::terminate
            throw std::runtime_error("Failed to generate unique session id");
        }

        sessions_[sessionId] = std::move(session);
        Log::info("Ssh session created with id: {}", sessionId.value());
        sessions_[sessionId]->startReading();
        onComplete(sessionId);
    });
}