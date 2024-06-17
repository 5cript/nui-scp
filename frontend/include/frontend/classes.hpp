#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <sstream>

template <typename T>
struct IsStringLiteralImpl : std::false_type
{};

template <std::size_t N>
struct IsStringLiteralImpl<const char (&)[N]> : std::true_type
{};

template <typename T>
concept IsStringLike = std::same_as<std::decay_t<T>, std::string> || std::same_as<std::decay_t<T>, std::string_view> ||
    std::same_as<T, const char*> || IsStringLiteralImpl<T>::value;

template <typename... T>
requires(IsStringLike<T> && ...)
inline std::string classes(T&&... args)
{
    std::stringstream sstr;
    ((sstr << std::forward<T>(args) << ' '), ...);
    std::string result = sstr.str();
    if (!result.empty())
        result.pop_back();
    return result;
}