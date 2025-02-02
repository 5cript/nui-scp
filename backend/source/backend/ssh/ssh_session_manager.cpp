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
                const auto fileMode = !parameters.contains("stdout") || !parameters.contains("stderr");

                std::string stdoutReceptacle{};
                std::string stderrReceptacle{};
                if (!fileMode)
                {
                    stdoutReceptacle = parameters.at("stdout").get<std::string>();
                    stderrReceptacle = parameters.at("stderr").get<std::string>();
                }
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
                     fileMode,
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

                        if (!fileMode)
                        {
                            const auto result = sessions_[uuid]->createPtyChannel();

                            if (result != 0)
                            {
                                Log::error("Failed to create pty channel: {}", result);
                                sessions_.erase(uuid);
                                hub->callRemote(responseId, nlohmann::json{{"error", "Failed to create pty channel"}});
                                return;
                            }

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
                        }

                        hub->callRemote(responseId, nlohmann::json{{"uuid", uuid}});
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
        "SshSessionManager::sftp::listDirectory",
        [this, hub = &hub](std::string const& responseId, std::string const& uuid, std::string const& path) {
            try
            {
                if (sessions_.find(uuid) == sessions_.end())
                {
                    Log::error("No session found with id: {}", uuid);
                    hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
                    return;
                }

                auto& session = sessions_[uuid];
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