#include <shared_data/file_operations/operation_state.hpp>

namespace SharedData
{
    void to_json(nlohmann::json& j, OperationState const& operationState)
    {
        j = Utility::enumToString(operationState);
    }
    void from_json(nlohmann::json const& j, OperationState& operationState)
    {
        operationState = Utility::enumFromString<OperationState>(j.get<std::string>());
    }
}