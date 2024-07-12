#include <frontend/toolbar.hpp>
#include <frontend/classes.hpp>

#include <ui5/components/toolbar.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct Toolbar::Implementation
{
    Persistence::StateHolder* stateHolder;

    Implementation(Persistence::StateHolder* stateHolder)
        : stateHolder{stateHolder}
    {}
};

Toolbar::Toolbar(Persistence::StateHolder* stateHolder)
    : impl_(std::make_unique<Implementation>(stateHolder))
{}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(Toolbar);

Nui::ElementRenderer Toolbar::operator()()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    // clang-format off
    return div{class_ = classes("[grid-area:Toolbar] p-4")}(
        ui5::toolbar{
            "alignContent"_prop = "Start",
            "design"_prop = "Solid",
        }(
            ui5::toolbar_select{
                "change"_event = [](Nui::val event) {
                    Nui::Console::log("Selected: ", event["detail"]["selectedItem"]["text"]);
                }
            }(
                ui5::toolbar_select_option{}("Item 1"),
                ui5::toolbar_select_option{}("Item 2"),
                ui5::toolbar_select_option{}("Item 3")
            ),
            ui5::toolbar_button{
                "text"_prop = "New Session",
                "click"_event = [](Nui::val) {
                    Nui::Console::log("New Session clicked");
                }
            }()
        )
    );
    // clang-format on
}