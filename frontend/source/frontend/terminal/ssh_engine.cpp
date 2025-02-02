#include <frontend/terminal/ssh_engine.hpp>
#include <frontend/nlohmann_compat.hpp>
#include <log/log.hpp>

#include <nui/frontend/val.hpp>
#include <nui/rpc.hpp>

#include <exception>

using namespace std::string_literals;

struct SshTerminalEngine::Implementation
{
    SshTerminalEngine::Settings settings;
    std::string id;

    Nui::RpcClient::AutoUnregister stdoutReceiver;
    Nui::RpcClient::AutoUnregister stderrReceiver;
    Nui::RpcClient::AutoUnregister onExitReceiver;

    std::string sshSessionId;

    std::function<void(std::string const&)> stdoutHandler;
    std::function<void(std::string const&)> stderrHandler;

    Implementation(SshTerminalEngine::Settings&& settings)
        : settings{std::move(settings)}
        , id{Nui::val::global("generateId")().as<std::string>()}
        , stdoutReceiver{}
        , stderrReceiver{}
        , onExitReceiver{}
        , sshSessionId{}
        , stdoutHandler{}
        , stderrHandler{}
    {}
};

SshTerminalEngine::SshTerminalEngine(Settings settings)
    : impl_{std::make_unique<Implementation>(std::move(settings))}
{}
SshTerminalEngine::~SshTerminalEngine()
{
    if (!moveDetector_.wasMoved())
    {
        dispose();
    }
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(SshTerminalEngine);

void SshTerminalEngine::open(std::function<void(bool, std::string const&)> onOpen, bool fileMode)
{
    if (!fileMode)
    {
        impl_->stdoutReceiver =
            Nui::RpcClient::autoRegisterFunction("sshTerminalStdout_" + impl_->id, [this](Nui::val val) {
                if (val.hasOwnProperty("data"))
                {
                    const std::string data = Nui::val::global("atob")(val["data"]).as<std::string>();
                    if (impl_->stdoutHandler)
                        impl_->stdoutHandler(data);
                }
                else
                    Log::info("sshTerminalStdout_" + impl_->id + " received an empty message");
            });

        impl_->stderrReceiver =
            Nui::RpcClient::autoRegisterFunction("sshTerminalStderr_" + impl_->id, [this](Nui::val val) {
                if (val.hasOwnProperty("data"))
                {
                    const std::string data = Nui::val::global("atob")(val["data"]).as<std::string>();
                    if (impl_->stderrHandler)
                        impl_->stderrHandler(data);
                }
                else
                    Log::error("sshTerminalStderr_" + impl_->id + " received an empty message");
            });
    }

    impl_->onExitReceiver = Nui::RpcClient::autoRegisterFunction("sshTerminalOnExit_" + impl_->id, [this](Nui::val) {
        Nui::RpcClient::callWithBackChannel(
            "SshSessionManager::disconnect",
            [this](Nui::val) {
                if (impl_->settings.onExit)
                    impl_->settings.onExit();
            },
            impl_->sshSessionId);
    });

    Nui::val obj = Nui::val::object();
    obj.set("engine", asVal(impl_->settings.engineOptions));
    if (!fileMode)
    {
        obj.set("stdout", "sshTerminalStdout_"s + impl_->id);
        obj.set("stderr", "sshTerminalStderr_"s + impl_->id);
    }
    obj.set("onExit", "sshTerminalOnExit_"s + impl_->id);

    Nui::RpcClient::callWithBackChannel(
        "SshSessionManager::connect",
        [this, onOpen = std::move(onOpen)](Nui::val val) {
            if (!val.hasOwnProperty("uuid"))
            {
                Log::error("SshSessionManager::connect callback did not return a uuid");
                std::string error = "";
                if (val.hasOwnProperty("error"))
                    error = val["error"].as<std::string>();
                return onOpen(false, error);
            }
            std::string uuid = val["uuid"].as<std::string>();
            impl_->sshSessionId = uuid;

            onOpen(true, "");
        },
        obj);
}

std::string SshTerminalEngine::sshSessionId() const
{
    return impl_->sshSessionId;
}

void SshTerminalEngine::dispose()
{
    Nui::RpcClient::callWithBackChannel(
        "SshSessionManager::disconnect",
        [](Nui::val) {
            // TODO: handle error
        },
        impl_->sshSessionId);

    impl_->stdoutReceiver.reset();
    impl_->stderrReceiver.reset();
    impl_->sshSessionId.clear();
}

void SshTerminalEngine::resize(int cols, int rows)
{
    Nui::RpcClient::callWithBackChannel(
        "SshSessionManager::ptyResize",
        [](Nui::val) {
            // TODO: handle error
        },
        impl_->sshSessionId,
        cols,
        rows);
}

void SshTerminalEngine::write(std::string const& data)
{
    Nui::RpcClient::callWithBackChannel(
        "SshSessionManager::write",
        [](Nui::val) {},
        impl_->sshSessionId,
        Nui::val::global("btoa")(data).as<std::string>());
}

void SshTerminalEngine::setStdoutHandler(std::function<void(std::string const&)> handler)
{
    impl_->stdoutHandler = std::move(handler);
}
void SshTerminalEngine::setStderrHandler(std::function<void(std::string const&)> handler)
{
    impl_->stderrHandler = std::move(handler);
}