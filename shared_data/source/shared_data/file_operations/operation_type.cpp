#include <shared_data/file_operations/operation_type.hpp>

namespace SharedData
{
    void to_json(nlohmann::json& j, OperationType const& operationType)
    {
        j = Utility::enumToString<OperationType>(operationType);
    }
    void from_json(nlohmann::json const& j, OperationType& operationType)
    {
        operationType = Utility::enumFromString<OperationType>(j.get<std::string>());
    }
}