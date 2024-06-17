#include <frontend/terminal/msys2.hpp>

#include <nui/rpc.hpp>

using namespace std::string_literals;

struct Msys2Terminal::Implementation
{
    Msys2Terminal::Settings settings;
    std::string id;

    Nui::RpcClient::AutoUnregister stdoutReceiver;
    Nui::RpcClient::AutoUnregister stderrReceiver;

    std::string processId;

    std::function<void(std::string const&)> stdoutHandler;
    std::function<void(std::string const&)> stderrHandler;

    Implementation(Msys2Terminal::Settings const& settings)
        : settings{settings}
        , id{Nui::val::global("nanoid")().as<std::string>()}
        , stdoutReceiver{}
        , stderrReceiver{}
        , processId{}
        , stdoutHandler{}
        , stderrHandler{}
    {}
};

Msys2Terminal::Msys2Terminal(Settings const& settings)
    : impl_{std::make_unique<Implementation>(settings)}
{}
Msys2Terminal::~Msys2Terminal() = default;

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(Msys2Terminal);

void Msys2Terminal::open(std::function<void(bool)> onOpen)
{
    impl_->stdoutReceiver =
        Nui::RpcClient::autoRegisterFunction("msys2TerminalStdout_" + impl_->id, [this](Nui::val val) {
            if (val.hasOwnProperty("data"))
            {
                const std::string data = Nui::val::global("atob")(val["data"]).as<std::string>();
                if (impl_->stdoutHandler)
                    impl_->stdoutHandler(data);
            }
            else
                Nui::Console::log("msys2TerminalStdout_" + impl_->id + " received an empty message");
        });

    impl_->stderrReceiver =
        Nui::RpcClient::autoRegisterFunction("msys2TerminalStderr_" + impl_->id, [this](Nui::val val) {
            if (val.hasOwnProperty("data"))
            {
                const std::string data = Nui::val::global("atob")(val["data"]).as<std::string>();
                if (impl_->stderrHandler)
                    impl_->stderrHandler(data);
            }
            else
                Nui::Console::error("msys2TerminalStderr_" + impl_->id + " received an empty message");
        });

    Nui::val obj = Nui::val::object();
    Nui::val args = Nui::val::array();

    obj.set("command", "D:\\msys2\\usr\\bin\\bash.exe");
    args.call<void>("push", "--login"s);
    args.call<void>("push", "-i"s);

    // TODO: dont hardcode all of this:
    // obj.set("command", "C:\\windows\\system32\\cmd.exe");
    // args.call<void>("push", "/c"s);
    // args.call<void>("push", "D:\\msys2\\usr\\bin\\bash.exe"s);
    // args.call<void>("push", "--login"s);
    // args.call<void>("push", "-i"s);
    obj.set("arguments", args);
    Nui::val env = Nui::val::object();
    env.set("MSYSTEM", impl_->settings.msystem);
    env.set("CHERE_INVOKING", "1");
    env.set("TERM", "xterm-256color");
    obj.set("environment", env);
    obj.set("defaultExitWaitTimeout", 3);
    obj.set("stdout", "msys2TerminalStdout_" + impl_->id);
    obj.set("stderr", "msys2TerminalStderr_" + impl_->id);
    obj.set("cleanEnvironment", true);
    obj.set("pathExtension", impl_->settings.msys2Directory + "/usr/bin");
    obj.set("isPty", true);

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

void Msys2Terminal::dispose()
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

void Msys2Terminal::resize(int cols, int rows)
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

void Msys2Terminal::write(std::string const& data)
{
    Nui::RpcClient::callWithBackChannel(
        "ProcessStore::write", [](Nui::val) {}, impl_->processId, Nui::val::global("btoa")(data).as<std::string>());
}

void Msys2Terminal::setStdoutHandler(std::function<void(std::string const&)> handler)
{
    impl_->stdoutHandler = std::move(handler);
}
void Msys2Terminal::setStderrHandler(std::function<void(std::string const&)> handler)
{
    impl_->stderrHandler = std::move(handler);
}