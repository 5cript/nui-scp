#pragma once

#include <persistence/state_core.hpp>

#include <persistence/state/terminal_engine.hpp>
#include <persistence/state/termios.hpp>
#include <persistence/inheritable_state.hpp>

namespace Persistence
{
    struct State
    {
        StateWrap<std::vector<InheritableState<CommonTerminalOptions>>> terminalOptions{};
        StateWrap<std::vector<InheritableState<Termios>>> termios{};
        StateWrap<std::vector<TerminalEngine>> terminalEngines{};
    };

    void to_json(nlohmann::json& j, State const& state);
    void from_json(nlohmann::json const& j, State& state);
}