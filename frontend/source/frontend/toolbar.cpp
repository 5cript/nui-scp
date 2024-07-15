#include <frontend/toolbar.hpp>
#include <frontend/classes.hpp>
#include <log/log.hpp>
#include <events/app_event_context.hpp>

#include <ui5/components/toolbar.hpp>
#include <ui5/components/toolbar_button.hpp>
#include <ui5/components/toolbar_select.hpp>
#include <ui5/components/toolbar_select_option.hpp>
#include <ui5/components/select.hpp>

#include <nui/event_system/observed_value.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct Toolbar::Implementation
{
    Persistence::StateHolder* stateHolder;
    FrontendEvents* events;
    Nui::Observed<std::vector<std::string>> terminalEngines;

    Implementation(Persistence::StateHolder* stateHolder, FrontendEvents* events)
        : stateHolder{stateHolder}
        , events{events}
        , terminalEngines{}
    {
        Log::info("Toolbar::Implementation()");
    }

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
            Log::info("Updating terminal engines list.");
            auto proxy = terminalEngines.modify();
            terminalEngines = std::move(engines);
            events->onNewSession.value() = terminalEngines.value().front();
        }
        Nui::globalEventContext.executeActiveEventsImmediately();
    });
}

Toolbar::Toolbar(Persistence::StateHolder* stateHolder, FrontendEvents* events)
    : impl_(std::make_unique<Implementation>(stateHolder, events))
{
    Log::info("Toolbar::Toolbar");
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
    return div{class_ = "toolbar"}(
        ui5::toolbar{
            "alignContent"_prop = "Start",
            "design"_prop = "Solid",
        }(
            ui5::toolbar_select{
                "change"_event = [this](Nui::val event) {
                    impl_->events->onNewSession.assignWithoutUpdate(
                        event["detail"]["selectedOption"]["textContent"].as<std::string>());
                }
            }(
                range(impl_->terminalEngines),
                [](long long, auto& engine) -> Nui::ElementRenderer {
                    return ui5::toolbar_select_option{}(engine);
                }
            ),
            ui5::toolbar_button{
                "text"_prop = "New Session",
                "click"_event = [this](Nui::val) {
                    impl_->events->onNewSession.modifyNow();
                }
            }()
        )
    );
    // clang-format on
}