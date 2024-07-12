#pragma once

#include <persistence/state_core.hpp>
#include <persistence/state/terminal_theme.hpp>

#include <optional>
#include <string>
#include <vector>

namespace Persistence
{
    struct TerminalOptions
    {
        std::optional<std::string> fontFamily{std::nullopt};
        std::optional<unsigned int> fontSize{std::nullopt};
        std::optional<float> lineHeight{std::nullopt};
        std::optional<bool> cursorBlink{std::nullopt};
        std::optional<std::string> renderer{std::nullopt};
        std::optional<int> letterSpacing{std::nullopt};
        std::optional<TerminalTheme> theme{};

        void useDefaultsFrom(TerminalOptions const& other);
    };
    void to_json(nlohmann::json& j, TerminalOptions const& engine);
    void from_json(nlohmann::json const& j, TerminalOptions& engine);
}