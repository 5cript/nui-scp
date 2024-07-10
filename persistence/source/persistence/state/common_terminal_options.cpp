#include <persistence/state/common_terminal_options.hpp>

namespace Persistence
{
    void to_json(nlohmann::json& j, TerminalTheme const& theme)
    {
        j = nlohmann::json::object();

        TO_JSON_OPTIONAL(j, theme, background);
        TO_JSON_OPTIONAL(j, theme, black);
        TO_JSON_OPTIONAL(j, theme, blue);
        TO_JSON_OPTIONAL(j, theme, brightBlack);
        TO_JSON_OPTIONAL(j, theme, brightBlue);
        TO_JSON_OPTIONAL(j, theme, brightCyan);
        TO_JSON_OPTIONAL(j, theme, brightGreen);
        TO_JSON_OPTIONAL(j, theme, brightMagenta);
        TO_JSON_OPTIONAL(j, theme, brightRed);
        TO_JSON_OPTIONAL(j, theme, brightWhite);
        TO_JSON_OPTIONAL(j, theme, brightYellow);
        TO_JSON_OPTIONAL(j, theme, cursor);
        TO_JSON_OPTIONAL(j, theme, cursorAccent);
        TO_JSON_OPTIONAL(j, theme, cyan);
        TO_JSON_OPTIONAL(j, theme, extendedAnsi);
        TO_JSON_OPTIONAL(j, theme, foreground);
        TO_JSON_OPTIONAL(j, theme, green);
        TO_JSON_OPTIONAL(j, theme, magenta);
        TO_JSON_OPTIONAL(j, theme, red);
        TO_JSON_OPTIONAL(j, theme, selectionBackground);
        TO_JSON_OPTIONAL(j, theme, selectionForeground);
        TO_JSON_OPTIONAL(j, theme, selectionInactiveBackground);
        TO_JSON_OPTIONAL(j, theme, white);
        TO_JSON_OPTIONAL(j, theme, yellow);
    }
    void from_json(nlohmann::json const& j, TerminalTheme& theme)
    {
        theme = {};

        FROM_JSON_OPTIONAL(j, theme, background);
        FROM_JSON_OPTIONAL(j, theme, black);
        FROM_JSON_OPTIONAL(j, theme, blue);
        FROM_JSON_OPTIONAL(j, theme, brightBlack);
        FROM_JSON_OPTIONAL(j, theme, brightBlue);
        FROM_JSON_OPTIONAL(j, theme, brightCyan);
        FROM_JSON_OPTIONAL(j, theme, brightGreen);
        FROM_JSON_OPTIONAL(j, theme, brightMagenta);
        FROM_JSON_OPTIONAL(j, theme, brightRed);
        FROM_JSON_OPTIONAL(j, theme, brightWhite);
        FROM_JSON_OPTIONAL(j, theme, brightYellow);
        FROM_JSON_OPTIONAL(j, theme, cursor);
        FROM_JSON_OPTIONAL(j, theme, cursorAccent);
        FROM_JSON_OPTIONAL(j, theme, cyan);
        FROM_JSON_OPTIONAL(j, theme, extendedAnsi);
        FROM_JSON_OPTIONAL(j, theme, foreground);
        FROM_JSON_OPTIONAL(j, theme, green);
        FROM_JSON_OPTIONAL(j, theme, magenta);
        FROM_JSON_OPTIONAL(j, theme, red);
        FROM_JSON_OPTIONAL(j, theme, selectionBackground);
        FROM_JSON_OPTIONAL(j, theme, selectionForeground);
        FROM_JSON_OPTIONAL(j, theme, selectionInactiveBackground);
        FROM_JSON_OPTIONAL(j, theme, white);
        FROM_JSON_OPTIONAL(j, theme, yellow);
    }

    void TerminalTheme::useDefaultsFrom(TerminalTheme const& other)
    {
        if (!background.has_value())
            background = other.background;
        if (!black.has_value())
            black = other.black;
        if (!blue.has_value())
            blue = other.blue;
        if (!brightBlack.has_value())
            brightBlack = other.brightBlack;
        if (!brightBlue.has_value())
            brightBlue = other.brightBlue;
        if (!brightCyan.has_value())
            brightCyan = other.brightCyan;
        if (!brightGreen.has_value())
            brightGreen = other.brightGreen;
        if (!brightMagenta.has_value())
            brightMagenta = other.brightMagenta;
        if (!brightRed.has_value())
            brightRed = other.brightRed;
        if (!brightWhite.has_value())
            brightWhite = other.brightWhite;
        if (!brightYellow.has_value())
            brightYellow = other.brightYellow;
        if (!cursor.has_value())
            cursor = other.cursor;
        if (!cursorAccent.has_value())
            cursorAccent = other.cursorAccent;
        if (!cyan.has_value())
            cyan = other.cyan;
        if (!extendedAnsi.has_value())
            extendedAnsi = other.extendedAnsi;
        if (!foreground.has_value())
            foreground = other.foreground;
        if (!green.has_value())
            green = other.green;
        if (!magenta.has_value())
            magenta = other.magenta;
        if (!red.has_value())
            red = other.red;
        if (!selectionBackground.has_value())
            selectionBackground = other.selectionBackground;
        if (!selectionForeground.has_value())
            selectionForeground = other.selectionForeground;
        if (!selectionInactiveBackground.has_value())
            selectionInactiveBackground = other.selectionInactiveBackground;
        if (!white.has_value())
            white = other.white;
        if (!yellow.has_value())
            yellow = other.yellow;
    }

    void to_json(nlohmann::json& j, CommonTerminalOptions const& engine)
    {
        j = nlohmann::json::object();

        TO_JSON_OPTIONAL(j, engine, fontFamily);
        TO_JSON_OPTIONAL(j, engine, fontSize);
        TO_JSON_OPTIONAL(j, engine, cursorBlink);
        TO_JSON_OPTIONAL(j, engine, renderer);
        TO_JSON_OPTIONAL(j, engine, theme);
        TO_JSON_OPTIONAL(j, engine, letterSpacing);
        TO_JSON_OPTIONAL(j, engine, inherits);
    }
    void from_json(nlohmann::json const& j, CommonTerminalOptions& engine)
    {
        engine = {};

        FROM_JSON_OPTIONAL(j, engine, inherits);
        FROM_JSON_OPTIONAL(j, engine, fontFamily);
        FROM_JSON_OPTIONAL(j, engine, fontSize);
        FROM_JSON_OPTIONAL(j, engine, cursorBlink);
        FROM_JSON_OPTIONAL(j, engine, renderer);
        FROM_JSON_OPTIONAL(j, engine, letterSpacing);
        FROM_JSON_OPTIONAL(j, engine, theme);
    }

    void CommonTerminalOptions::useDefaultsFrom(CommonTerminalOptions const& other)
    {
        if (!fontFamily.has_value())
            fontFamily = other.fontFamily;
        if (!fontSize.has_value())
            fontSize = other.fontSize;
        if (!cursorBlink.has_value())
            cursorBlink = other.cursorBlink;
        if (!theme.has_value())
            theme = other.theme;
        else
        {
            theme->useDefaultsFrom(other.theme.value());
        }
    }
}