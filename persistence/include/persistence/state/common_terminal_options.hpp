#pragma once

#include <persistence/state_core.hpp>

#include <optional>
#include <string>
#include <vector>

namespace Persistence
{
    struct TerminalTheme
    {
        std::optional<std::string> background{"#303030"};
        std::optional<std::string> black{std::nullopt};
        std::optional<std::string> blue{std::nullopt};
        std::optional<std::string> brightBlack{std::nullopt};
        std::optional<std::string> brightBlue{std::nullopt};
        std::optional<std::string> brightCyan{std::nullopt};
        std::optional<std::string> brightGreen{std::nullopt};
        std::optional<std::string> brightMagenta{std::nullopt};
        std::optional<std::string> brightRed{std::nullopt};
        std::optional<std::string> brightWhite{std::nullopt};
        std::optional<std::string> brightYellow{std::nullopt};
        std::optional<std::string> cursor{std::nullopt};
        std::optional<std::string> cursorAccent{std::nullopt};
        std::optional<std::string> cyan{std::nullopt};
        std::optional<std::vector<std::string>> extendedAnsi{std::nullopt};
        std::optional<std::string> foreground{std::nullopt};
        std::optional<std::string> green{std::nullopt};
        std::optional<std::string> magenta{std::nullopt};
        std::optional<std::string> red{std::nullopt};
        std::optional<std::string> selectionBackground{std::nullopt};
        std::optional<std::string> selectionForeground{std::nullopt};
        std::optional<std::string> selectionInactiveBackground{std::nullopt};
        std::optional<std::string> white{"#efefef"};
        std::optional<std::string> yellow{std::nullopt};

        void useDefaultsFrom(TerminalTheme const& other);
    };
    void to_json(nlohmann::json& j, TerminalTheme const& theme);
    void from_json(nlohmann::json const& j, TerminalTheme& theme);

    struct CommonTerminalOptions
    {
        std::optional<std::string> inherits{std::nullopt};
        std::optional<std::string> fontFamily{std::nullopt};
        std::optional<unsigned int> fontSize{std::nullopt};
        std::optional<float> lineHeight{std::nullopt};
        std::optional<bool> cursorBlink{std::nullopt};
        std::optional<std::string> renderer{std::nullopt};
        std::optional<int> letterSpacing{std::nullopt};
        std::optional<TerminalTheme> theme{};

        void useDefaultsFrom(CommonTerminalOptions const& other);
    };
    void to_json(nlohmann::json& j, CommonTerminalOptions const& engine);
    void from_json(nlohmann::json const& j, CommonTerminalOptions& engine);
}