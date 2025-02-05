#include <frontend/toolbar.hpp>
#include <frontend/classes.hpp>
#include <log/log.hpp>
#include <events/app_event_context.hpp>
#include <constants/layouts.hpp>

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
    Nui::Observed<std::vector<std::string>> layouts;
    std::string selectedLayout;

    Implementation(Persistence::StateHolder* stateHolder, FrontendEvents* events)
        : stateHolder{stateHolder}
        , events{events}
        , terminalEngines{}
        , layouts{}
        , selectedLayout{}
    {
        Log::info("Toolbar::Implementation()");
    }

    void updateSessionsList();
};

void Toolbar::Implementation::updateSessionsList()
{
    stateHolder->load([this](bool success, Persistence::StateHolder& holder) {
        if (!success)
            return;

        auto const& state = holder.stateCache();

        std::vector<std::string> engines;
        for (auto const& [name, engine] : state.sessions)
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
    impl_->updateSessionsList();
}

void Toolbar::connectLayoutsChanged()
{
    listen(impl_->events->onLayoutsChanged, [this](bool) {
        impl_->stateHolder->load([this](bool success, Persistence::StateHolder&) {
            if (!success)
                return;

            reloadLayouts();
        });
    });
}

void Toolbar::reloadLayouts()
{
    impl_->selectedLayout = "";
    impl_->layouts.value().clear();
    impl_->layouts.value().push_back(std::string{Constants::defaultLayoutName});
    if (const auto iter = impl_->stateHolder->stateCache().sessions.find(impl_->events->onNewSession.value());
        iter != impl_->stateHolder->stateCache().sessions.end())
    {
        if (iter->second.layouts)
        {
            for (auto const& [name, layout] : iter->second.layouts.value())
                impl_->layouts.value().push_back(name);

            if (!impl_->layouts.value().empty())
                impl_->selectedLayout = impl_->layouts.value().front();

            impl_->layouts.modifyNow();
        }
    }
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

                    reloadLayouts();
                }
            }(
                range(impl_->terminalEngines),
                [](long long, auto& engine) -> Nui::ElementRenderer {
                    return ui5::toolbar_select_option{}(engine);
                }
            ),
            ui5::toolbar_select{
                "change"_event = [this](Nui::val event) {
                    Log::info("Selected layout: {}", event["detail"]["selectedOption"]["textContent"].as<std::string>());
                    impl_->selectedLayout = event["detail"]["selectedOption"]["textContent"].as<std::string>();
                },
                !(reference.onMaterialize([this](auto){
                    connectLayoutsChanged();
                }))
            }(
                range(impl_->layouts),
                [](long long, auto& layout) -> Nui::ElementRenderer {
                    return ui5::toolbar_select_option{}(layout);
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

std::string Toolbar::selectedLayout() const
{
    return impl_->selectedLayout;
}