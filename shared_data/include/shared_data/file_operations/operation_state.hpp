#pragma once

#include <shared_data/shared_data.hpp>
#include <utility/enum_string_convert.hpp>
#include <utility/describe.hpp>

namespace SharedData
{
    BOOST_DEFINE_ENUM_CLASS(
        OperationState,
        NotStarted,
        Preparing,
        Prepared,
        Running,
        Finalizing,
        Completed,
        Canceled,
        Failed)

    void to_json(nlohmann::json& j, OperationState const& operationState);
    void from_json(nlohmann::json const& j, OperationState& operationState);

#ifdef __EMSCRIPTEN__
    inline void to_val(Nui::val& v, OperationState const& operationState)
    {
        v = Nui::val::u8string(Utility::enumToString<OperationState>(operationState).c_str());
    }
    inline void from_val(Nui::val const& v, OperationState& operationState)
    {
        operationState = Utility::enumFromString<OperationState>(v.template as<std::string>());
    }
#endif
}