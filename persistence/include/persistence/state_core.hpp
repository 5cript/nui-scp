#pragma once

#include <nui/core.hpp>
#include <utility/json.hpp>

#include <utility>

#ifdef NUI_FRONTEND
#    include <nui/frontend/event_system/observed_value.hpp>
#endif

namespace Persistence
{
#ifdef NUI_FRONTEND
    template <typename T>
    using StateWrap = Nui::Observed<T>;

    template <typename T>
    void to_json(nlohmann::json& j, StateWrap<T> const& wrap)
    {
        j = to_json(wrap.value());
    }

    template <typename T>
    void from_json(nlohmann::json const& j, StateWrap<T>& wrap)
    {
        auto proxy = wrap.modify();
        *proxy = j.get<T>();
    }

    template <typename T>
    struct IsStateWrap : std::false_type
    {};

    template <typename T>
    struct IsStateWrap<StateWrap<T>> : std::true_type
    {};
#else
    template <typename T>
    using StateWrap = T;

    template <typename T>
    struct IsStateWrap : std::false_type
    {};
#endif
    template <typename T>
    T& unwrap(StateWrap<T>& wrap)
    {
#ifdef NUI_FRONTEND
        return wrap.value();
#else
        return wrap;
#endif
    }
    template <typename T>
    T const& unwrap(StateWrap<T> const& wrap)
    {
#ifdef NUI_FRONTEND
        return wrap.value();
#else
        return wrap;
#endif
    }

    template <typename T>
    constexpr bool isStateWrap = IsStateWrap<T>::value;
}