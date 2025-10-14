#include <frontend/terminal/user_control_engine.hpp>
#include <frontend/nlohmann_compat.hpp>
#include <log/log.hpp>
#include <nui/frontend/api/timer.hpp>

#include <nui/frontend/val.hpp>
#include <nui/rpc.hpp>

using namespace std::string_literals;

struct UserControlEngine::Implementation
{
    UserControlEngine::Settings settings;
    std::string id;

    Nui::RpcClient::AutoUnregister stdoutReceiver;
    Nui::RpcClient::AutoUnregister stderrReceiver;

    std::function<void(std::string const&)> stdoutHandler;
    std::function<void(std::string const&)> stderrHandler;

    Implementation(UserControlEngine::Settings&& settings)
        : settings{std::move(settings)}
        , id{Nui::val::global("generateId")().as<std::string>()}
        , stdoutReceiver{}
        , stderrReceiver{}
        , stdoutHandler{}
        , stderrHandler{}
    {}
};

UserControlEngine::UserControlEngine(Settings settings)
    : impl_{std::make_unique<Implementation>(std::move(settings))}
{}
UserControlEngine::~UserControlEngine()
{
    if (!moveDetector_.wasMoved())
    {
        dispose([]() {});
    }
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(UserControlEngine);

void UserControlEngine::open(std::function<void(bool, std::string const&)> onOpen)
{
    onOpen(true, "Never Fails");
}

void UserControlEngine::dispose(std::function<void()> onDisposeComplete)
{
    impl_->stdoutReceiver.reset();
    impl_->stderrReceiver.reset();
    onDisposeComplete();
}

void UserControlEngine::resize(int, int)
{}

void UserControlEngine::write(std::string const& data)
{
    if (impl_->settings.onInput)
        impl_->settings.onInput(data);
}

void UserControlEngine::setStdoutHandler(std::function<void(std::string const&)> handler)
{
    impl_->stdoutHandler = std::move(handler);
}
void UserControlEngine::setStderrHandler(std::function<void(std::string const&)> handler)
{
    impl_->stderrHandler = std::move(handler);
}