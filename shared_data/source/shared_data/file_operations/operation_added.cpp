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
        if (operationAdded.localPath.has_value())
            j["localPath"] = operationAdded.localPath->generic_string();
        if (operationAdded.remotePath.has_value())
            j["remotePath"] = operationAdded.remotePath->generic_string();
    }
    void from_json(nlohmann::json const& j, OperationAdded& operationAdded)
    {
        j.at("operationId").get_to(operationAdded.operationId);
        j.at("type").get_to(operationAdded.type);

        if (j.contains("totalBytes") && !j.at("totalBytes").is_null())
            j.at("totalBytes").get_to(*operationAdded.totalBytes);
        else
            operationAdded.totalBytes = std::nullopt;

        if (j.contains("localPath") && !j.at("localPath").is_null())
            operationAdded.localPath = j.at("localPath").get<std::string>();
        else
            operationAdded.localPath = std::nullopt;

        if (j.contains("remotePath") && !j.at("remotePath").is_null())
            operationAdded.remotePath = j.at("remotePath").get<std::string>();
        else
            operationAdded.remotePath = std::nullopt;
    }
}