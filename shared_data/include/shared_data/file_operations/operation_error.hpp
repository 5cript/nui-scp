#pragma once

#include <shared_data/shared_data.hpp>
#include <shared_data/file_operations/operation_error_type.hpp>
#include <ssh/sftp_error.hpp>

namespace SecureShell
{
    BOOST_DESCRIBE_ENUM(WrapperErrors, None, OwnerNull, SharedPtrDestroyed, ShortWrite, FileNull)
    BOOST_DESCRIBE_STRUCT(SftpError, (), (message, sshError, sftpError, wrapperError))

    void to_json(nlohmann::json& j, WrapperErrors const& wrapperErrors);
    void from_json(nlohmann::json const& j, WrapperErrors& wrapperErrors);

    void to_json(nlohmann::json& j, SftpError const& sftpError);
    void from_json(nlohmann::json const& j, SftpError& sftpError);

#ifdef __EMSCRIPTEN__
    inline void to_val(Nui::val& v, WrapperErrors const& wrapperErrors)
    {
        v = Nui::val::u8string(Utility::enumToString<WrapperErrors>(wrapperErrors).c_str());
    }
    inline void from_val(Nui::val const& v, WrapperErrors& wrapperErrors)
    {
        wrapperErrors = Utility::enumFromString<WrapperErrors>(v.template as<std::string>());
    }
    inline void to_val(Nui::val& v, SftpError const& sftpError)
    {
        v = Nui::val::object();
        v.set("message", Nui::val::u8string(sftpError.message.c_str()));
        v.set("sshError", sftpError.sshError);
        v.set("sftpError", sftpError.sftpError);
        v.set("wrapperError", Nui::val::u8string(Utility::enumToString<WrapperErrors>(sftpError.wrapperError).c_str()));
    }
    inline void from_val(Nui::val const& v, SftpError& sftpError)
    {
        sftpError.message = v["message"].template as<std::string>();
        sftpError.sshError = v["sshError"].template as<int>();
        sftpError.sftpError = v["sftpError"].template as<int>();
        sftpError.wrapperError = Utility::enumFromString<WrapperErrors>(v["wrapperError"].template as<std::string>());
    }
#endif
}

namespace SharedData
{
    struct OperationError
    {
        OperationErrorType type;
        std::optional<SecureShell::SftpError> sftpError = std::nullopt;

        std::string toString() const
        {
            const auto enumString = boost::describe::enum_to_string(type, "INVALID_ENUM_VALUE");
            if (sftpError.has_value())
                return fmt::format("{}: {}", enumString, sftpError->toString());
            return enumString;
        }
    };

    void to_json(nlohmann::json& j, OperationError const& operationError);
    void from_json(nlohmann::json const& j, OperationError& operationError);

#ifdef __EMSCRIPTEN__
    inline void to_val(Nui::val& v, OperationError const& operationError)
    {
        v = Nui::val::object();
        v.set("type", Nui::val::u8string(Utility::enumToString<OperationErrorType>(operationError.type).c_str()));
        if (operationError.sftpError.has_value())
        {
            Nui::val sftpErrorVal;
            to_val(sftpErrorVal, operationError.sftpError.value());
            v.set("sftpError", sftpErrorVal);
        }
        else
        {
            v.set("sftpError", Nui::val::undefined());
        }
    }
    inline void from_val(Nui::val const& v, OperationError& operationError)
    {
        operationError.type = Utility::enumFromString<OperationErrorType>(v["type"].template as<std::string>());
        if (v.hasOwnProperty("sftpError") && !v["sftpError"].isUndefined())
        {
            SecureShell::SftpError sftpError;
            from_val(v["sftpError"], sftpError);
            operationError.sftpError = sftpError;
        }
        else
        {
            operationError.sftpError = std::nullopt;
        }
    }
#endif
}