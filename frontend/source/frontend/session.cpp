#include <persistence/state/terminal_engine.hpp>
#include <frontend/session.hpp>
#include <frontend/terminal/terminal.hpp>
#include <frontend/terminal/executing_engine.hpp>
#include <frontend/terminal/user_control_engine.hpp>
#include <frontend/terminal/ssh_engine.hpp>
#include <frontend/classes.hpp>
#include <persistence/state_holder.hpp>
#include <log/log.hpp>

#include <fmt/format.h>
#include <nui/event_system/event_context.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/listen.hpp>
#include <nui/frontend/api/console.hpp>
#include <nui/frontend/utility/delocalized.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

#include <string>

struct Session::Implementation
{
    Persistence::StateHolder* stateHolder;
    Persistence::TerminalEngine engine;
    Nui::Observed<Persistence::TerminalOptions> options;
    Nui::Observed<std::unique_ptr<Terminal>> terminal;
    Nui::Delocalized<int> stable;
    std::string initialName;
    std::string tabTitle;
    std::function<void()> delayedOnOpenTask;
    std::function<void(Session const& self)> closeSelf;
    Nui::Observed<bool> isVisible;

    Implementation(
        Persistence::StateHolder* stateHolder,
        Persistence::TerminalEngine engine,
        std::string initialName,
        std::function<void(Session const& self)> closeSelf,
        bool visible)
        : stateHolder{stateHolder}
        , engine{std::move(engine)}
        , options{this->engine.terminalOptions.value()}
        , terminal{}
        , stable{}
        , initialName{std::move(initialName)}
        , tabTitle{this->initialName}
        , delayedOnOpenTask{}
        , closeSelf{std::move(closeSelf)}
        , isVisible{visible}
    {}
};

Session::Session(
    Persistence::StateHolder* stateHolder,
    Persistence::TerminalEngine engine,
    std::string initialName,
    std::function<void(Session const* session, std::string)> doTabTitleChange,
    std::function<void(Session const& self)> closeSelf,
    bool visible)
    : impl_{std::make_unique<Implementation>(
          stateHolder,
          std::move(engine),
          std::move(initialName),
          std::move(closeSelf),
          visible)}
{
    if (std::holds_alternative<Persistence::ExecutingTerminalEngine>(impl_->engine.engine))
    {
        impl_->terminal =
            std::make_unique<Terminal>(std::make_unique<ExecutingTerminalEngine>(ExecutingTerminalEngine::Settings{
                .engineOptions = std::get<Persistence::ExecutingTerminalEngine>(impl_->engine.engine),
                .termios = impl_->engine.termios.value(),
                .onProcessChange =
                    [this, doTabTitleChange = std::move(doTabTitleChange)](std::string const& cmdline) {
                        impl_->tabTitle = cmdline;
                        Log::info("Tab title changed: {}", cmdline);
                        doTabTitleChange(this, cmdline);
                    },
            }));
    }
    else if (std::holds_alternative<Persistence::SshTerminalEngine>(impl_->engine.engine))
    {
        impl_->terminal = std::make_unique<Terminal>(std::make_unique<SshTerminalEngine>(SshTerminalEngine::Settings{
            .engineOptions = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine),
        }));

        const auto user = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine)
                              .sshSessionOptions.value()
                              .user.value_or("__todo_default__");
        auto host = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine).sshSessionOptions.value().host;
        const auto port =
            std::get<Persistence::SshTerminalEngine>(impl_->engine.engine).sshSessionOptions.value().port.value_or(22);

        // assume ipv6 when finding ':' in host
        if (host.find(":") != std::string::npos)
            host = "[" + host + "]";
        impl_->tabTitle = user + "@" + host + ":" + std::to_string(port);
    }
    else
    {
        Log::error("Unsupported terminal engine type");
        return;
    }
    createTerminalElement();
    Nui::globalEventContext.executeActiveEventsImmediately();
}

Session::~Session() = default;

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(Session);

void Session::createTerminalElement()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    // clang-format off
    impl_->stable.element(
        div{
            style =
                Style{
                    "height"_style = "100%",
                    "background-color"_style = observe(impl_->options).generate([this]() -> std::string {
                        if (impl_->options->theme && impl_->options->theme->background)
                            return *impl_->options->theme->background;
                        return "#202020";
                    }),
                },
        }(
            observe(impl_->terminal),
            [this]() -> Nui::ElementRenderer {
                return div{
                    style = "height: 100%; width: 100%",
                    reference.onMaterialize([this](Nui::val element) {
                        Log::info("Terminal materialized");
                        if (impl_->terminal.value())
                        {
                            impl_->terminal.value()->open(
                                element,
                                *impl_->options,
                                std::bind(&Session::onOpen, this, std::placeholders::_1, std::placeholders::_2));
                        }
                    })
                }();
            }
        )
    );
    // clang-format on
}

void Session::onOpen(bool success, std::string const& info)
{
    if (!success)
    {
        impl_->terminal = std::make_unique<Terminal>(std::make_unique<UserControlEngine>(UserControlEngine::Settings{
            .onInput =
                [this](std::string const& input) {
                    if (input == "\u0003" && impl_->closeSelf)
                        impl_->closeSelf(*this);
                },
        }));
        impl_->terminal.value()->write(
            fmt::format("\033[1;31mFailed to create instance: {}.\r\nPress Ctrl+C do close this tab.\033[00m", info),
            false);
        Nui::globalEventContext.executeActiveEventsImmediately();
    }
}

std::optional<std::string> Session::getProcessIdIfExecutingEngine() const
{
    if (std::holds_alternative<Persistence::ExecutingTerminalEngine>(impl_->engine.engine))
        return static_cast<ExecutingTerminalEngine&>(impl_->terminal.value()->engine()).id();
    return std::nullopt;
}

std::string Session::name() const
{
    return impl_->initialName;
}

std::string Session::tabTitle() const
{
    return impl_->tabTitle;
}

bool Session::visible() const
{
    return impl_->isVisible.value();
}

void Session::visible(bool value)
{
    impl_->isVisible = value;
    Nui::globalEventContext.executeActiveEventsImmediately();
}

Nui::ElementRenderer Session::operator()()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    Log::info("Session::operator()");

    // clang-format off
    return div{
        class_ = observe(impl_->isVisible).generate([this]() {
            return classes("terminal-session", impl_->isVisible.value() ? "terminal-session-visible" : "terminal-session-hidden");
        }),
    }(
        delocalizedSlot(
            0,
            impl_->stable,
            {
                class_ = "terminal-session-content",
                style = "height: 100%;",
            }
        )
    );
    // clang-format on
}