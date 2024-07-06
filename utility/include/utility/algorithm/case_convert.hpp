#pragma once

#include <numeric>
#include <string>
#include <algorithm>

namespace Utility::Algorithm
{
    /**
     * @brief Converts the passed string to upper case by out paramter.
     *
     * @param input The string to convert.
     */
    inline void toUpperCaseInplace(std::string& input)
    {
        std::transform(input.begin(), input.end(), input.begin(), [](char c) {
            return std::toupper(c);
        });
    }

    /**
     * @brief Converts the passed string to upper case and returns it.
     *
     * @param input The string to convert.
     * @return std::string The string in upper case.
     */
    inline std::string toUpperCase(std::string const& input)
    {
        std::string result(input.size(), '\0');
        std::transform(input.begin(), input.end(), result.begin(), [](char c) {
            return std::toupper(c);
        });
        return result;
    }

    /**
     * @brief Converts the passed string to lower case by out paramter.
     *
     * @param input The string to convert.
     */
    inline void toLowerCaseInplace(std::string& input)
    {
        std::transform(input.begin(), input.end(), input.begin(), [](char c) {
            return std::tolower(c);
        });
    }

    /**
     * @brief Converts the passed string to lower case and returns it.
     *
     * @param input The string to convert.
     * @return std::string The string in lower case.
     */
    inline std::string toLowerCase(std::string const& input)
    {
        std::string result(input.size(), '\0');
        std::transform(input.begin(), input.end(), result.begin(), [](char c) {
            return std::tolower(c);
        });
        return result;
    }
}