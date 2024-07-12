#pragma once

#include <persistence/state_core.hpp>
#include <persistence/state/ssh_options.hpp>
#include <persistence/reference.hpp>

#include <string>
#include <optional>
#include <filesystem>

namespace Persistence
{
    struct SshSessionOptions
    {
        std::string host{};
        std::optional<int> port{std::nullopt};
        std::optional<std::string> user{std::nullopt};

        Referenceable<SshOptions> sshOptions{};
        void useDefaultsFrom(SshSessionOptions const& other);
    };
    void to_json(nlohmann::json& j, SshSessionOptions const& options);
    void from_json(nlohmann::json const& j, SshSessionOptions& options);
}