#pragma once

#include <persistence/state_core.hpp>

#include <persistence/state/terminal_engine.hpp>

namespace Persistence
{
    struct State
    {
        StateWrap<std::vector<TerminalEngine>> terminalEngines{};
    };

    void to_json(nlohmann::json& j, State const& state);
    void from_json(nlohmann::json const& j, State& state);
}