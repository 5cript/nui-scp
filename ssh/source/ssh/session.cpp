#include <ssh/session.hpp>
#include <ssh/sequential.hpp>

#include <fmt/format.h>

namespace SecureShell
{
    Session::Session()
        : processingThread_{}
        , session_{}
        , channels_{}
        , channelsToRemove_{}
    {}

    void Session::start()
    {
        processingThread_.start(std::chrono::milliseconds{100}, std::chrono::milliseconds{1});
    }

    void Session::stop()
    {
        processingThread_.stop();
    }

    bool Session::isRunning() const
    {
        return processingThread_.isRunning();
    }

    void Session::shutdown()
    {
        removeAllChannels();
        processingThread_.pushTask([this]() {
            session_.disconnect();
        });
        processingThread_.stop();
    }

    Session::~Session()
    {
        shutdown();
    }

    void Session::removeAllChannels()
    {
        processingThread_.pushTask([this]() {
            if (!channelsToRemove_.empty())
                removalTask();
            channelsToRemove_ = std::move(channels_);
            removalTask();
        });
    }

    void Session::channelRemoveItself(Channel* channel)
    {
        if (channel)
        {
            processingThread_.pushTask([this, channel]() {
                auto it = std::find_if(channels_.begin(), channels_.end(), [channel](const auto& c) {
                    return c.get() == channel;
                });
                if (it != channels_.end())
                {
                    channelsToRemove_.push_back(*it);
                    channels_.erase(it);
                    processingThread_.pushTask([this]() {
                        removalTask();
                    });
                }
                else
                {
                    // Possibly already flagged for removal
                }
            });
        }
    }

    void Session::removalTask()
    {
        for (const auto& toRemove : channelsToRemove_)
        {
            auto& channel = static_cast<ssh::Channel&>(*toRemove);
            if (channel.isOpen())
            {
                channel.sendEof();
                channel.close();
            }
        }
        channelsToRemove_.clear();
    }

    std::future<std::expected<std::weak_ptr<Channel>, int>>
    Session::createPtyChannel(std::optional<std::unordered_map<std::string, std::string>> environment)
    {
        auto promise = std::make_shared<std::promise<std::expected<std::weak_ptr<Channel>, int>>>();
        auto fut = promise->get_future();
        processingThread_.pushTask(
            [this, environment = std::move(environment), promise = std::move(promise)]() mutable {
                auto ptyChannel = std::make_unique<ssh::Channel>(session_);
                auto& channel = *ptyChannel;
                auto result = Detail::sequential(
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
                    promise->set_value(std::unexpected(session_.getErrorCode()));
                    return;
                }

                auto sharedChannel = std::make_shared<Channel>(this, std::move(ptyChannel));
                channels_.push_back(sharedChannel);
                promise->set_value(sharedChannel);
                return;
            });
        return fut;
    }

    std::expected<std::unique_ptr<Session>, std::string> makeSession(
        Persistence::SshTerminalEngine const& engine,
        AskPassCallback askPass,
        void* askPassUserDataKeyPhrase,
        void* askPassUserDataPassword,
        std::vector<PasswordCacheEntry>* pwCache)
    {
        auto session = std::make_unique<Session>();

        const auto sessionOptions = engine.sshSessionOptions.value();
        const auto sshOptions = sessionOptions.sshOptions.value();

#ifndef _WIN32
        // TODO: Implement this for Windows (its complicated, because there is no one single agent)
        const bool tryAgent =
            sshOptions.tryAgentForAuthentication ? sshOptions.tryAgentForAuthentication.value() : false;
#endif

        auto result = Detail::sequential(
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
            result = Detail::sequential([&] {
                return static_cast<ssh::Session&>(*session).userauthPublickeyAuto();
            });
        }

        if (result.result != SSH_AUTH_SUCCESS && sessionOptions.sshKey)
        {
            const auto sshKey = sessionOptions.sshKey.value();

            ssh_key key{nullptr};
            result = Detail::sequential(
                [&sshKey, &key, askPass, askPassUserDataKeyPhrase]() {
                    return ssh_pki_import_privkey_file(
                        sshKey.c_str(), nullptr, askPass, askPassUserDataKeyPhrase, &key);
                },
                [&key, &session]() {
                    return static_cast<ssh::Session&>(*session).userauthPublickey(key);
                });
        }

        if (result.result != SSH_AUTH_SUCCESS)
        {
            std::string buf(1024, '\0');
            result = Detail::sequential([&] {
                if (pwCache)
                {
                    std::optional<std::string> pwFromCache{};
                    for (const auto& cache : *pwCache)
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
                }

                const auto r = askPass("Password: ", buf.data(), buf.size(), 0, 0, &askPassUserDataPassword);
                if (r == 0)
                {
                    const auto result = static_cast<ssh::Session&>(*session).userauthPassword(buf.data());
                    if (result == SSH_AUTH_SUCCESS)
                    {
                        if (pwCache)
                            pwCache->emplace_back(sessionOptions.user, sessionOptions.host, sessionOptions.port, buf);
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
            return std::unexpected(fmt::format("Failed to authenticate: {}", authResult));
        }

        return session;
    }
}