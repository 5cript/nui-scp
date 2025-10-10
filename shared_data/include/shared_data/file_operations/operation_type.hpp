#pragma once

#include <shared_data/shared_data.hpp>

namespace SharedData
{
    enum class OperationType
    {
        Scan,
        Download,
        BulkDownload,
        Upload,
        Rename,
        Delete
    };

    void to_json(nlohmann::json& j, OperationType const& operationType);
    void from_json(nlohmann::json const& j, OperationType& operationType);

#ifdef NUI_FRONTEND
    void to_val(Nui::val& v, OperationType const& operationType);
    void from_val(Nui::val const& v, OperationType& operationType);
#endif
}