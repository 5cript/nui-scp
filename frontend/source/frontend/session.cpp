#include <frontend/session.hpp>
#include <frontend/terminal/terminal.hpp>
#include <frontend/terminal/executing_engine.hpp>
#include <frontend/classes.hpp>
#include <persistence/state_holder.hpp>
#include <log/log.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/utility/delocalized.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct Session::Implementation
{
    Persistence::StateHolder* stateHolder;
    Persistence::TerminalEngine engine;
    Persistence::Termios termios;
    Nui::Observed<Persistence::TerminalOptions> options;
    Nui::Observed<std::unique_ptr<Terminal>> terminal;
    Nui::Delocalized<int> stable;
    std::string initialName;
    std::string tabTitle;

    Implementation(
        Persistence::StateHolder* stateHolder,
        Persistence::TerminalEngine engine,
        Persistence::Termios termios,
        Persistence::TerminalOptions options,
        std::string initialName)
        : stateHolder{stateHolder}
        , engine{std::move(engine)}
        , termios{std::move(termios)}
        , options{std::move(options)}
        , terminal{}
        , stable{}
        , initialName{std::move(initialName)}
        , tabTitle{this->initialName}
    {}
};

Session::Session(
    Persistence::StateHolder* stateHolder,
    Persistence::TerminalEngine engine,
    Persistence::Termios termios,
    Persistence::TerminalOptions options,
    std::string initialName,
    std::function<void(Session const* session, std::string)> doTabTitleChange)
    : impl_{std::make_unique<Implementation>(
          stateHolder,
          std::move(engine),
          std::move(termios),
          std::move(options),
          std::move(initialName))}
{
    impl_->terminal =
        std::make_unique<Terminal>(std::make_unique<ExecutingTerminalEngine>(ExecutingTerminalEngine::Settings{
            .engineOptions = std::get<Persistence::ExecutingTerminalEngine>(impl_->engine.engine),
            .termios = impl_->termios,
            .onProcessChange =
                [this, doTabTitleChange = std::move(doTabTitleChange)](std::string const& cmdline) {
                    impl_->tabTitle = cmdline;
                    Log::info("Tab title changed: {}", cmdline);
                    doTabTitleChange(this, cmdline);
                },
        }));

    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    impl_->stable.element(
        div{style =
                Style{
                    "height"_style = "100%",
                    "background-color"_style = observe(impl_->options).generate([this]() -> std::string {
                        if (impl_->options->theme && impl_->options->theme->background)
                            return *impl_->options->theme->background;
                        return "#202020";
                    }),
                },
            reference.onMaterialize([this](Nui::val element) {
                Nui::Console::log("Terminal materialized");
                if (impl_->terminal.value())
                    impl_->terminal.value()->open(element, *impl_->options);
            })}());
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(Session);

std::string Session::name() const
{
    return impl_->initialName;
}

std::string Session::tabTitle() const
{
    return impl_->tabTitle;
}

Nui::ElementRenderer Session::operator()()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    Nui::Console::log("Session::operator()");

    // clang-format off
    return div{
        class_ = "terminal-session",
        style = "height: 100%;",
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