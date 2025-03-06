#include <backend/ssh_session_manager.hpp>
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
{}

SshSessionManager::~SshSessionManager()
{
    joinSessionAdder();
}

void SshSessionManager::registerRpcConnect(Nui::Window&, Nui::RpcHub& hub)
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
}

void SshSessionManager::registerRpcCreateChannel(Nui::Window&, Nui::RpcHub& hub)
{
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

            const auto weakChannel = sessions_[sessionId]->createPtyChannel({.environment = env}).get();
            if (!weakChannel.has_value())
            {
                Log::error("Failed to create pty channel: {}", weakChannel.error());
                hub->callRemote(responseId, nlohmann::json{{"error", "Failed to create pty channel"}});
                return;
            }

            const auto channelId = Ids::generateChannelId();
            channels_.emplace(channelId, std::move(weakChannel).value());

            Log::info(
                "Created pty channel with id '{}', channel total is now '{}'.", channelId.value(), channels_.size());

            hub->callRemote(responseId, nlohmann::json{{"id", channelId.value()}});
        });
}

void SshSessionManager::registerRpcStartChannelRead(Nui::Window& wnd, Nui::RpcHub& hub)
{
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

            if (channels_.find(channelId) == channels_.end())
            {
                Log::error("No channel found with id: {}", channelId.value());
                hub->callRemote(responseId, nlohmann::json{{"error", "No channel found with id"}});
                return;
            }

            auto& channel = channels_[channelId];

            auto locked = channel.lock();
            if (!locked)
            {
                Log::error("Failed to lock channel with id: {}", channelId.value());
                hub->callRemote(responseId, nlohmann::json{{"error", "Failed to lock channel"}});
                return;
            }

            const std::string stdoutReceptacle{"sshTerminalStdout_" + channelId.value()};
            const std::string stderrReceptacle{"sshTerminalStderr_" + channelId.value()};
            const std::string exitReceptacle{"sshTerminalOnExit_" + channelId.value()};

            locked->startReading(
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
                    Log::info("Channel for session '{}' lost with id: {}", sessionId.value(), channelId.value());

                    auto iter = channels_.find(channelId);
                    if (iter != channels_.end())
                    {
                        if (auto locked = iter->second.lock(); locked)
                        {
                            locked->close();
                        }
                        channels_.erase(iter);
                    }

                    wnd->runInJavascriptThread([hub, sessionId, channelId, exitReceptacle]() {
                        hub->callRemote(
                            exitReceptacle,
                            nlohmann::json{{"sessionId", sessionId.value()}, {"channelId", channelId.value()}});
                    });
                });

            hub->callRemote(responseId, nlohmann::json{{"success", true}});
        });
}

