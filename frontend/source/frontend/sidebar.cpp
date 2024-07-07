#include <frontend/sidebar.hpp>
#include <frontend/tailwind.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct Sidebar::Implementation
{
    Persistence::StateHolder* stateHolder;

    Implementation(Persistence::StateHolder* stateHolder)
        : stateHolder{stateHolder}
    {}
};

Sidebar::Sidebar(Persistence::StateHolder* stateHolder)
    : impl_(std::make_unique<Implementation>(stateHolder))
{}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(Sidebar);

Nui::ElementRenderer Sidebar::operator()()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    return div{class_ = classes(defaultBgText, "p-4 [grid-area:Sidebar]")}(
        h1{class_ = "text-3xl font-bold"}("Sidebar"), p{class_ = "text-lg"}("This is the sidebar."));
}