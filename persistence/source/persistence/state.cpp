#include <persistence/state.hpp>
#include <nlohmann/json.hpp>

namespace Persistence
{
    void to_json(nlohmann::json& j, State const& state)
    {
        j = nlohmann::json::object();
        j["terminalEngines"] = unwrap(state.terminalEngines);
    }
    void from_json(nlohmann::json const& j, State& state)
    {
#ifdef NUI_FRONTEND
        auto proxy = state.terminalEngines.modify();
#endif
        j.at("terminalEngines").get_to(unwrap(state.terminalEngines));
    }
}