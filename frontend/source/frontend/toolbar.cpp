#include <frontend/toolbar.hpp>
#include <frontend/tailwind.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct Toolbar::Implementation
{};

Toolbar::Toolbar()
    : impl_(std::make_unique<Implementation>())
{}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(Toolbar);

Nui::ElementRenderer Toolbar::operator()()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    return div{class_ = classes(defaultBgText, "[grid-area:Toolbar] p-4")}("Toolbar");
}