#include <shared_data/file_operations/operation_added.hpp>

namespace SharedData
{
    void to_json(nlohmann::json& j, OperationAdded const& operationAdded)
    {
        j = nlohmann::json{
            {"operationId", operationAdded.operationId},
            {"type", operationAdded.type},
        };

        if (operationAdded.totalBytes.has_value())
            j["totalBytes"] = operationAdded.totalBytes.value();
    }
    void from_json(nlohmann::json const& j, OperationAdded& operationAdded)
    {
        j.at("operationId").get_to(operationAdded.operationId);
        j.at("type").get_to(operationAdded.type);
        if (j.contains("totalBytes") && !j.at("totalBytes").is_null())
            j.at("totalBytes").get_to(*operationAdded.totalBytes);
    }
}