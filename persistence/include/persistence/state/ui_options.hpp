#pragma once

#include <persistence/state_core.hpp>

namespace Persistence
{
    struct UiOptions
    {
        bool fileGridPathBarOnTop{false};

        void useDefaultsFrom(UiOptions const& other);
    };
    void to_json(nlohmann::json& j, UiOptions const& options);
    void from_json(nlohmann::json const& j, UiOptions& options);
}