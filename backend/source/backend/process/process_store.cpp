#include <backend/process/process_store.hpp>

#include <boost/process/v2.hpp>
#include <nlohmann/json.hpp>
#include <roar/utility/base64.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef _WIN32
#    include <backend/pty/windows/conpty.hpp>
#endif

#include <iostream>

using namespace std::chrono_literals;
namespace bp2 = boost::process::v2;

namespace
{
    struct ProcessInfo
    {
        bool isPty{false};
    };

    enum class ProcessAttachedState
    {
        PseudoConsole = 0,
        ProcessInfo = 1
    };
}

ProcessStore::ProcessStore(boost::asio::any_io_executor executor)
    : executor_{std::move(executor)}
    , processes_{}
    , uuidGenerator_{}
{}
ProcessStore::~ProcessStore()
{}

std::string ProcessStore::emplace(
    std::string const& command,
    std::vector<std::string> const& arguments,
    Environment const& environment,
    bool isPty,
    std::chrono::seconds defaultExitWaitTimeout)
{
    const auto processId = boost::uuids::to_string(uuidGenerator_());
    processes_[processId] = std::make_shared<Process>(executor_);
    processes_[processId]->attachState(ProcessAttachedState::ProcessInfo, ProcessInfo{isPty});

    if (isPty)
    {
#ifdef _WIN32
        using namespace ConPTY;
        auto pty = createPseudoConsole();
        processes_[processId]->attachState(0, std::move(pty));

        bp2::windows::default_launcher launcher;
        auto& pty2 = processes_[processId]->getState<ConPTY::PseudoConsole>(ProcessAttachedState::PseudoConsole);
        pty2.prepareProcessLauncher(launcher);
        processes_[processId]->spawn(command, arguments, environment, defaultExitWaitTimeout, &launcher);
        pty2.closeOtherPipeEnd();
#endif
    }
    else
    {
        processes_[processId]->spawn(command, arguments, environment, defaultExitWaitTimeout);
    }
    return processId;
}

