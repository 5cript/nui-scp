#include <persistence/state/ui_options.hpp>

namespace Persistence
{
    void to_json(nlohmann::json& j, UiOptions const& options)
    {
        j["fileGridPathBarOnTop"] = options.fileGridPathBarOnTop;
    }
    void from_json(nlohmann::json const& j, UiOptions& options)
    {
        if (j.contains("fileGridPathBarOnTop"))
            options.fileGridPathBarOnTop = j["fileGridPathBarOnTop"].get<bool>();
    }

    void UiOptions::useDefaultsFrom(UiOptions const&)
    {
        /**/
    }
}