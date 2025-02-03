#pragma once

#include <persistence/state_core.hpp>

#include <unordered_map>
#include <string>

namespace Persistence
{
    struct UiOptions
    {
        bool fileGridPathBarOnTop{false};
        std::unordered_map<std::string /*extension*/, std::string /*assetPath*/> fileGridExtensionIcons{
            {".cpp", "icons/cpp.png"},     {".hpp", "icons/cpp.png"},       {".cxx", "icons/cpp.png"},
            {".hxx", "icons/cpp.png"},     {".tpp", "icons/cpp.png"},       {".c", "icons/c.png"},
            {".h", "icons/c.png"},         {".cc", "icons/cpp.png"},        {".hh", "icons/cpp.png"},
            {".js", "icons/js.png"},       {".html", "icons/html.png"},     {".css", "icons/css.png"},
            {".java", "icons/java.png"},   {".jar", "icons/jar.png"},       {".cs", "icons/csharp.png"},
            {".log", "icons/log.png"},     {".sqlite", "icons/sqlite.png"}, {".rs", "icons/rust.png"},
            {".swift", "icons/swift.png"}, {".py", "icons/python.png"},
        };

        void useDefaultsFrom(UiOptions const& other);
    };
    void to_json(nlohmann::json& j, UiOptions const& options);
    void from_json(nlohmann::json const& j, UiOptions& options);
}