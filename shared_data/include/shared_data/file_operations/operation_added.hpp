#pragma once

#include <ids/ids.hpp>
#include <shared_data/file_operations/operation_type.hpp>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

namespace SharedData
{
    struct OperationAdded
    {
        Ids::OperationId operationId;
        OperationType type;
    };

    void to_json(nlohmann::json& j, OperationAdded const& operationAdded);
    void from_json(nlohmann::json const& j, OperationAdded& operationAdded);

#ifdef NUI_FRONTEND
    void to_val(Nui::val& v, OperationAdded const& operationAdded);
    void from_val(Nui::val const& v, OperationAdded& operationAdded);
#endif
}