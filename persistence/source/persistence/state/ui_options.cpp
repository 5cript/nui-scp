#include <persistence/state/ui_options.hpp>

namespace Persistence
{
    void to_json(nlohmann::json& j, UiOptions const& options)
    {
        j["fileGridPathBarOnTop"] = options.fileGridPathBarOnTop;
        j["fileGridExtensionIcons"] = options.fileGridExtensionIcons;
    }
    void from_json(nlohmann::json const& j, UiOptions& options)
    {
        if (j.contains("fileGridPathBarOnTop"))
            options.fileGridPathBarOnTop = j["fileGridPathBarOnTop"].get<bool>();

        if (j.contains("fileGridExtensionIcons"))
        {
            options.fileGridExtensionIcons =
                j["fileGridExtensionIcons"].get<std::unordered_map<std::string, std::string>>();
        }
    }

    void UiOptions::useDefaultsFrom(UiOptions const& other)
    {
        if (fileGridExtensionIcons.empty())
            fileGridExtensionIcons = other.fileGridExtensionIcons;
    }
}