#include <frontend/session_area.hpp>
#include <frontend/session.hpp>
#include <frontend/classes.hpp>
#include <log/log.hpp>
#include <events/app_event_context.hpp>

#include <ui5/components/tab_container.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/rpc.hpp>

#include <list>
#include <variant>

struct SessionArea::Implementation
{
    Persistence::StateHolder* stateHolder;
    FrontendEvents* events;
    InputDialog* newItemAskDialog;
    ConfirmDialog* confirmDialog;
    Toolbar* toolbar;
    Nui::Observed<std::vector<std::unique_ptr<Session>>> sessions;
    int selected;

    Implementation(
        Persistence::StateHolder* stateHolder,
        FrontendEvents* events,
        InputDialog* newItemAskDialog,
        ConfirmDialog* confirmDialog,
        Toolbar* toolbar)
        : stateHolder{stateHolder}
        , events{events}
        , newItemAskDialog{newItemAskDialog}
        , confirmDialog{confirmDialog}
        , toolbar{toolbar}
        , sessions{}
        , selected{0}
    {}
};

SessionArea::SessionArea(
    Persistence::StateHolder* stateHolder,
    FrontendEvents* events,
    InputDialog* newItemAskDialog,
    ConfirmDialog* confirmDialog,
    Toolbar* toolbar)
    : impl_{std::make_unique<Implementation>(stateHolder, events, newItemAskDialog, confirmDialog, toolbar)}
{
    listen(events->onNewSession, [this](std::string const& name) -> void {
        addSession(name);
    });

    stateHolder->load([this](bool success, Persistence::StateHolder& holder) {
        if (!success)
            return;

        auto const& state = holder.stateCache();

        for (auto const& [name, session] : state.sessions)
        {
            if (session.startupSession && session.startupSession.value())
                addSession(name);
        }
    });

    registerRpc();
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(SessionArea);

void SessionArea::registerRpc()
{
    Nui::RpcClient::registerFunction("SessionArea::processDied", [this](Nui::val val) {
        auto const processId = val["id"].as<std::string>();
        Log::info("Process with id '{}' terminated.", processId);

        std::size_t index = 0;
        for (auto const& session : impl_->sessions.value())
        {
            if (session->getProcessIdIfExecutingEngine().value_or("") == processId)
            {
                break;
            }
            ++index;
        }

        if (index < impl_->sessions.size())
            removeSession(index);
    });
}

void SessionArea::removeSession(std::size_t index)
{
    if (index >= impl_->sessions.size())
    {
        Log::critical("Session index out of bounds: {}", index);
        return;
    }

    Log::info("Removing session: {}", impl_->sessions.value()[index]->name());

    if (impl_->sessions.value()[index]->visible() && impl_->sessions.size() > 1)
        setSelected(std::max(0ull, index - 1ull));

    impl_->sessions.value()[index]->managerShutdown([this, index]() {
        impl_->sessions.erase(impl_->sessions.begin() + index);
        Nui::globalEventContext.executeActiveEventsImmediately();
    });
}

void SessionArea::setSelected(int index)
{
    const auto wasAnythingSelected = [this, index]() {
        if (impl_->selected >= 0 && impl_->selected < static_cast<int>(impl_->sessions.size()))
        {
            impl_->sessions.value()[impl_->selected]->visible(false);
        }
        if (index >= 0 && index < static_cast<int>(impl_->sessions.size()))
        {
            impl_->sessions.value()[index]->visible(true);
            impl_->selected = index;
            return true;
        }
        return false;
    }();

    if (wasAnythingSelected)
        Nui::globalEventContext.executeActiveEventsImmediately();
}

void SessionArea::addSession(std::string const& name)
{
    using namespace std::string_literals;

    impl_->stateHolder->load([this, name](bool success, Persistence::StateHolder& holder) {
        if (!success)
            return;

        auto const& state = holder.stateCache();

        auto iter = state.sessions.find(name);
        if (iter == end(state.sessions))
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

        Log::info("Adding session: {} with layout {}", name, impl_->toolbar->selectedLayout());
        impl_->sessions.emplace_back(std::make_unique<Session>(
            impl_->stateHolder,
            impl_->events,
            engine,
            state.uiOptions,
            name,
            impl_->toolbar->selectedLayout(),
            impl_->newItemAskDialog,
            impl_->confirmDialog,
            [this, index = impl_->sessions.size()]() {
                removeSession(index);
            },
            impl_->sessions.size() == 0));

        if (impl_->selected >= 0 && impl_->selected < static_cast<int>(impl_->sessions.size()))
            impl_->sessions.value()[impl_->selected]->visible(false);
        impl_->selected = impl_->sessions.size() - 1;
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
            "fixed"_prop = true
        }(
            range(impl_->sessions),
            [this](long long i, auto& session) -> Nui::ElementRenderer {
                // tabs dont actually reside here:
                return ui5::tab{
                    "text"_prop = session->tabTitle(),
                    "selected"_prop = i == impl_->selected
                }();
            }
        ),
        div{
            style = "position: relative; width: 100%; height: calc(100% - 30px); display: block",
            class_ = "session-area-content"
        }(
            range(impl_->sessions),
            [this](long long i, auto& session) -> Nui::ElementRenderer {
                return session->operator()(i == impl_->selected);
            }
        )
    );
    // clang-format on
}