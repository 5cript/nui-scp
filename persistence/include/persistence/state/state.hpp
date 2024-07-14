#pragma once

#include <log/level.hpp>
#include <persistence/state_core.hpp>

#include <persistence/state/terminal_engine.hpp>
#include <persistence/state/termios.hpp>
#include <persistence/state/ssh_options.hpp>
#include <persistence/state/ssh_session_options.hpp>

namespace Persistence
{
    struct State
    {
        std::unordered_map<std::string, TerminalOptions> terminalOptions{};
        std::unordered_map<std::string, Termios> termios{};
        std::unordered_map<std::string, SshOptions> sshOptions{};
        std::unordered_map<std::string, TerminalEngine> terminalEngines{};
        std::unordered_map<std::string, SshSessionOptions> sshSessionOptions{};
        Log::Level logLevel{Log::Level::Info};
    };

    void to_json(nlohmann::json& j, State const& state);
    void from_json(nlohmann::json const& j, State& state);
}