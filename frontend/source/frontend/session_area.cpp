#include <frontend/session_area.hpp>
#include <frontend/tailwind.hpp>
#include <frontend/session.hpp>
#include <log/log.hpp>

#include <ui5/components/tab_container.hpp>

#include <nui/frontend/api/console.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct SessionArea::Implementation
{
    Persistence::StateHolder* stateHolder;
    Nui::Observed<std::vector<Session>> sessions;

    Implementation(Persistence::StateHolder* stateHolder)
        : stateHolder{stateHolder}
    {
        Nui::Console::log("SessionArea::Implementation()");
    }
};

SessionArea::SessionArea(Persistence::StateHolder* stateHolder)
    : impl_{std::make_unique<Implementation>(stateHolder)}
{
    Nui::Console::log("SessionArea::SessionArea()");
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(SessionArea);

void SessionArea::addSession(std::string const& name)
{
    impl_->stateHolder->load([this, name](bool success, Persistence::StateHolder& holder) {
        if (!success)
            return;

        auto const& state = holder.stateCache();
        auto engine = std::find_if(
            begin(state.terminalEngines.value()), end(state.terminalEngines.value()), [name](const auto& engine) {
                if (engine.name == name)
                    return true;
                return false;
            });

        if (engine == end(state.terminalEngines.value()))
        {
            Log::error("No engine found for name: {}", name);
            return;
        }

        auto options = engine->options;
        if (options.inherits)
        {
            Log::debug("Inheriting options from: {}", *options.inherits);
            auto parent = std::find_if(
                begin(state.terminalOptions.value()),
                end(state.terminalOptions.value()),
                [options](const auto& parentOption) {
                    if (parentOption.id == *options.inherits)
                        return true;
                    return false;
                });

            if (parent == end(state.terminalOptions.value()))
            {
                Log::warn("Parent option not found for id: {}", *options.inherits);
                return;
            }

            options.useDefaultsFrom(parent->value);
        }

        const auto termios = [&]() {
            if (!engine->termiosInherit.empty())
            {
                Log::debug("Inheriting termios from: {}", engine->termiosInherit);
                auto parent = std::find_if(
                    begin(state.termios.value()), end(state.termios.value()), [engine](const auto& parentTermios) {
                        if (parentTermios.id == engine->termiosInherit)
                            return true;
                        return false;
                    });

                if (parent == end(state.termios.value()))
                {
                    Log::warn("Parent termios not found for id: {}", engine->termiosInherit);
                    return Persistence::Termios{};
                }

                return parent->value;
            }
            return Persistence::Termios{};
        }();

        Log::info("Adding session: {}", name);
        impl_->sessions.emplace_back(impl_->stateHolder, *engine, termios, options);
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
        class_ = classes(defaultBgText, "session-area", "[grid-area:SessionArea]")
    }(
        ui5::tabcontainer{
            style = "height: calc(100% - 10px);display: block",
            class_ = "session-area-tabs",
            "fixed"_prop = true,
        }(
            range(impl_->sessions),
            [](long long, auto& session) {
                return ui5::tab{"text"_prop = "asdf"}(
                    session()
                );
            }
        )
    );
    // clang-format on
}