#pragma once

#include <ids/ids.hpp>
#include <shared_data/shared_data.hpp>
#include <shared_data/file_operations/operation_error.hpp>
#include <utility/describe.hpp>
#include <utility/enum_string_convert.hpp>

#include <nlohmann/json.hpp>

#include <string>

namespace SharedData
{
    BOOST_DEFINE_ENUM_CLASS(OperationCompletionReason, Completed, Canceled, Failed)

    void to_json(nlohmann::json& j, OperationCompletionReason const& operationCompletionReason);
    void from_json(nlohmann::json const& j, OperationCompletionReason& operationCompletionReason);

#ifdef __EMSCRIPTEN__
    inline void to_val(Nui::val& v, OperationCompletionReason const& operationCompletionReason)
    {
        v = Nui::val::u8string(Utility::enumToString<OperationCompletionReason>(operationCompletionReason).c_str());
    }
    inline void from_val(Nui::val const& v, OperationCompletionReason& operationCompletionReason)
    {
        operationCompletionReason = Utility::enumFromString<OperationCompletionReason>(v.template as<std::string>());
    }
#endif

    struct OperationCompleted
    {
        OperationCompletionReason reason;
        Ids::OperationId operationId;
        std::chrono::system_clock::time_point completionTime;
        std::optional<std::filesystem::path> localPath{std::nullopt};
        std::optional<std::filesystem::path> remotePath{std::nullopt};
        std::optional<OperationError> error{std::nullopt};
    };

    void to_json(nlohmann::json& j, OperationCompleted const& operationCompleted);
    void from_json(nlohmann::json const& j, OperationCompleted& operationCompleted);

#ifdef __EMSCRIPTEN__
    inline void to_val(Nui::val& v, OperationCompleted const& operationCompleted)
    {
        v = Nui::val::object();

        v.set("reason", Nui::convertToVal(operationCompleted.reason));
        v.set("operationId", Nui::convertToVal(operationCompleted.operationId));

        v.set(
            "completionTime",
            static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                     operationCompleted.completionTime.time_since_epoch())
                                     .count()));

        if (operationCompleted.localPath.has_value())
            v.set("localPath", Nui::val::u8string(operationCompleted.localPath->generic_string().c_str()));
        else
            v.set("localPath", Nui::val::null());

        if (operationCompleted.remotePath.has_value())
            v.set("remotePath", Nui::val::u8string(operationCompleted.remotePath->generic_string().c_str()));
        else
            v.set("remotePath", Nui::val::null());

        if (operationCompleted.error.has_value())
        {
            Nui::val error;
            to_val(error, *operationCompleted.error);
            v.set("error", error);
        }
        else
            v.set("error", Nui::val::null());
    }
    inline void from_val(Nui::val const& v, OperationCompleted& operationCompleted)
    {
        if (!v.hasOwnProperty("reason") || !v.hasOwnProperty("operationId") || !v.hasOwnProperty("completionTime"))
        {
            throw std::runtime_error("Invalid OperationCompleted object");
        }

        Nui::val reason;
        Nui::convertFromVal(v["reason"], operationCompleted.reason);
        Nui::convertFromVal(v["operationId"], operationCompleted.operationId);
        operationCompleted.completionTime = std::chrono::system_clock::time_point{
            std::chrono::milliseconds{v["completionTime"].template as<std::int64_t>()}};

        if (v.hasOwnProperty("localPath") && !v["localPath"].isNull())
            operationCompleted.localPath = std::filesystem::path{v["localPath"].template as<std::string>()};
        if (v.hasOwnProperty("remotePath") && !v["remotePath"].isNull())
            operationCompleted.remotePath = std::filesystem::path{v["remotePath"].template as<std::string>()};
        if (v.hasOwnProperty("error") && !v["error"].isNull())
        {
            operationCompleted.error = OperationError{};
            Nui::convertFromVal(v["error"], operationCompleted.error.value());
        }
    }
#endif
}