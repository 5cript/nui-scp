#include <frontend/session_area.hpp>
#include <frontend/session.hpp>
#include <frontend/classes.hpp>
#include <log/log.hpp>
#include <events/app_event_context.hpp>

#include <ui5/components/tab_container.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

#include <list>

struct SessionArea::Implementation
{
    Persistence::StateHolder* stateHolder;
    FrontendEvents* events;
    Nui::Observed<std::list<Session>> sessions;

    Implementation(Persistence::StateHolder* stateHolder, FrontendEvents* events)
        : stateHolder{stateHolder}
        , events{events}
        , sessions{}
    {
        listen<std::string>(appEventContext, events->onNewSession, [](std::string const& name) -> bool {
            Log::info("New session event: {}", name);
            return true;
        });
    }
};

SessionArea::SessionArea(Persistence::StateHolder* stateHolder, FrontendEvents* events)
    : impl_{std::make_unique<Implementation>(stateHolder, events)}
{}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(SessionArea);

void SessionArea::addSession(std::string const& name)
{
    impl_->stateHolder->load([this, name](bool success, Persistence::StateHolder& holder) {
        if (!success)
            return;

        auto const& state = holder.stateCache();

        auto engine = state.terminalEngines.find(name);
        if (engine == end(state.terminalEngines))
        {
            Log::error("No engine found for name: {}", name);
            return;
        }

        auto options = engine->second.options;
        if (options.hasReference())
        {
            Log::debug("Inheriting options from: {}", options.ref());
            auto parent = state.terminalOptions.find(options.ref());

            if (parent == end(state.terminalOptions))
            {
                Log::warn("Parent option not found for id: {}", options.ref());
                return;
            }

            options.useDefaultsFrom(parent->second);
        }

        const auto termios = [&]() {
            auto termios = engine->second.termios;
            if (!engine->second.termios.hasReference())
            {
                Log::debug("Inheriting termios from: {}", engine->second.termios.ref());
                auto parent = state.termios.find(engine->second.termios.ref());

                if (parent == end(state.termios))
                {
                    Log::warn("Parent termios not found for id: {}", engine->second.termios.ref());
                    return termios;
                }

                termios.useDefaultsFrom(parent->second);
            }
            return termios;
        }();

        Log::info("Adding session: {}", name);
        impl_->sessions.emplace_back(
            impl_->stateHolder,
            engine->second,
            termios.value(),
            options.value(),
            name,
            [this](Session const*, std::string const&) {
                {
                    impl_->sessions.modify();
                }
                Nui::globalEventContext.executeActiveEventsImmediately();
            });
        Nui::globalEventContext.executeActiveEventsImmediately();
    });
}

Nui::ElementRenderer SessionArea::operator()()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    Nui::Console::log("SessionArea::operator()");

    // clang-format off
    return div{
        class_ = classes("session-area", "[grid-area:SessionArea]")
    }(
        ui5::tabcontainer{
            style = "height: calc(100% - 10px);display: block",
            class_ = "session-area-tabs",
            "fixed"_prop = true,
        }(
            range(impl_->sessions),
            [](long long, auto& session) -> Nui::ElementRenderer {
                return ui5::tab{"text"_prop = session.tabTitle()}(
                    session()
                );
            }
        )
    );
    // clang-format on
}