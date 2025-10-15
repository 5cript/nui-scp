#pragma once

#include <ids/ids.hpp>
#include <shared_data/shared_data.hpp>
#include <utility/describe.hpp>

#include <nlohmann/json.hpp>

#include <cstdint>

namespace SharedData
{
    struct ScanProgress
    {
        Ids::OperationId operationId;
        std::uint64_t totalBytes;
        std::uint64_t currentIndex;
        std::uint64_t totalScanned;
    };
    BOOST_DESCRIBE_STRUCT(ScanProgress, (), (operationId, totalBytes, currentIndex, totalScanned))
}