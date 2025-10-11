#pragma once

#include <shared_data/shared_data.hpp>
#include <utility/describe.hpp>
#include <utility/enum_string_convert.hpp>

#include <nlohmann/json.hpp>

#include <string>

namespace SharedData
{
    BOOST_DEFINE_ENUM_CLASS(
        OperationErrorType,
        FileExists,
        FileNotFound,
        OpenFailure,
        FileStreamExpired,
        FileStatFailed,
        SftpError,
        InvalidPath,
        RenameFailure,
        CannotSetFilePermissions,
        FutureTimeout,
        OperationNotPrepared,
        CannotFinalizeDuringRead,
        InvalidOptionsKey,
        TargetFileNotGood,
        CannotWorkCompletedOperation,
        CannotWorkFailedOperation,
        CannotWorkCanceledOperation,
        UnknownWorkState,
        InvalidOperationState,
        OperationNotPossibleOnFileType);

    void to_json(nlohmann::json& j, OperationErrorType const& operationErrorType);
    void from_json(nlohmann::json const& j, OperationErrorType& operationErrorType);

#ifdef __EMSCRIPTEN__
    inline void to_val(Nui::val& v, OperationErrorType const& operationErrorType)
    {
        v = Nui::val::u8string(Utility::enumToString<OperationErrorType>(operationErrorType).c_str());
    }
    inline void from_val(Nui::val const& v, OperationErrorType& operationErrorType)
    {
        operationErrorType = Utility::enumFromString<OperationErrorType>(v.template as<std::string>());
    }
#endif
}