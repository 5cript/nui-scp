#include <frontend/toolbar.hpp>
#include <frontend/classes.hpp>
#include <log/log.hpp>

#include <ui5/components/toolbar.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct Toolbar::Implementation
{
    Persistence::StateHolder* stateHolder;
    Nui::Observed<std::vector<std::string>> terminalEngines;

    Implementation(Persistence::StateHolder* stateHolder)
        : stateHolder{stateHolder}
    {}

    void updateEnginesList();
};

void Toolbar::Implementation::updateEnginesList()
{
    stateHolder->load([this](bool success, Persistence::StateHolder& holder) {
        if (!success)
            return;

        auto const& state = holder.stateCache();

        std::vector<std::string> engines;
        for (auto const& [name, engine] : state.terminalEngines)
            engines.push_back(name);

        {
            Log::debug("Updating terminal engines list.");
            Nui::Console::log(Nui::convertToVal(engines));
            auto proxy = terminalEngines.modify();
            terminalEngines = std::move(engines);
        }
        Nui::globalEventContext.executeActiveEventsImmediately();
    });
}

Toolbar::Toolbar(Persistence::StateHolder* stateHolder)
    : impl_(std::make_unique<Implementation>(stateHolder))
{
    impl_->updateEnginesList();
}

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
                    //Nui::Console::log("Selected: ", event["detail"]["selectedItem"]["text"]);
                }
            }(
                // range(impl_->terminalEngines),
                // [](long long, auto& engine) -> Nui::ElementRenderer {
                //     return ui5::toolbar_select_option{}(engine);
                // }
                ui5::toolbar_select_option{
                    "selected"_prop = true
                }("X")
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