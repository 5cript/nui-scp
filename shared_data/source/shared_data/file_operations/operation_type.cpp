#include <shared_data/file_operations/operation_type.hpp>

namespace SharedData
{
    void to_json(nlohmann::json& j, OperationType const& operationType)
    {
        switch (operationType)
        {
            case OperationType::Download:
                j = "Download";
                break;
            case OperationType::Upload:
                j = "Upload";
                break;
            case OperationType::Rename:
                j = "Rename";
                break;
            case OperationType::Delete:
                j = "Delete";
                break;
            case OperationType::Scan:
                j = "Scan";
                break;
            case OperationType::BulkDownload:
                j = "BulkDownload";
                break;
        }
    }
    void from_json(nlohmann::json const& j, OperationType& operationType)
    {
        auto const& str = j.get<std::string>();
        if (str == "Download")
            operationType = OperationType::Download;
        else if (str == "Upload")
            operationType = OperationType::Upload;
        else if (str == "Rename")
            operationType = OperationType::Rename;
        else if (str == "Delete")
            operationType = OperationType::Delete;
        else
            throw std::runtime_error("Invalid OperationType string: " + str);
    }
}