void SshSessionManager::registerRpcChannelClose(Nui::Window&, Nui::RpcHub& hub)
{
    hub.registerFunction(
        "SshSessionManager::Session::closeChannel",
        [this, hub = &hub](std::string const& responseId, std::string const& channelIdString) {
            try
            {
                const auto channelId = Ids::makeChannelId(channelIdString);

                if (auto iter = channels_.find(channelId); iter != channels_.end())
                {
                    if (auto channel = iter->second.lock(); channel)
                    {
                        channel->close();
                    }
                    channels_.erase(iter);
                    Log::info(
                        "Closed channel with id '{}', now remaining channels total '{}'.",
                        channelId.value(),
                        channels_.size());
                }
                else
                {
                    Log::error("No channel found with id: {}", channelId.value());
                    hub->callRemote(responseId, nlohmann::json{{"error", "No channel found with id"}});
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
}

void SshSessionManager::registerRpcEndSession(Nui::Window&, Nui::RpcHub& hub)
{
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
}

void SshSessionManager::registerRpcChannelWrite(Nui::Window&, Nui::RpcHub& hub)
{
    hub.registerFunction(
        "SshSessionManager::Channel::write",
        [this, hub = &hub](std::string const& responseId, std::string const& channelIdString, std::string const& data) {
            try
            {
                const auto channelId = Ids::makeChannelId(channelIdString);

                if (auto iter = channels_.find(channelId); iter != channels_.end())
                {
                    if (auto channel = iter->second.lock(); channel)
                    {
                        channel->write(Roar::base64Decode(data));
                    }
                    else
                    {
                        Log::error("Failed to get channel: {}", channelId.value());
                        hub->callRemote(responseId, nlohmann::json{{"error", "Failed to get channel"}});
                        return;
                    }
                }
                else
                {
                    Log::error("No channel found with id: {}", channelId.value());
                    hub->callRemote(responseId, nlohmann::json{{"error", "No channel found with id"}});
                    return;
                }
            }
            catch (std::exception const& e)
            {
                Log::error("Error writing to pty: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });
}

void SshSessionManager::registerRpcChannelPtyResize(Nui::Window&, Nui::RpcHub& hub)
{
    hub.registerFunction(
        "SshSessionManager::Channel::ptyResize",
        [this, hub = &hub](std::string const& responseId, std::string const& channelIdString, int cols, int rows) {
            try
            {
                const auto channelId = Ids::makeChannelId(channelIdString);

                if (auto iter = channels_.find(channelId); iter != channels_.end())
                {
                    if (auto channel = iter->second.lock(); channel)
                    {
                        channel->resizePty(cols, rows);
                        // FIXME: Why does this always fail?
                        // if (fut.wait_for(std::chrono::milliseconds{500}) != std::future_status::ready)
                        // {
                        //     Log::error("Failed to resize pty: timeout");
                        //     hub->callRemote(responseId, nlohmann::json{{"error", "Failed to resize pty: timeout"}});
                        //     return;
                        // }
                        // const auto result = fut.get();
                        // if (result != 0)
                        // {
                        //     Log::error("Failed to resize pty: {}", result);
                        //     hub->callRemote(responseId, nlohmann::json{{"error", "Failed to resize pty"}});
                        //     return;
                        // }
                        hub->callRemote(responseId, nlohmann::json{{"success", true}});
                    }
                    else
                    {
                        Log::error("Failed to get channel: {}", channelId.value());
                        hub->callRemote(responseId, nlohmann::json{{"error", "Failed to get channel"}});
                        return;
                    }
                }
                else
                {
                    Log::error("No channel found with id: {}", channelId.value());
                    hub->callRemote(responseId, nlohmann::json{{"error", "No channel found with id"}});
                    return;
                }
            }
            catch (std::exception const& e)
            {
                Log::error("Error resizing pty: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });
}

void SshSessionManager::registerRpc(Nui::Window& wnd, Nui::RpcHub& hub)
{
    registerRpcConnect(wnd, hub);
    registerRpcCreateChannel(wnd, hub);
    registerRpcStartChannelRead(wnd, hub);
    registerRpcChannelClose(wnd, hub);
    registerRpcEndSession(wnd, hub);
    registerRpcChannelWrite(wnd, hub);
    registerRpcChannelPtyResize(wnd, hub);

    // hub.registerFunction(
    //     "SshSessionManager::sftp::listDirectory",
    //     [this, hub = &hub](std::string const& responseId, std::string const& sessionIdString, std::string const&
    //     path) {
    //         try
    //         {
    //             const auto sessionId = Ids::makeSessionId(sessionIdString);

    //             if (sessions_.find(sessionId) == sessions_.end())
    //             {
    //                 Log::error("No session found with id: {}", sessionId.value());
    //                 hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
    //                 return;
    //             }

    //             auto& session = sessions_[sessionId];
    //             auto sftpSession = session->getSftpSession();
    //             if (!sftpSession)
    //             {
    //                 Log::error("Failed to create sftp session");
    //                 hub->callRemote(responseId, nlohmann::json{{"error", "Failed to create sftp session"}});
    //                 return;
    //             }

    //             auto result = sftpSession->listDirectory(path);
    //             if (!result.has_value())
    //             {
    //                 Log::error("Failed to list directory: {}", result.error().message);
    //                 hub->callRemote(responseId, nlohmann::json{{"error", result.error().message}});
    //                 return;
    //             }

    //             hub->callRemote(responseId, nlohmann::json{{"entries", *result}});
    //         }
    //         catch (std::exception const& e)
    //         {
    //             Log::error("Error listing directory: {}", e.what());
    //             hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
    //             return;
    //         }
    //     });

    // hub.registerFunction(
    //     "SshSessionManager::sftp::createDirectory",
    //     [this, hub = &hub](std::string const& responseId, std::string const& sessionIdString, std::string const&
    //     path) {
    //         try
    //         {
    //             const auto sessionId = Ids::makeSessionId(sessionIdString);

    //             if (sessions_.find(sessionId) == sessions_.end())
    //             {
    //                 Log::error("No session found with id: {}", sessionId.value());
    //                 hub->callRemote(responseId, nlohmann::json{{"error", "No session found with id"}});
    //                 return;
    //             }

    //             auto& session = sessions_[sessionId];
    //             auto sftpSession = session->getSftpSession();
    //             if (!sftpSession)
    //             {
    //                 Log::error("Failed to create sftp session");
    //                 hub->callRemote(responseId, nlohmann::json{{"error", "Failed to create sftp session"}});
    //                 return;
    //             }

    //             auto result = sftpSession->createDirectory(path);
    //             if (result)
    //             {
    //                 Log::error("Failed to create directory: {}", result->message);
    //                 hub->callRemote(responseId, nlohmann::json{{"error", result->message}});
    //                 return;
    //             }

    //             hub->callRemote(responseId, nlohmann::json{{"success", true}});
    //         }
    //         catch (std::exception const& e)
    //         {
    //             Log::error("Error creating directory: {}", e.what());
    //             hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
    //             return;
    //         }
    //     });
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
        std::pair<SshSessionManager*, std::string> askPassUserDataKeyPhrase{this, "Key phrase"};
        std::pair<SshSessionManager*, std::string> askPassUserDataPassword{this, "Password"};
        auto maybeSession =
            makeSession(engine, askPassDefault, &askPassUserDataKeyPhrase, &askPassUserDataPassword, &pwCache_);
        if (maybeSession)
        {
            const auto sessionId = Ids::SessionId{Ids::generateId()};
            sessions_.emplace(sessionId, std::move(maybeSession).value());
            sessions_[sessionId]->start();
            onComplete(sessionId);
        }
        else
        {
            Log::error("Failed to create session: {}", maybeSession.error());
            onComplete(std::nullopt);
        }
    });
}