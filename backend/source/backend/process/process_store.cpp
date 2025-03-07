#include <backend/process/process_store.hpp>

#include <backend/process/boost_process.hpp>
#include <csignal>
#include <nlohmann/json.hpp>
#include <roar/utility/base64.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <log/log.hpp>

#ifdef _WIN32
#    include <backend/pty/windows/conpty.hpp>
#else
#    include <backend/pty/linux/pty.hpp>
#endif

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

ProcessStore::ProcessStore(boost::asio::any_io_executor executor, Nui::Window& wnd, Nui::RpcHub& hub)
    : executor_{std::move(executor)}
    , wnd_{&wnd}
    , hub_{&hub}
    , processes_{}
    , uuidGenerator_{}
{}
ProcessStore::~ProcessStore()
{}

std::string ProcessStore::emplace(
    std::string const& command,
    std::vector<std::string> const& arguments,
    Environment const& environment,
#ifdef _WIN32
    Persistence::Termios const&,
#else
    Persistence::Termios const& termios,
#endif
    bool isPty,
    std::chrono::seconds defaultExitWaitTimeout)
{
    const auto processId = boost::uuids::to_string(uuidGenerator_());
    processes_[processId] = std::make_shared<Process>(executor_, [this, processId]() {
        wnd_->runInJavascriptThread([this, processId]() {
            notifyChildExit(*hub_, processId);
        });
    });
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
        processes_[processId]->spawn(
            command,
            arguments,
            environment,
            defaultExitWaitTimeout,
            [launcher = std::move(launcher)](
                auto executor, auto const& executable, auto const& arguments, auto const& env) mutable {
                return std::make_unique<bp2::process>(
                    launcher(executor, executable, arguments, bp2::process_environment{env}));
            });
        pty2.closeOtherPipeEnd();
#else
        using namespace PTY;
        auto pty = createPseudoTerminal(executor_, termios);
        if (!pty)
        {
            processes_.erase(processId);
            Log::error("Failed to create PTY");
        }
        Log::info("Created PTY");
        processes_[processId]->attachState(0, std::move(pty).value());

        auto& pty2 = processes_[processId]->getState<PTY::PseudoTerminal>(ProcessAttachedState::PseudoConsole);
        processes_[processId]->spawn(
            command,
            arguments,
            environment,
            defaultExitWaitTimeout,
            [&pty2](auto executor, auto const& executable, auto const& arguments, auto const& env) {
                bp2::posix::default_launcher launcher;
                return std::make_unique<bp2::process>(launcher(
                    executor, executable, arguments, bp2::process_environment{env}, pty2.makeProcessLauncherInit()));
            });
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

void ProcessStore::notifyChildExit(Nui::RpcHub& hub, std::string const& id)
{
    auto process = processes_.find(id);
    if (process == processes_.end())
    {
        Log::error("Process not found");
        return;
    }

    process->second->exit();
    processes_.erase(process);
    hub.callRemote("SessionArea::processDied", nlohmann::json{{"id", id}});
}

void ProcessStore::notifyChildExit(Nui::RpcHub& hub, long long pid)
{
    for (auto& [id, process] : processes_)
    {
        if (process->pid() == pid)
        {
            process->exit();
            const auto idCopy = id;
            processes_.erase(id);
            hub.callRemote("SessionArea::processDied", nlohmann::json{{"id", idCopy}, {"pid", pid}});
            return;
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
                Log::debug("Spawning process with parameters: {}", parameters.dump(4));

                const auto command = parameters.at("command").get<std::string>();
                const auto arguments = parameters.at("arguments").get<std::vector<std::string>>();
                const auto environment =
                    parameters.at("environment").get<std::unordered_map<std::string, std::string>>();
                const std::chrono::seconds defaultExitWaitTimeout =
                    std::chrono::seconds{parameters.at("defaultExitWaitTimeout").get<int>()};

                const auto isPty = parameters.at("isPty").get<bool>();
                const auto stdoutReceptacle = parameters.at("stdout").get<std::string>();
                const auto stderrReceptacle = parameters.at("stderr").get<std::string>();

                Environment env;

                if (parameters.contains("cleanEnvironment"))
                {
                    const auto cleanEnvironment = parameters.at("cleanEnvironment").get<bool>();
                    if (!cleanEnvironment)
                        env.loadFromCurrent();
                }
                else
                {
                    env.loadFromCurrent();
                }

                if (parameters.contains("pathExtension"))
                {
                    const auto pathExtension = parameters.at("pathExtension").get<std::string>();
                    if (!pathExtension.empty())
                        env.extendPath(pathExtension);
                }

                Persistence::Termios termios = {};
                if (parameters.contains("termios"))
                    termios = parameters.at("termios").get<Persistence::Termios>();

                env.merge(environment);
                auto id = std::make_shared<std::string>();
                const auto processId =
                    emplace(command, arguments, std::move(env), std::move(termios), isPty, defaultExitWaitTimeout);

                *id = processId;

#ifdef _WIN32
                using PtyType = ConPTY::PseudoConsole;
#else
                using PtyType = PTY::PseudoTerminal;
#endif

                hub->callRemote(responseId, nlohmann::json{{"id", processId}});

                if (processes_[processId]->getState<ProcessInfo>(ProcessAttachedState::ProcessInfo).isPty)
                {
                    Log::info("Starting PTY reading");
                    auto& pty = processes_[processId]->getState<PtyType>(ProcessAttachedState::PseudoConsole);
                    pty.startReading(
                        [wnd, hub, stdoutReceptacle, id](std::string_view message) {
                            wnd->runInJavascriptThread([hub, stdoutReceptacle, id, msg = std::string{message}]() {
                                hub->callRemote(
                                    stdoutReceptacle, nlohmann::json{{"id", *id}, {"data", Roar::base64Encode(msg)}});
                            });
                        },
                        [wnd, hub, stderrReceptacle, id](std::string_view message) {
                            wnd->runInJavascriptThread([hub, stderrReceptacle, id, msg = std::string{message}]() {
                                hub->callRemote(
                                    stderrReceptacle, nlohmann::json{{"id", *id}, {"data", Roar::base64Encode(msg)}});
                            });
                        });
                }
                else
                {
                    Log::info("Starting non-PTY reading");
                    processes_[processId]->startReading(
                        [hub, stdoutReceptacle, id, wnd](std::string_view message) {
                            wnd->runInJavascriptThread([hub, stdoutReceptacle, id, msg = std::string{message}]() {
                                hub->callRemote(
                                    stdoutReceptacle, nlohmann::json{{"id", *id}, {"data", Roar::base64Encode(msg)}});
                            });
                            return true;
                        },
                        [hub, stderrReceptacle, id, wnd](std::string_view message) {
                            wnd->runInJavascriptThread([hub, stderrReceptacle, id, msg = std::string{message}]() {
                                hub->callRemote(
                                    stderrReceptacle, nlohmann::json{{"id", *id}, {"data", Roar::base64Encode(msg)}});
                            });
                            return true;
                        });
                }
            }
            catch (std::exception const& e)
            {
                Log::error("Failed to spawn process: {}", e.what());
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "ProcessStore::terminate", [this, hub = &hub](std::string const& responseId, nlohmann::json const& parameters) {
            try
            {
                const auto id = parameters.at("id").get<std::string>();
                Log::info("Terminating process with UUID: {}", id);

                auto process = processes_.find(id);
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
        "ProcessStore::exit", [this, hub = &hub](std::string const& responseId, std::string const& id) {
            try
            {
                Log::info("Exiting process with UUID: {}", id);
                auto process = processes_.find(id);
                if (process == processes_.end())
                {
                    hub->callRemote(responseId, nlohmann::json{{"error", "Process not found"}});
                    return;
                }

                process->second->exit();
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
        [this, hub = &hub](std::string const& responseId, std::string const& id, std::string const& data) {
            try
            {
                auto process = processes_.find(id);
                if (process == processes_.end())
                {
                    hub->callRemote(responseId, nlohmann::json{{"error", "Process not found"}});
                    return;
                }

#ifdef _WIN32
                using PtyType = ConPTY::PseudoConsole;
#else
                using PtyType = PTY::PseudoTerminal;
#endif

                if (process->second->getState<ProcessInfo>(ProcessAttachedState::ProcessInfo).isPty)
                {
                    auto& pty = process->second->getState<PtyType>(ProcessAttachedState::PseudoConsole);
                    pty.write(Roar::base64Decode(data));
                }
                else
                    process->second->write(Roar::base64Decode(data) + "\r");

                hub->callRemote(responseId, nlohmann::json{{"success", true}});
            }
            catch (std::exception const& e)
            {
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "ProcessStore::ptyProcesses", [this, hub = &hub](std::string const& responseId, std::string const& id) {
            try
            {
                auto process = processes_.find(id);
                if (process == processes_.end())
                {
                    hub->callRemote(responseId, nlohmann::json{{"error", "Process not found"}});
                    return;
                }

#ifdef _WIN32
            // using PtyType = ConPTY::PseudoConsole;
#else
                using PtyType = PTY::PseudoTerminal;
#endif

                if (process->second->getState<ProcessInfo>(ProcessAttachedState::ProcessInfo).isPty)
                {
#ifdef _WIN32
                    hub->callRemote(responseId, nlohmann::json{{"error", "Not implemented"}});
#else
                    auto& pty = process->second->getState<PtyType>(ProcessAttachedState::PseudoConsole);
                    const auto procs = pty.listProcessesUnderPty();
                    if (procs.empty())
                    {
                        hub->callRemote(responseId, nlohmann::json{{"error", "No processes found"}});
                        return;
                    }
                    nlohmann::json j = nlohmann::json::object();
                    j["latest"] = {
                        {"pid", procs.front().pid},
                        {"cmdline", procs.front().cmdline},
                    };
                    j["all"] = nlohmann::json::array();
                    for (const auto& p : procs)
                    {
                        j["all"].push_back({
                            {"pid", p.pid},
                            {"cmdline", p.cmdline},
                        });
                    }
                    hub->callRemote(responseId, j);
#endif
                }
            }
            catch (std::exception const& e)
            {
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });

    hub.registerFunction(
        "ProcessStore::ptyResize",
        [this, hub = &hub](std::string const& responseId, std::string const& id, int cols, int rows) {
            try
            {
                Log::debug("Resizing PTY with UUID: {} to cols: {}, rows: {}", id, cols, rows);
                auto process = processes_.find(id);
                if (process == processes_.end())
                {
                    hub->callRemote(responseId, nlohmann::json{{"error", "Process not found"}});
                    return;
                }

#ifdef _WIN32
                using PtyType = ConPTY::PseudoConsole;
#else
                using PtyType = PTY::PseudoTerminal;
#endif

                if (process->second->getState<ProcessInfo>(ProcessAttachedState::ProcessInfo).isPty)
                {
                    auto& pty = process->second->getState<PtyType>(ProcessAttachedState::PseudoConsole);
                    pty.resize(cols, rows);
                }

#ifndef _WIN32
                // Does nothing ?
                process->second->signal(SIGWINCH);
#endif
            }
            catch (std::exception const& e)
            {
                hub->callRemote(responseId, nlohmann::json{{"error", e.what()}});
                return;
            }
        });
}