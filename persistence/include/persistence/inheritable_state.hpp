#pragma once

#include <nlohmann/json.hpp>

namespace Persistence
{
    template <typename T>
    struct InheritableState : T
    {
        std::string id;
        using T::T;
    };
    template <typename T>
    void from_json(nlohmann::json const& j, InheritableState<T>& state)
    {
        j.at("id").get_to(state.id);
        from_json(j, static_cast<T&>(state));
    }
    template <typename T>
    void to_json(nlohmann::json& j, InheritableState<T> const& state)
    {
        j["id"] = state.id;
        to_json(j, static_cast<T const&>(state));
    }
}