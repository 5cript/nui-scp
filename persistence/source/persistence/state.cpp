#include <persistence/state.hpp>
#include <nlohmann/json.hpp>

#include <tuple>

namespace Persistence
{
    void to_json(nlohmann::json& j, State const& state)
    {
        j = nlohmann::json::object();

        j["terminalOptions"] = unwrap(state.terminalOptions);
        j["terminalEngines"] = unwrap(state.terminalEngines);
        j["termios"] = unwrap(state.termios);
    }
    void from_json(nlohmann::json const& j, State& state)
    {
#ifdef NUI_FRONTEND
        auto proxies =
            std::tuple{state.terminalEngines.modify(), state.terminalOptions.modify(), state.termios.modify()};
#endif
        if (j.contains("terminalOptions"))
            state.terminalOptions = j.at("terminalOptions").get<std::vector<InheritableState<CommonTerminalOptions>>>();

        if (j.contains("terminalEngines"))
            state.terminalEngines = j.at("terminalEngines").get<std::vector<TerminalEngine>>();

        if (j.contains("termios"))
            state.termios = j.at("termios").get<std::vector<InheritableState<Termios>>>();
    }
}