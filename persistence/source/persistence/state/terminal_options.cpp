#include <persistence/state/terminal_options.hpp>

namespace Persistence
{
    void to_json(nlohmann::json& j, TerminalOptions const& engine)
    {
        j = nlohmann::json::object();

        TO_JSON_OPTIONAL(j, engine, fontFamily);
        TO_JSON_OPTIONAL(j, engine, fontSize);
        TO_JSON_OPTIONAL(j, engine, lineHeight);
        TO_JSON_OPTIONAL(j, engine, cursorBlink);
        TO_JSON_OPTIONAL(j, engine, renderer);
        TO_JSON_OPTIONAL(j, engine, theme);
        TO_JSON_OPTIONAL(j, engine, letterSpacing);
    }
    void from_json(nlohmann::json const& j, TerminalOptions& engine)
    {
        engine = {};

        FROM_JSON_OPTIONAL(j, engine, fontFamily);
        FROM_JSON_OPTIONAL(j, engine, fontSize);
        FROM_JSON_OPTIONAL(j, engine, lineHeight);
        FROM_JSON_OPTIONAL(j, engine, cursorBlink);
        FROM_JSON_OPTIONAL(j, engine, renderer);
        FROM_JSON_OPTIONAL(j, engine, letterSpacing);
        FROM_JSON_OPTIONAL(j, engine, theme);
    }

    void TerminalOptions::useDefaultsFrom(TerminalOptions const& other)
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