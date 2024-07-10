#pragma once

#include <nlohmann/json.hpp>

namespace Persistence
{
    template <typename T>
    struct InheritableState
    {
        std::string id;
        T value;
    };
    template <typename T>
    void from_json(nlohmann::json const& j, InheritableState<T>& state)
    {
        from_json(j, state.value);
        j.at("id").get_to(state.id);
    }
    template <typename T>
    void to_json(nlohmann::json& j, InheritableState<T> const& state)
    {
        to_json(j, state.value);
        j["id"] = state.id;
    }
}