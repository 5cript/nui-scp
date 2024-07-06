#pragma once

#include <persistence/state_core.hpp>
#include <utility/json.hpp>

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
        StateWrap<std::string> command{};
        StateWrap<std::optional<std::vector<std::string>>> arguments{};
        StateWrap<std::optional<std::unordered_map<std::string, std::string>>> environment{};
        StateWrap<std::optional<int>> exitTimeoutSeconds{};
        StateWrap<std::optional<bool>> cleanEnvironment{};
    };
    void to_json(nlohmann::json& j, ExecutingTerminalEngine const& engine);
    void from_json(nlohmann::json const& j, ExecutingTerminalEngine& engine);

    struct SshTerminalEngine
    {
        StateWrap<std::string> host{};
        StateWrap<std::optional<int>> port{22};
        // optional => use current logged in user
        StateWrap<std::optional<std::string>> user{};
        // TODO: KeePassXC integration or enforce agent usage
        // StateWrap<std::string> password{};
        StateWrap<std::optional<std::string>> privateKey{};
        StateWrap<std::optional<std::string>> keyPassphrase{};
        // Default: bash
        StateWrap<std::optional<std::string>> shell{};
        StateWrap<std::optional<std::filesystem::path>> sshDirectory{};
        StateWrap<std::optional<bool>> tryAgentForAuthentication{true};
        // TODO: ?
        // StateWrap<std::unordered_map<std::string, std::string>> environment;
    };
    void to_json(nlohmann::json& j, SshTerminalEngine const& engine);
    void from_json(nlohmann::json const& j, SshTerminalEngine& engine);

    struct TerminalEngine
    {
        StateWrap<std::string> type;
        StateWrap<std::variant<ExecutingTerminalEngine, SshTerminalEngine, std::monostate>> engine;
    };
    void to_json(nlohmann::json& j, TerminalEngine const& engine);
    void from_json(nlohmann::json const& j, TerminalEngine& engine);

    ExecutingTerminalEngine defaultMsys2TerminalEngine();
    ExecutingTerminalEngine defaultBashTerminalEngine();
}