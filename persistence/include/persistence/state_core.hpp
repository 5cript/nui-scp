#pragma once

#include <nui/core.hpp>
#include <nlohmann/json.hpp>

#include <utility>
#include <optional>

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

namespace Detail
{
    template <typename T>
    struct FromJsonUnwrapped
    {
        static void fromJson(nlohmann::json const& json, T& value, char const* name)
        {
            if (auto it = json.find(name); it != json.end())
                value = it->get<typename T::value_type>();
            else
                value = std::nullopt;
        }
    };

    template <typename T>
    struct ToJsonUnwrapped
    {
        static void toJson(nlohmann::json& json, T const& value, char const* name)
        {
            if (value)
                json[name] = *value;
        }
    };

#ifdef NUI_FRONTEND
    template <typename T>
    struct FromJsonUnwrapped<Persistence::StateWrap<T>>
    {
        static void fromJson(nlohmann::json const& json, Persistence::StateWrap<T>& value, char const* name)
        {
            const auto proxy = value.modify();
            if (auto it = json.find(name); it != json.end())
                Persistence::unwrap(value) = it->get<typename T::value_type>();
        }
    };

    template <typename T>
    struct ToJsonUnwrapped<Persistence::StateWrap<T>>
    {
        static void toJson(nlohmann::json& json, Persistence::StateWrap<T> const& value, char const* name)
        {
            if (auto const& v = Persistence::unwrap(value); v)
                json[name] = *v;
        }
    };
#endif
} // namespace Detail

#define TO_JSON_OPTIONAL_RENAME(JSON, CLASS, MEMBER, JSON_NAME) \
    Detail::ToJsonUnwrapped<std::decay_t<decltype(CLASS.MEMBER)>>::toJson(JSON, CLASS.MEMBER, JSON_NAME)

#define TO_JSON_OPTIONAL(JSON, CLASS, MEMBER) TO_JSON_OPTIONAL_RENAME(JSON, CLASS, MEMBER, #MEMBER)

#define FROM_JSON_OPTIONAL_RENAME(JSON, CLASS, MEMBER, JSON_NAME) \
    Detail::FromJsonUnwrapped<std::decay_t<decltype(CLASS.MEMBER)>>::fromJson(JSON, CLASS.MEMBER, JSON_NAME)

#define FROM_JSON_OPTIONAL(JSON, CLASS, MEMBER) FROM_JSON_OPTIONAL_RENAME(JSON, CLASS, MEMBER, #MEMBER)