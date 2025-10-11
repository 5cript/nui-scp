#include <shared_data/file_operations/operation_completed.hpp>

namespace SharedData
{
    void to_json(nlohmann::json& j, OperationCompletionReason const& operationCompletionReason)
    {
        j = Utility::enumToString<OperationCompletionReason>(operationCompletionReason);
    }
    void from_json(nlohmann::json const& j, OperationCompletionReason& operationCompletionReason)
    {
        operationCompletionReason = Utility::enumFromString<OperationCompletionReason>(j.template get<std::string>());
    }

    void to_json(nlohmann::json& j, OperationCompleted const& operationCompleted)
    {
        j = nlohmann::json{
            {"reason", operationCompleted.reason},
            {"operationId", operationCompleted.operationId},
            {"completionTime",
             static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                      operationCompleted.completionTime.time_since_epoch())
                                      .count())},
        };

        if (operationCompleted.localPath.has_value())
            j["localPath"] = operationCompleted.localPath->generic_string();

        if (operationCompleted.remotePath.has_value())
            j["remotePath"] = operationCompleted.remotePath->generic_string();

        if (operationCompleted.error.has_value())
            j["error"] = *operationCompleted.error;
    }
    void from_json(nlohmann::json const& j, OperationCompleted& operationCompleted)
    {
        j.at("reason").get_to(operationCompleted.reason);
        j.at("operationId").get_to(operationCompleted.operationId);
        operationCompleted.completionTime =
            std::chrono::system_clock::time_point{std::chrono::milliseconds{j.at("completionTime").get<int64_t>()}};

        if (j.contains("localPath"))
            operationCompleted.localPath = j.at("localPath").get<std::filesystem::path>();
        else
            operationCompleted.localPath = std::nullopt;
        if (j.contains("remotePath"))
            operationCompleted.remotePath = j.at("remotePath").get<std::filesystem::path>();
        else
            operationCompleted.remotePath = std::nullopt;
        if (j.contains("error"))
            operationCompleted.error = j.at("error").get<OperationError>();
        else
            operationCompleted.error = std::nullopt;
    }
}