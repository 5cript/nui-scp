#include <persistence/reference.hpp>

namespace Persistence
{
    void to_json(nlohmann::json& obj, Reference const& ref)
    {
        obj["$ref"] = ref.ref;
    }
    void from_json(nlohmann::json const& obj, Reference& ref)
    {
        ref = {};
        ref.ref = obj["$ref"].get<std::string>();
    }
}