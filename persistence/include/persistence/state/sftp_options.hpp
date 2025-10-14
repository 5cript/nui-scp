#pragma once

#include <persistence/state_core.hpp>

#include <string>
#include <optional>
#include <filesystem>

namespace Persistence
{
    struct TransferOptions
    {
        std::optional<std::string> tempFileSuffix{std::nullopt};
        std::optional<bool> mayOverwrite{std::nullopt};
        std::optional<bool> reserveSpace{std::nullopt};
        std::optional<bool> tryContinue{std::nullopt};
        std::optional<bool> inheritPermissions{std::nullopt};
        std::optional<bool> doCleanup{std::nullopt};
        std::optional<std::filesystem::perms> customPermissions{std::nullopt};

        void useDefaultsFrom(TransferOptions const& other);
    };
    void to_json(nlohmann::json& j, TransferOptions const& options);
    void from_json(nlohmann::json const& j, TransferOptions& options);

    struct SftpOptions
    {
        std::optional<TransferOptions> downloadOptions{};
        std::optional<TransferOptions> uploadOptions{};
        std::optional<int> concurrency{std::nullopt}; // How many parallel transfers are allowed?
        std::chrono::seconds operationTimeout{5};

        void useDefaultsFrom(SftpOptions const& other);
    };
    void to_json(nlohmann::json& j, SftpOptions const& options);
    void from_json(nlohmann::json const& j, SftpOptions& options);
}