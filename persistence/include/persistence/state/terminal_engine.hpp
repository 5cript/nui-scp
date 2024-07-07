#pragma once

#include <persistence/state_core.hpp>
#include <nlohmann/json.hpp>

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
        msys2,
        shell, // TODO
        cmd, // TODO
        powershell, // TODO
        ssh // TODO
    };

    struct ExecutingTerminalEngine
    {
        std::string command{};
        std::optional<std::vector<std::string>> arguments{};
        std::optional<std::unordered_map<std::string, std::string>> environment{};
        std::optional<int> exitTimeoutSeconds{};
        std::optional<bool> cleanEnvironment{};
    };
    void to_json(nlohmann::json& j, ExecutingTerminalEngine const& engine);
    void from_json(nlohmann::json const& j, ExecutingTerminalEngine& engine);

    struct SshTerminalEngine
    {
        std::string host{};
        std::optional<int> port{22};
        // optional => use current logged in user
        std::optional<std::string> user{};
        // TODO: KeePassXC integration or enforce agent usage
        // std::string> password{};
        std::optional<std::string> privateKey{};
        std::optional<std::string> keyPassphrase{};
        // Default: bash
        std::optional<std::string> shell{};
        std::optional<std::filesystem::path> sshDirectory{};
        std::optional<bool> tryAgentForAuthentication{true};
        // TODO: ?
        // StateWrap<std::unordered_map<std::string, std::string>> environment;
    };
    void to_json(nlohmann::json& j, SshTerminalEngine const& engine);
    void from_json(nlohmann::json const& j, SshTerminalEngine& engine);

    struct TerminalEngine
    {
        StateWrap<std::string> type;
        StateWrap<std::variant<std::monostate, ExecutingTerminalEngine, SshTerminalEngine>> engine;
    };
    void to_json(nlohmann::json& j, TerminalEngine const& engine);
    void from_json(nlohmann::json const& j, TerminalEngine& engine);

    ExecutingTerminalEngine defaultMsys2TerminalEngine();
    ExecutingTerminalEngine defaultBashTerminalEngine();
}