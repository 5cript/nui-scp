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
}