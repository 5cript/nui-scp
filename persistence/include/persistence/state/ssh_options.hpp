#pragma once

#include <persistence/state_core.hpp>

namespace Persistence
{
    struct SshOptions
    {
        std::optional<std::filesystem::path> sshDirectory{std::nullopt};
        std::optional<std::filesystem::path> knownHostsFile{std::nullopt};
        std::optional<bool> tryAgentForAuthentication{true};
        std::optional<bool> usePublicKeyAutoAuth{std::nullopt};
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
        std::optional<std::string> identityAgent{std::nullopt};
        std::optional<int> connectTimeoutSeconds{std::nullopt};
        std::optional<int> connectTimeoutUSeconds{std::nullopt};

        void useDefaultsFrom(SshOptions const& other);
    };
    void to_json(nlohmann::json& j, SshOptions const& options);
    void from_json(nlohmann::json const& j, SshOptions& options);
}