void ProcessStore::pruneDeadProcesses()
{
    for (auto iter = processes_.begin(); iter != processes_.end();)
    {
        if (!iter->second->running())
        {
            iter = processes_.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void ProcessStore::registerRpc(Nui::Window& wnd, Nui::RpcHub& hub)
{
    hub.registerFunction(
        "ProcessStore::spawn",
        [this, hub = &hub, wnd = &wnd](std::string const& responseId, nlohmann::json const& parameters) {
            try
            {
                std::cout << parameters.dump(4) << std::endl;

                const auto command = parameters.at("command").get<std::string>();
                const auto arguments = parameters.at("arguments").get<std::vector<std::string>>();
                const auto environment =
                    parameters.at("environment").get<std::unordered_map<std::string, std::string>>();
                const std::chrono::seconds defaultExitWaitTimeout =
                    std::chrono::seconds{parameters.at("defaultExitWaitTimeout").get<int>()};

                const auto isPty = parameters.at("isPty").get<bool>();
                const auto stdoutReceptacle = parameters.at("stdout").get<std::string>();
                const auto stderrReceptacle = parameters.at("stderr").get<std::string>();
                const auto cleanEnvironment = parameters.at("cleanEnvironment").get<bool>();
                const auto pathExtension = parameters.at("pathExtension").get<std::string>();

                Environment env;
                if (!cleanEnvironment)
                    env.loadFromCurrent();

                if (!pathExtension.empty())
                    env.extendPath(pathExtension);

                env.merge(environment);
                auto uuid = std::make_shared<std::string>();
                const auto processId = emplace(command, arguments, std::move(env), isPty, defaultExitWaitTimeout);

                *uuid = processId;

#ifdef _WIN32
                if (processes_[processId]->getState<ProcessInfo>(ProcessAttachedState::ProcessInfo).isPty)
                {
                    auto& pty =
                        processes_[processId]->getState<ConPTY::PseudoConsole>(ProcessAttachedState::PseudoConsole);
                    pty.startReading(
                        [wnd, hub, stdoutReceptacle, uuid](std::string_view message) {
                            wnd->runInJavascriptThread([hub, stdoutReceptacle, uuid, msg = std::string{message}]() {
                                hub->callRemote(
                                    stdoutReceptacle,
                                    nlohmann::json{{"uuid", *uuid}, {"data", Roar::base64Encode(msg)}});
                            });
                        },
                        [wnd, hub, stderrReceptacle, uuid](std::string_view message) {
                            wnd->runInJavascriptThread([hub, stderrReceptacle, uuid, msg = std::string{message}]() {
                                hub->callRemote(
                                    stderrReceptacle,
                                    nlohmann::json{{"uuid", *uuid}, {"data", Roar::base64Encode(msg)}});
                            });
                        });
                }
                else
                {
                    processes_[processId]->startReading(
                        [hub, stdoutReceptacle, uuid](std::string_view message) {
                            hub->callRemote(
                                stdoutReceptacle,
                                nlohmann::json{{"uuid", *uuid}, {"data", Roar::base64Encode(std::string{message})}});
                            return true;
                        },
                        [hub, stderrReceptacle, uuid](std::string_view message) {
                            hub->callRemote(
                                stderrReceptacle,
                                nlohmann::json{{"uuid", *uuid}, {"data", Roar::base64Encode(std::string{message})}});
                            return true;
                        });
                }
#else
#endif

                hub->callRemote(responseId, nlohmann::json{{"uuid", processId}});
            }
            catch (std::exception const& e)
            {
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "ProcessStore::terminate", [this, hub = &hub](std::string const& responseId, nlohmann::json const& parameters) {
            try
            {
                const auto uuid = parameters.at("uuid").get<std::string>();

                auto process = processes_.find(uuid);
                if (process == processes_.end())
                {
                    hub->callRemote(responseId, nlohmann::json{{"error", "Process not found"}});
                    return;
                }

                process->second->exit(0s);
                processes_.erase(process);
                hub->callRemote(responseId, nlohmann::json{{"success", true}});
            }
            catch (std::exception const& e)
            {
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "ProcessStore::exit", [this, hub = &hub](std::string const& responseId, std::string const& uuid) {
            try
            {
                auto process = processes_.find(uuid);
                if (process == processes_.end())
                {
                    hub->callRemote(responseId, nlohmann::json{{"error", "Process not found"}});
                    return;
                }

                process->second->exit(3s);
                processes_.erase(process);
                hub->callRemote(responseId, nlohmann::json{{"success", true}});
            }
            catch (std::exception const& e)
            {
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "ProcessStore::write",
        [this, hub = &hub](std::string const& responseId, std::string const& uuid, std::string const& data) {
            try
            {
                auto process = processes_.find(uuid);
                if (process == processes_.end())
                {
                    hub->callRemote(responseId, nlohmann::json{{"error", "Process not found"}});
                    return;
                }

#ifdef _WIN32
                if (process->second->getState<ProcessInfo>(ProcessAttachedState::ProcessInfo).isPty)
                {
                    auto& pty = process->second->getState<ConPTY::PseudoConsole>(ProcessAttachedState::PseudoConsole);
                    pty.write(Roar::base64Decode(data));
                }
                else
                    process->second->write(Roar::base64Decode(data) + "\r");
#endif
                hub->callRemote(responseId, nlohmann::json{{"success", true}});
            }
            catch (std::exception const& e)
            {
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "ProcessStore::ptyResize",
        [this, hub = &hub](std::string const& responseId, std::string const& uuid, int cols, int rows) {
            try
            {
                auto process = processes_.find(uuid);
                if (process == processes_.end())
                {
                    hub->callRemote(responseId, nlohmann::json{{"error", "Process not found"}});
                    return;
                }

#ifdef _WIN32
                if (process->second->getState<ProcessInfo>(ProcessAttachedState::ProcessInfo).isPty)
                {
                    auto& pty = process->second->getState<ConPTY::PseudoConsole>(ProcessAttachedState::PseudoConsole);
                    pty.resize(cols, rows);
                }
#endif
            }
            catch (std::exception const& e)
            {
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });
}