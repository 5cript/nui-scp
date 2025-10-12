#pragma once

#include <ids/ids.hpp>
#include <shared_data/shared_data.hpp>
#include <shared_data/file_operations/operation_type.hpp>
#include <utility/describe.hpp>

#include <nlohmann/json.hpp>

namespace SharedData
{
    struct OperationAdded
    {
        Ids::OperationId operationId;
        OperationType type;
        std::optional<std::uint64_t> totalBytes{std::nullopt};
        std::optional<std::filesystem::path> localPath{};
        std::optional<std::filesystem::path> remotePath{};
    };
    BOOST_DESCRIBE_STRUCT(OperationAdded, (), (operationId, type, totalBytes, localPath, remotePath))

    void to_json(nlohmann::json& j, OperationAdded const& operationAdded);
    void from_json(nlohmann::json const& j, OperationAdded& operationAdded);
}