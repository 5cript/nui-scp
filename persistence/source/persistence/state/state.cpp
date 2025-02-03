#include <persistence/state/state.hpp>
#include <nlohmann/json.hpp>

#include <tuple>

namespace Persistence
{
    void to_json(nlohmann::json& j, State const& state)
    {
        j = nlohmann::json::object();

        j["terminalOptions"] = state.terminalOptions;
        j["terminalEngines"] = state.terminalEngines;
        j["termios"] = state.termios;
        j["sshOptions"] = state.sshOptions;
        j["sshSessionOptions"] = state.sshSessionOptions;
        j["uiOptions"] = state.uiOptions;
        j["logLevel"] = [&]() {
            return Log::levelToString(state.logLevel);
        }();
    }
    void from_json(nlohmann::json const& j, State& state)
    {
        if (j.contains("terminalOptions"))
            j.at("terminalOptions").get_to(state.terminalOptions);

        if (j.contains("terminalEngines"))
            j.at("terminalEngines").get_to(state.terminalEngines);

        if (j.contains("termios"))
            j.at("termios").get_to(state.termios);

        if (j.contains("sshOptions"))
            j.at("sshOptions").get_to(state.sshOptions);

        if (j.contains("sshSessionOptions"))
            j.at("sshSessionOptions").get_to(state.sshSessionOptions);

        if (j.contains("uiOptions"))
            j.at("uiOptions").get_to(state.uiOptions);

        if (j.contains("logLevel"))
            state.logLevel = Log::levelFromString(j.at("logLevel").get<std::string>());
    }
}