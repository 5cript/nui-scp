#pragma once

#include <persistence/state_core.hpp>
#include <nlohmann/json.hpp>
#include <persistence/state/common_terminal_options.hpp>

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
        ssh // TODO
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
        ExecutingTerminalEngine() = default;
        ~ExecutingTerminalEngine() = default;
        ExecutingTerminalEngine(ExecutingTerminalEngine const&) = default;
        ExecutingTerminalEngine(ExecutingTerminalEngine&&) = default;
        ExecutingTerminalEngine& operator=(ExecutingTerminalEngine const&) = default;
        ExecutingTerminalEngine& operator=(ExecutingTerminalEngine&&) = default;

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
        std::string host{};
        std::optional<int> port{std::nullopt};
        std::optional<std::string> bindAddr{std::nullopt};
        // optional => use current logged in user
        std::optional<std::string> user{std::nullopt};
        // TODO: KeePassXC integration or enforce agent usage
        // std::string password{std::nullopt};
        std::optional<std::string> sshKey{std::nullopt};
        // Default: bash
        std::optional<std::string> shell{std::nullopt};
        std::optional<std::filesystem::path> sshDirectory{std::nullopt};
        std::optional<std::filesystem::path> knownHostsFile{std::nullopt};
        std::optional<bool> tryAgentForAuthentication{true};
        std::optional<std::string> logVerbosity{std::nullopt};
        std::optional<std::string> keyExchangeAlgorithms{std::nullopt};
        std::optional<std::string> compressionClientToServer{std::nullopt};
        std::optional<std::string> compressionServerToClient{std::nullopt};
        std::optional<int> compressionLevel{std::nullopt};
        std::optional<bool> strictHostKeyCheck{std::nullopt};
        std::optional<std::string> proxyCommand{std::nullopt};
        std::optional<std::string> gssapiServerIdentity{std::nullopt};
        std::optional<std::string> gssapiClientIdentity{std::nullopt};
        std::optional<bool> gssapiDelegateCredentials{std::nullopt};
        std::optional<bool> noDelay{std::nullopt};
        std::optional<bool> bypassConfig{std::nullopt};
        std::optional<bool> usePublicKeyAutoAuth{std::nullopt};
        std::optional<std::string> identityAgent{std::nullopt};
        std::optional<int> connectTimeoutSeconds{std::nullopt};
        std::optional<int> connectTimeoutUSeconds{std::nullopt};
    };
    void to_json(nlohmann::json& j, SshTerminalEngine const& engine);
    void from_json(nlohmann::json const& j, SshTerminalEngine& engine);

    struct TerminalEngine
    {
        std::string type{};
        std::string name{};
        CommonTerminalOptions options{};
        std::string termiosInherit{};
        std::variant<std::monostate, ExecutingTerminalEngine, SshTerminalEngine> engine{};
    };
    void to_json(nlohmann::json& j, TerminalEngine const& engine);
    void from_json(nlohmann::json const& j, TerminalEngine& engine);

    ExecutingTerminalEngine defaultMsys2TerminalEngine();
    ExecutingTerminalEngine defaultBashTerminalEngine();
}