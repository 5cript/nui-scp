#pragma once

#include <shared_data/shared_data.hpp>
#include <utility/enum_string_convert.hpp>
#include <utility/describe.hpp>

namespace SharedData
{
    BOOST_DEFINE_ENUM_CLASS(OperationType, Scan, Download, BulkDownload, Upload, Rename, Delete)

    void to_json(nlohmann::json& j, OperationType const& operationType);
    void from_json(nlohmann::json const& j, OperationType& operationType);

#ifdef __EMSCRIPTEN__
    inline void to_val(Nui::val& v, OperationType const& operationType)
    {
        v = Nui::val::u8string(Utility::enumToString<OperationType>(operationType).c_str());
    }
    inline void from_val(Nui::val const& v, OperationType& operationType)
    {
        operationType = Utility::enumFromString<OperationType>(v.template as<std::string>());
    }
#endif
}