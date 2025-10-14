#pragma once

#include <optional>
#include <type_traits>

namespace Utility
{
    template <typename T>
    struct IsOptional : std::false_type
    {};

    template <typename T>
    struct IsOptional<std::optional<T>> : std::true_type
    {};

    template <typename T>
    struct StripOptional : public std::type_identity<T>
    {};

    template <typename T>
    struct StripOptional<std::optional<T>> : public std::type_identity<T>
    {};

    template <typename T>
    using StripOptional_t = typename StripOptional<T>::type;

    template <typename T>
    constexpr auto IsOptional_v = IsOptional<T>::value;

    template <typename T>
    concept OptionalType = IsOptional_v<T>;
}