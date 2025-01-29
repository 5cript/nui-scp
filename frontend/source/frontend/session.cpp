#include <persistence/state/terminal_engine.hpp>
#include <frontend/session.hpp>
#include <frontend/terminal/terminal.hpp>
#include <frontend/terminal/executing_engine.hpp>
#include <frontend/terminal/user_control_engine.hpp>
#include <frontend/terminal/ssh_engine.hpp>
#include <frontend/classes.hpp>
#include <nui-file-explorer/file_grid.hpp>
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

using namespace Nui;
using namespace Nui::Elements;
using namespace Nui::Attributes;

struct Session::Implementation
{
    Persistence::StateHolder* stateHolder;
    Persistence::TerminalEngine engine;
    Nui::Observed<Persistence::TerminalOptions> options;
    Nui::Observed<std::unique_ptr<Terminal>> terminal;
    std::string initialName;
    std::shared_ptr<Nui::Observed<std::string>> tabTitle;
    std::string id;
    std::function<void(Session const& self)> closeSelf;
    Nui::Observed<bool> isVisible;
    std::shared_ptr<Nui::Dom::Element> terminalElement;
    NuiFileExplorer::FileGrid fileGrid;
    std::shared_ptr<Nui::Dom::Element> fileExplorer;

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
        , initialName{std::move(initialName)}
        , tabTitle{std::make_shared<Nui::Observed<std::string>>(this->initialName)}
        , id{Nui::val::global("generateId")().as<std::string>()}
        , closeSelf{std::move(closeSelf)}
        , isVisible{visible}
        , terminalElement{}
        , fileGrid{}
        , fileExplorer{}
    {
        fileGrid.items({
            NuiFileExplorer::FileGrid::Item{
                .path = "G",
                .icon = "nui://app.example/icons/folder_main.png",
            },
            NuiFileExplorer::FileGrid::Item{
                .path = "A",
                .icon = "nui://app.example/icons/folder_main.png",
            },
            NuiFileExplorer::FileGrid::Item{
                .path = "world",
                .icon = "nui://app.example/icons/folder_main.png",
            },
            NuiFileExplorer::FileGrid::Item{
                .path = "world2",
                .icon = "nui://app.example/icons/folder_main.png",
            },
            NuiFileExplorer::FileGrid::Item{
                .path = "world77",
                .icon = "nui://app.example/icons/folder_main.png",
            },
            NuiFileExplorer::FileGrid::Item{
                .path = "world4",
                .icon = "nui://app.example/icons/folder_main.png",
            },
            NuiFileExplorer::FileGrid::Item{
                .path = "world5",
                .icon = "nui://app.example/icons/folder_main.png",
            },
            NuiFileExplorer::FileGrid::Item{
                .path = "world6",
                .icon = "nui://app.example/icons/folder_main.png",
            },
        });
    }
};

auto Session::makeTerminalElement() -> Nui::ElementRenderer
{
    using Nui::Elements::div; // because of the global div.

    // clang-format off
    return div{}(
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
    );
    // clang-format on
}

auto Session::makeFileExplorerElement() -> Nui::ElementRenderer
{
    using Nui::Elements::div; // because of the global div.
    using namespace Nui::Attributes;

    // clang-format off
    return div{
        style = "width: 100%; height: auto; display: block",
    }(
        impl_->fileGrid()
    );
    // clang-format on
}

Session::Session(
    Persistence::StateHolder* stateHolder,
    Persistence::TerminalEngine engine,
    std::string initialName,
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
                    [this](std::string const& cmdline) {
                        Log::info("Tab title changed: {}", cmdline);
                        *impl_->tabTitle = cmdline;
                        Nui::globalEventContext.executeActiveEventsImmediately();
                    },
            }));
    }
    else if (std::holds_alternative<Persistence::SshTerminalEngine>(impl_->engine.engine))
    {
        impl_->terminal = std::make_unique<Terminal>(std::make_unique<SshTerminalEngine>(SshTerminalEngine::Settings{
            .engineOptions = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine),
            .onExit =
                [this]() {
                    // TODO: this is harsh, when the connection dropped unexpectedly, so keep the terminal open and
                    // print a disconnect warning.
                    if (impl_->closeSelf)
                        impl_->closeSelf(*this);
                },
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
        *impl_->tabTitle = user + "@" + host + ":" + std::to_string(port);
    }
    else
    {
        Log::error("Unsupported terminal engine type");
        return;
    }
    Nui::globalEventContext.executeActiveEventsImmediately();
}

Session::~Session()
{
    if (impl_->terminalElement)
    {
        Nui::val::global("contentPanelManager").call<void>("removePanel", impl_->id);
    }
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(Session);

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
    else
    {
        impl_->terminal.value()->focus();
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

std::weak_ptr<Nui::Observed<std::string>> Session::tabTitle() const
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
    if (value)
        impl_->terminal.value()->focus();
}

Nui::ElementRenderer Session::operator()(bool visible)
{
    using Nui::Elements::div; // because of the global div.
    Log::info("Session::operator()");

    impl_->isVisible = visible;

    // clang-format off
    return div{
        class_ = observe(impl_->isVisible).generate([this]() {
            return classes("terminal-session", impl_->isVisible.value() ? "terminal-session-visible" : "terminal-session-hidden");
        }),
        style = Style{
            "background-color"_style = observe(impl_->options).generate([this]() -> std::string {
                if (impl_->options->theme && impl_->options->theme->background)
                    return *impl_->options->theme->background;
                return "#202020";
            }),
        },
        !reference.onMaterialize([this](Nui::val element){
            Nui::val::global("contentPanelManager").call<void>("addPanel", element, impl_->id, Nui::bind([this]() -> Nui::val {
                Nui::Console::log("terminal factory content panel manager");
                if (impl_->terminalElement)
                {
                    Log::critical("Terminal element already exists - make sure that the session is not recreated");
                    return Nui::val::undefined();
                }
                impl_->terminalElement = Nui::Dom::makeStandaloneElement(makeTerminalElement());
                return impl_->terminalElement->val();
            }), Nui::bind([this]() -> Nui::val {
                // OpenFileExplorer
                if (impl_->fileExplorer)
                {
                    Log::warn("There is already a file explorer, cannot open another one");
                    return Nui::val::undefined();
                }
                impl_->fileExplorer = Nui::Dom::makeStandaloneElement(makeFileExplorerElement());
                return impl_->fileExplorer->val();
            }), Nui::bind([this]() -> Nui::val {
                // Remove FileExplorer
                if (!impl_->fileExplorer)
                {
                    Log::warn("There is no file explorer to remove");
                    return Nui::val::undefined();
                }
                impl_->fileExplorer.reset();
                return Nui::val::undefined();
            }));
        })
    }();
    // clang-format on
}