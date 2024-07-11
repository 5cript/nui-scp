#include <backend/ssh/ssh_session_manager.hpp>
#include <libssh/libssh.h>
#include <roar/filesystem/special_paths.hpp>
#include <log/log.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

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
    std::thread t{[=, &pwPromise]() {
        std::lock_guard<std::mutex> lock{manager->passwordProvidersMutex_};
        auto iter = manager->passwordProviders_.begin();
        auto end = manager->passwordProviders_.end();
        std::function<void()> askNextProvider;
        askNextProvider = [&iter, end, &askNextProvider, prompt, &pwPromise, &whatFor]() {
            if (iter == end)
            {
                pwPromise.set_value(std::nullopt);
                return;
            }

            iter->second->getPassword(whatFor, prompt, [&](std::optional<std::string> pw) {
                if (pw.has_value())
                    pwPromise.set_value(pw);
                else
                    askNextProvider();
            });
        };

        askNextProvider();
    }};

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

void SshSessionManager::registerRpc(Nui::RpcHub&)
{}

void SshSessionManager::addPasswordProvider(int priority, PasswordProvider* provider)
{
    std::lock_guard lock{passwordProvidersMutex_};
    passwordProviders_.emplace(priority, provider);
}

bool SshSessionManager::addSession(Persistence::SshTerminalEngine const& engine)
{
    auto session = std::make_unique<Session>();

    const bool tryAgent = engine.tryAgentForAuthentication ? engine.tryAgentForAuthentication.value() : false;

    auto result = sequential(
        [&] {
            if (engine.logVerbosity)
                return session->session.setOption(SSH_OPTIONS_LOG_VERBOSITY_STR, engine.logVerbosity.value().c_str());
            return 0;
        },
        [&] {
            int port = 22;
            if (engine.port)
                port = engine.port.value();
            return session->session.setOption(SSH_OPTIONS_PORT, &port);
        },
        [&] {
            return session->session.setOption(SSH_OPTIONS_HOST, engine.host.c_str());
        },
        [&] {
            if (engine.bindAddr.has_value())
                return session->session.setOption(SSH_OPTIONS_BINDADDR, engine.bindAddr.value().c_str());
            return 0;
        },
        [&] {
            if (engine.user.has_value())
                return session->session.setOption(SSH_OPTIONS_USER, engine.user.value().c_str());
            return 0;
        },
        [&] {
            if (engine.sshDirectory.has_value())
                session->session.setOption(SSH_OPTIONS_SSH_DIR, engine.sshDirectory.value().c_str());
            return 0;
        },
        [&] {
            if (engine.knownHostsFile.has_value())
                session->session.setOption(SSH_OPTIONS_KNOWNHOSTS, engine.knownHostsFile.value().c_str());
            return 0;
        },
        [&] {
            long timeout = 0;
            if (engine.connectTimeoutSeconds.has_value())
            {
                timeout = engine.connectTimeoutSeconds.value();
                return session->session.setOption(SSH_OPTIONS_TIMEOUT, &timeout);
            }
            return 0;
        },
        [&] {
            long timeout = 0;
            if (engine.connectTimeoutUSeconds.has_value())
            {
                timeout = engine.connectTimeoutUSeconds.value();
                return session->session.setOption(SSH_OPTIONS_TIMEOUT_USEC, &timeout);
            }
            return 0;
        },
        [&] {
            if (engine.keyExchangeAlgorithms.has_value())
                return session->session.setOption(
                    SSH_OPTIONS_KEY_EXCHANGE, engine.keyExchangeAlgorithms.value().c_str());
            return 0;
        },
        [&] {
            if (engine.compressionClientToServer.has_value())
                return session->session.setOption(
                    SSH_OPTIONS_COMPRESSION_C_S, engine.compressionClientToServer.value().c_str());
            return 0;
        },
        [&] {
            if (engine.compressionServerToClient.has_value())
                return session->session.setOption(
                    SSH_OPTIONS_COMPRESSION_S_C, engine.compressionServerToClient.value().c_str());
            return 0;
        },
        [&] {
            if (engine.compressionLevel.has_value())
                return session->session.setOption(SSH_OPTIONS_COMPRESSION_LEVEL, engine.compressionLevel.value());
            return 0;
        },
        [&] {
            if (engine.strictHostKeyCheck)
            {
                int strict = engine.strictHostKeyCheck.value() ? 1 : 0;
                return session->session.setOption(SSH_OPTIONS_STRICTHOSTKEYCHECK, &strict);
            }
            return 0;
        },
        [&] {
            if (engine.proxyCommand)
                return session->session.setOption(SSH_OPTIONS_PROXYCOMMAND, engine.proxyCommand.value().c_str());
            return 0;
        },
        [&] {
            if (engine.gssapiServerIdentity)
                return session->session.setOption(
                    SSH_OPTIONS_GSSAPI_SERVER_IDENTITY, engine.gssapiServerIdentity.value().c_str());
            return 0;
        },
        [&] {
            if (engine.gssapiDelegateCredentials)
            {
                return session->session.setOption(
                    SSH_OPTIONS_GSSAPI_DELEGATE_CREDENTIALS, engine.gssapiDelegateCredentials.value() ? 1 : 0);
            }
            return 0;
        },
        [&] {
            if (engine.gssapiClientIdentity)
                return session->session.setOption(
                    SSH_OPTIONS_GSSAPI_CLIENT_IDENTITY, engine.gssapiClientIdentity.value().c_str());
            return 0;
        },
        [&] {
            if (engine.noDelay)
            {
                int noDelay = engine.noDelay.value() ? 1 : 0;
                return session->session.setOption(SSH_OPTIONS_NODELAY, &noDelay);
            }
            return 0;
        },
        [&] {
            if (engine.sshKey)
            {
                int pubkeyAuth = 1;
                return session->session.setOption(SSH_OPTIONS_PUBKEY_AUTH, &pubkeyAuth);
            }
            return 0;
        },
        [&] {
            if (engine.bypassConfig)
            {
                int processConfig = engine.bypassConfig.value() ? 0 : 1;
                return session->session.setOption(SSH_OPTIONS_PROCESS_CONFIG, &processConfig);
            }
            return 0;
        },
        [&] {
            if (engine.identityAgent)
                return session->session.setOption(SSH_OPTIONS_IDENTITY_AGENT, engine.identityAgent.value().c_str());
            return 0;
        },
        [&] {
            return session->session.connect();
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

            const auto result = ssh_userauth_agent(session->session.getCSession(), nullptr);
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

    if (result.result == SSH_AUTH_ERROR)
        return false;

    if (result.result != SSH_AUTH_SUCCESS && engine.usePublicKeyAutoAuth && engine.usePublicKeyAutoAuth.value())
    {
        result = sequential([&] {
            return session->session.userauthPublickeyAuto();
        });
    }

    if (result.result != SSH_AUTH_SUCCESS && engine.sshKey)
    {
        const auto sshKey = engine.sshKey.value();

        ssh_key key{nullptr};
        result = sequential(
            [this, &sshKey, &key]() {
                std::pair<SshSessionManager*, std::string> data{this, "KEY_PASSPHRASE"};
                return ssh_pki_import_privkey_file(sshKey.c_str(), nullptr, &askPassDefault, &data, &key);
            },
            [&key, &session]() {
                return session->session.userauthPublickey(key);
            });
    }

    if (result.result != SSH_AUTH_SUCCESS)
    {
        std::pair<SshSessionManager*, std::string> data{this, "PASSWORD"};
        std::string buf(1024, '\0');
        result = sequential([&] {
            const auto r = askPassDefault("Password: ", buf.data(), buf.size(), 0, 0, &data);
            if (r == 0)
                return session->session.userauthPassword(buf.data());
            else
                return (int)SSH_AUTH_DENIED;
        });
    }

    std::stringstream sstr;
    sstr << boost::uuids::random_generator()();
    sessions_[sstr.str()] = std::move(session);
    return result.success();
}