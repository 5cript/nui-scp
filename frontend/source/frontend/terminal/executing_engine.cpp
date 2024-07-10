#include <exception>
#include <frontend/terminal/executing_engine.hpp>
#include <frontend/nlohmann_compat.hpp>
#include <log/log.hpp>

#include <nui/rpc.hpp>

using namespace std::string_literals;

struct ExecutingTerminalEngine::Implementation
{
    ExecutingTerminalEngine::Settings settings;
    std::string id;

    Nui::RpcClient::AutoUnregister stdoutReceiver;
    Nui::RpcClient::AutoUnregister stderrReceiver;

    std::string processId;

    std::function<void(std::string const&)> stdoutHandler;
    std::function<void(std::string const&)> stderrHandler;

    Implementation(ExecutingTerminalEngine::Settings&& settings)
        : settings{std::move(settings)}
        , id{Nui::val::global("nanoid")().as<std::string>()}
        , stdoutReceiver{}
        , stderrReceiver{}
        , processId{}
        , stdoutHandler{}
        , stderrHandler{}
    {}
};

ExecutingTerminalEngine::ExecutingTerminalEngine(Settings settings)
    : impl_{std::make_unique<Implementation>(std::move(settings))}
{}
ExecutingTerminalEngine::~ExecutingTerminalEngine() = default;

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(ExecutingTerminalEngine);

void ExecutingTerminalEngine::open(std::function<void(bool)> onOpen)
{
    impl_->stdoutReceiver =
        Nui::RpcClient::autoRegisterFunction("execTerminalStdout_" + impl_->id, [this](Nui::val val) {
            if (val.hasOwnProperty("data"))
            {
                const std::string data = Nui::val::global("atob")(val["data"]).as<std::string>();
                if (impl_->stdoutHandler)
                    impl_->stdoutHandler(data);
            }
            else
                Nui::Console::log("execTerminalStdout_" + impl_->id + " received an empty message");
        });

    impl_->stderrReceiver =
        Nui::RpcClient::autoRegisterFunction("execTerminalStderr_" + impl_->id, [this](Nui::val val) {
            if (val.hasOwnProperty("data"))
            {
                const std::string data = Nui::val::global("atob")(val["data"]).as<std::string>();
                if (impl_->stderrHandler)
                    impl_->stderrHandler(data);
            }
            else
                Nui::Console::error("execTerminalStderr_" + impl_->id + " received an empty message");
        });

    Nui::val obj = Nui::val::object();

    obj.set("command", impl_->settings.engineOptions.command);
    if (impl_->settings.engineOptions.arguments)
    {
        Nui::val args = Nui::val::array();
        for (auto const& arg : *impl_->settings.engineOptions.arguments)
        {
            args.call<void>("push", arg);
        }
        obj.set("arguments", args);
    }
    else
    {
        obj.set("arguments", Nui::val::array());
    }

    if (impl_->settings.engineOptions.environment)
    {
        Nui::val env = Nui::val::object();
        for (auto const& [key, value] : *impl_->settings.engineOptions.environment)
        {
            env.set(key.c_str(), value);
        }
        obj.set("environment", env);
    }
    else
    {
        obj.set("environment", Nui::val::object());
    }

    if (impl_->settings.engineOptions.exitTimeoutSeconds)
    {
        obj.set("defaultExitWaitTimeout", *impl_->settings.engineOptions.exitTimeoutSeconds);
    }
    if (impl_->settings.engineOptions.cleanEnvironment)
    {
        obj.set("cleanEnvironment", *impl_->settings.engineOptions.cleanEnvironment);
    }
    obj.set("isPty", impl_->settings.engineOptions.isPty);

    obj.set("stdout", "execTerminalStdout_" + impl_->id);
    obj.set("stderr", "execTerminalStderr_" + impl_->id);
    try
    {
        obj.set("termios", asVal(impl_->settings.termios));
    }
    catch (std::exception const& exc)
    {
        Log::error("Failed to serialize termios: {}", exc.what());
    }

    Nui::RpcClient::callWithBackChannel(
        "ProcessStore::spawn",
        [this, onOpen = std::move(onOpen)](Nui::val val) {
            Nui::Console::log("ProcessStore::spawn callback", val);

            if (!val.hasOwnProperty("uuid"))
            {
                Nui::Console::error("ProcessStore::spawn callback did not return a uuid");
                if (val.hasOwnProperty("error"))
                    Nui::Console::error(val["error"].as<std::string>());
                return onOpen(false);
            }
            std::string uuid = val["uuid"].as<std::string>();
            impl_->processId = uuid;

            onOpen(true);
        },
        obj);
}

void ExecutingTerminalEngine::dispose()
{
    Nui::RpcClient::callWithBackChannel(
        "ProcessStore::exit",
        [](Nui::val) {
            // TODO: handle error
        },
        impl_->processId);

    impl_->stdoutReceiver.reset();
    impl_->stderrReceiver.reset();
}

void ExecutingTerminalEngine::resize(int cols, int rows)
{
    Nui::RpcClient::callWithBackChannel(
        "ProcessStore::ptyResize",
        [](Nui::val) {
            // TODO: handle error
        },
        impl_->processId,
        cols,
        rows);
}

void ExecutingTerminalEngine::write(std::string const& data)
{
    Nui::RpcClient::callWithBackChannel(
        "ProcessStore::write", [](Nui::val) {}, impl_->processId, Nui::val::global("btoa")(data).as<std::string>());
}

void ExecutingTerminalEngine::setStdoutHandler(std::function<void(std::string const&)> handler)
{
    impl_->stdoutHandler = std::move(handler);
}
void ExecutingTerminalEngine::setStderrHandler(std::function<void(std::string const&)> handler)
{
    impl_->stderrHandler = std::move(handler);
}