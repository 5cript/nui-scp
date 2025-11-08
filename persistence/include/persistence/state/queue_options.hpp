#pragma once

#include <persistence/state_core.hpp>

#include <optional>

namespace Persistence
{
    struct QueueOptions
    {
        std::optional<bool> autoRemoveCompletedOperations{std::nullopt};
        std::optional<bool> startInPausedState{std::nullopt};

        void useDefaultsFrom(QueueOptions const& other);
    };
    void to_json(nlohmann::json& j, QueueOptions const& options);
    void from_json(nlohmann::json const& j, QueueOptions& options);
}