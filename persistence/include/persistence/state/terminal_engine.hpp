#pragma once

#include <persistence/state_core.hpp>
#include <persistence/state/ssh_options.hpp>
#include <persistence/state/ssh_session_options.hpp>
#include <persistence/state/termios.hpp>
#include <persistence/reference.hpp>
#include <nlohmann/json.hpp>
#include <persistence/state/terminal_options.hpp>

#include <unordered_map>
#include <optional>
#include <vector>
#include <string>
#include <filesystem>
#include <variant>

namespace Persistence
{
    enum class TerminalEngineType
    {
        shell,
        cmd, // TODO
        powershell, // TODO
        ssh
    };

    struct BaseTerminalEngine
    {
        bool isPty{true};

        BaseTerminalEngine() = default;
        virtual ~BaseTerminalEngine() = default;
        BaseTerminalEngine(BaseTerminalEngine const&) = default;
        BaseTerminalEngine(BaseTerminalEngine&&) = default;
        BaseTerminalEngine& operator=(BaseTerminalEngine const&) = default;
        BaseTerminalEngine& operator=(BaseTerminalEngine&&) = default;
    };
    void to_json(nlohmann::json& j, BaseTerminalEngine const& engine);
    void from_json(nlohmann::json const& j, BaseTerminalEngine& engine);

    struct ExecutingTerminalEngine : BaseTerminalEngine
    {
        std::string command{};
        std::optional<std::vector<std::string>> arguments{std::nullopt};
        std::optional<std::unordered_map<std::string, std::string>> environment{std::nullopt};
        std::optional<int> exitTimeoutSeconds{std::nullopt};
        std::optional<bool> cleanEnvironment{std::nullopt};
    };
    void to_json(nlohmann::json& j, ExecutingTerminalEngine const& engine);
    void from_json(nlohmann::json const& j, ExecutingTerminalEngine& engine);

    struct SshTerminalEngine : BaseTerminalEngine
    {
        Referenceable<SshSessionOptions> sshSessionOptions{};
    };
    void to_json(nlohmann::json& j, SshTerminalEngine const& engine);
    void from_json(nlohmann::json const& j, SshTerminalEngine& engine);

    struct TerminalEngine
    {
        std::string type{};
        std::optional<bool> startupSession{};
        Referenceable<TerminalOptions> terminalOptions{};
        Referenceable<Termios> termios{};
        std::variant<std::monostate, ExecutingTerminalEngine, SshTerminalEngine> engine{};
        std::optional<nlohmann::json> layout{};
    };
    void to_json(nlohmann::json& j, TerminalEngine const& engine);
    void from_json(nlohmann::json const& j, TerminalEngine& engine);

    ExecutingTerminalEngine defaultMsys2TerminalEngine();
    ExecutingTerminalEngine defaultBashTerminalEngine();
}