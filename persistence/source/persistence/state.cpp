#include <persistence/state.hpp>
#include <nlohmann/json.hpp>

namespace Persistence
{
    void to_json(nlohmann::json& j, State const& state)
    {
        j = nlohmann::json::object();

        j["terminalOptions"] = unwrap(state.terminalOptions);
        j["terminalEngines"] = unwrap(state.terminalEngines);
    }
    void from_json(nlohmann::json const& j, State& state)
    {
#ifdef NUI_FRONTEND
        auto proxy = state.terminalEngines.modify();
        auto proxy2 = state.terminalOptions.modify();
#endif
        if (j.contains("terminalOptions"))
            state.terminalOptions = j.at("terminalOptions").get<std::vector<InheritableState<CommonTerminalOptions>>>();

        if (j.contains("terminalEngines"))
            state.terminalEngines = j.at("terminalEngines").get<std::vector<TerminalEngine>>();
    }
}