#pragma once

#include <memory>
#include <type_traits>

namespace Utility
{
    template <typename T>
    struct IsUniquePtr : std::false_type
    {};

    template <typename T>
    struct IsUniquePtr<std::unique_ptr<T>> : std::true_type
    {};

    template <typename T>
    struct StripUniquePtr : public std::type_identity<T>
    {};

    template <typename T>
    struct StripUniquePtr<std::unique_ptr<T>> : public std::type_identity<T>
    {};

    template <typename T>
    using StripUniquePtr_t = typename StripUniquePtr<T>::type;

    template <typename T>
    constexpr auto IsUniquePtr_v = IsUniquePtr<T>::value;

    template <typename T>
    concept UniquePtrType = IsUniquePtr_v<T>;
}