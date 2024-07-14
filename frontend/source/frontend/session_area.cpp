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
#include <variant>

struct SessionArea::Implementation
{
    Persistence::StateHolder* stateHolder;
    FrontendEvents* events;
    Nui::Observed<std::vector<std::unique_ptr<Session>>> sessions;

    Implementation(Persistence::StateHolder* stateHolder, FrontendEvents* events)
        : stateHolder{stateHolder}
        , events{events}
        , sessions{}
    {}
};

SessionArea::SessionArea(Persistence::StateHolder* stateHolder, FrontendEvents* events)
    : impl_{std::make_unique<Implementation>(stateHolder, events)}
{
    listen(appEventContext, events->onNewSession, [this](std::string const& name) -> void {
        addSession(name);
    });

    stateHolder->load([this](bool success, Persistence::StateHolder& holder) {
        if (!success)
            return;

        auto const& state = holder.stateCache();

        for (auto const& [name, engine] : state.terminalEngines)
        {
            if (engine.startupSession && engine.startupSession.value())
                addSession(name);
        }
    });
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(SessionArea);

void SessionArea::addSession(std::string const& name)
{
    impl_->stateHolder->load([this, name](bool success, Persistence::StateHolder& holder) {
        if (!success)
            return;

        auto const& state = holder.stateCache();

        auto iter = state.terminalEngines.find(name);
        if (iter == end(state.terminalEngines))
        {
            Log::error("No engine found for name: {}", name);
            return;
        }

        auto [engineKey, engine] = *iter;

        if (!engine.terminalOptions.resolveWith(state.terminalOptions))
            Log::warn("Failed to resolve ref options for engine: {}", name);

        if (!engine.termios.resolveWith(state.termios))
            Log::warn("Failed to resolve ref termios for engine: {}", name);

        if (std::holds_alternative<Persistence::SshTerminalEngine>(engine.engine))
        {
            auto& sshEngine = std::get<Persistence::SshTerminalEngine>(engine.engine);
            auto& sshSessionOptions = sshEngine.sshSessionOptions;

            if (!sshSessionOptions.resolveWith(state.sshSessionOptions))
                Log::warn("Failed to resolve ref ssh session options for engine: {}", name);

            if (!sshSessionOptions->sshOptions.resolveWith(state.sshOptions))
                Log::warn("Failed to resolve ref ssh options for engine: {}", name);
        }

        Log::info("Adding session: {}", name);
        impl_->sessions.emplace_back(std::make_unique<Session>(
            impl_->stateHolder,
            engine,
            name,
            [this](Session const*, std::string const&) {
                {
                    impl_->sessions.modify();
                }
                Nui::globalEventContext.executeActiveEventsImmediately();
            },
            [this](Session const& session) {
                for (auto iter = begin(impl_->sessions); iter != end(impl_->sessions); ++iter)
                {
                    if ((*iter.getWrapped()).get() == &session)
                    {
                        if (session.visible() && impl_->sessions.size() > 1)
                        {
                            impl_->sessions.value()[0]->visible(true);
                        }
                        impl_->sessions.erase(iter);
                        break;
                    }
                }
                Nui::globalEventContext.executeActiveEventsImmediately();
            },
            impl_->sessions.size() == 0));
        Nui::globalEventContext.executeActiveEventsImmediately();
    });
}

Nui::ElementRenderer SessionArea::operator()()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    Log::info("SessionArea::operator()");

    // clang-format off
    return div{
        class_ = "session-area"
    }(
        ui5::tabcontainer{
            style = "width: 100%; display: block",
            class_ = "session-area-tabs",
            "tab-select"_event = [this](Nui::val event){
                const auto index = event["detail"]["tabIndex"].as<int>();
                if (index >= 0 && index < static_cast<int>(impl_->sessions.size()))
                {
                    // Could do some logic, but this is easier when tabs are getting deleted.
                    for (auto const& session : impl_->sessions.value())
                    {
                        if (session->visible())
                            session->visible(false);
                    }
                    impl_->sessions.value()[index]->visible(true);
                }
            },
            "fixed"_prop = true,
        }(
            range(impl_->sessions),
            [](long long, auto& session) -> Nui::ElementRenderer {
                // tabs dont actually reside here:
                return ui5::tab{"text"_prop = session->tabTitle()}();
            }
        ),
        div{
            style = "position: relative; width: 100%; height: calc(100% - 30px); display: block",
            class_ = "session-area-content"
        }(
            range(impl_->sessions),
            [](long long, auto& session) -> Nui::ElementRenderer {
                return session->operator()();
            }
        )
    );
    // clang-format on
}