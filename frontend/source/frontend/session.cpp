#include <frontend/session.hpp>
#include <frontend/terminal/terminal.hpp>
#include <frontend/terminal/msys2.hpp>
#include <persistence/state_holder.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct Session::Implementation
{
    Persistence::StateHolder* stateHolder;
    Persistence::TerminalEngine engine;
    Persistence::CommonTerminalOptions options;
    Nui::Observed<std::unique_ptr<Terminal>> terminal;
    Nui::StableElement stable;

    Implementation(
        Persistence::StateHolder* stateHolder,
        Persistence::TerminalEngine engine,
        Persistence::CommonTerminalOptions options)
        : stateHolder{stateHolder}
        , engine{std::move(engine)}
        , options{std::move(options)}
        , terminal{}
    {}
};

Session::Session(
    Persistence::StateHolder* stateHolder,
    Persistence::TerminalEngine engine,
    Persistence::CommonTerminalOptions options)
    : impl_{std::make_unique<Implementation>(stateHolder, std::move(engine), std::move(options))}
{
    impl_->terminal = std::make_unique<Terminal>(std::make_unique<Msys2Terminal>(Msys2Terminal::Settings{
        .engineOptions = std::get<Persistence::ExecutingTerminalEngine>(impl_->engine.engine)}));
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(Session);

Nui::StableElement& Session::stable()
{
    return impl_->stable;
}

Nui::ElementRenderer Session::operator()()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    Nui::Console::log("Session::operator()");

    // clang-format off
    return stabilize(
        impl_->stable,
        div{
            class_ = "terminal-session",
        }(
            observe(impl_->terminal),
            [this](){
                return div{
                    style = "height: 100%;",
                    reference.onMaterialize([this](Nui::val element) {
                        Nui::Console::log("materialize");
                        if (impl_->terminal.value())
                            impl_->terminal.value()->open(element, impl_->options);
                    })
                }();
            }
        )
    );
    // clang-format on
}