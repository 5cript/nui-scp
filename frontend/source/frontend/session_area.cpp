#include <frontend/session_area.hpp>
#include <frontend/tailwind.hpp>
#include <frontend/terminal/terminal.hpp>
#include <frontend/terminal/msys2.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct SessionArea::Implementation
{
    std::unique_ptr<Terminal> terminal;

    Implementation()
        : terminal{std::make_unique<Terminal>(std::make_unique<Msys2Terminal>(Msys2Terminal::Settings{}))}
    {
        Nui::Console::log("SessionArea::Implementation()");
    }
};

SessionArea::SessionArea()
    : impl_{std::make_unique<Implementation>()}
{
    Nui::Console::log("SessionArea::SessionArea()");
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(SessionArea);

Nui::ElementRenderer SessionArea::operator()()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    Nui::Console::log("SessionArea::operator()");

    // clang-format off
    return div{
        class_ = classes(defaultBgText, "[grid-area:SessionArea]")
    }(
        div{
            style = "height: 100%;",
            reference.onMaterialize([this](Nui::val element) {
                Nui::Console::log("materialize");
                impl_->terminal->open(element);
            })
        }()
    );
    // clang-format on
}