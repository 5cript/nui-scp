#include <shared_data/file_operations/operation_error.hpp>

namespace SecureShell
{
    void to_json(nlohmann::json& j, WrapperErrors const& wrapperErrors)
    {
        j = Utility::enumToString<WrapperErrors>(wrapperErrors);
    }
    void from_json(nlohmann::json const& j, WrapperErrors& wrapperErrors)
    {
        wrapperErrors = Utility::enumFromString<WrapperErrors>(j.template get<std::string>());
    }

    void to_json(nlohmann::json& j, SftpError const& sftpError)
    {
        j = nlohmann::json::object();
        j["message"] = sftpError.message;
        j["sshError"] = sftpError.sshError;
        j["sftpError"] = sftpError.sftpError;
        j["wrapperError"] = Utility::enumToString<WrapperErrors>(sftpError.wrapperError);
    }

    void from_json(nlohmann::json const& j, SftpError& sftpError)
    {
        sftpError.message = j.at("message").get<std::string>();
        sftpError.sshError = j.at("sshError").get<int>();
        sftpError.sftpError = j.at("sftpError").get<int>();
        sftpError.wrapperError = Utility::enumFromString<WrapperErrors>(j.at("wrapperError").get<std::string>());
    }
}

namespace SharedData
{
    void to_json(nlohmann::json& j, OperationError const& operationError)
    {
        j = nlohmann::json::object();
        j["type"] = Utility::enumToString<OperationErrorType>(operationError.type);
        if (operationError.sftpError.has_value())
            j["sftpError"] = operationError.sftpError.value();
    }
    void from_json(nlohmann::json const& j, OperationError& operationError)
    {
        operationError.type = Utility::enumFromString<OperationErrorType>(j.at("type").get<std::string>());
        if (j.contains("sftpError"))
            operationError.sftpError = j.at("sftpError").get<SecureShell::SftpError>();
        else
            operationError.sftpError = std::nullopt;
    }
}