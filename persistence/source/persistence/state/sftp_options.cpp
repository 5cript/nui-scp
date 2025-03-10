#include <persistence/state/sftp_options.hpp>

namespace Persistence
{
    void TransferOptions::useDefaultsFrom(TransferOptions const& other)
    {
        if (!tempFileSuffix)
            tempFileSuffix = other.tempFileSuffix;
        if (!mayOverwrite)
            mayOverwrite = other.mayOverwrite;
        if (!reserveSpace)
            reserveSpace = other.reserveSpace;
        if (!tryContinue)
            tryContinue = other.tryContinue;
        if (!inheritPermissions)
            inheritPermissions = other.inheritPermissions;
        if (!doCleanup)
            doCleanup = other.doCleanup;
        if (!customPermissions)
            customPermissions = other.customPermissions;
    }
    void to_json(nlohmann::json& j, TransferOptions const& options)
    {
        if (options.tempFileSuffix)
            j["tempFileSuffix"] = *options.tempFileSuffix;
        if (options.mayOverwrite)
            j["mayOverwrite"] = *options.mayOverwrite;
        if (options.reserveSpace)
            j["reserveSpace"] = *options.reserveSpace;
        if (options.tryContinue)
            j["tryContinue"] = *options.tryContinue;
        if (options.inheritPermissions)
            j["inheritPermissions"] = *options.inheritPermissions;
        if (options.doCleanup)
            j["doCleanup"] = *options.doCleanup;
        if (options.customPermissions)
            j["customPermissions"] = static_cast<unsigned int>(*options.customPermissions);
    }
    void from_json(nlohmann::json const& j, TransferOptions& options)
    {
        if (j.contains("tempFileSuffix"))
            options.tempFileSuffix = j["tempFileSuffix"].template get<std::string>();
        if (j.contains("mayOverwrite"))
            options.mayOverwrite = j["mayOverwrite"].get<bool>();
        if (j.contains("reserveSpace"))
            options.reserveSpace = j["reserveSpace"].get<bool>();
        if (j.contains("tryContinue"))
            options.tryContinue = j["tryContinue"].get<bool>();
        if (j.contains("inheritPermissions"))
            options.inheritPermissions = j["inheritPermissions"].get<bool>();
        if (j.contains("doCleanup"))
            options.doCleanup = j["doCleanup"].get<bool>();
        if (j.contains("customPermissions"))
            options.customPermissions = std::filesystem::perms{j["customPermissions"].template get<unsigned int>()};
    }

    void to_json(nlohmann::json& j, SftpOptions const& options)
    {
        if (options.downloadOptions)
            j["downloadOptions"] = *options.downloadOptions;
        if (options.uploadOptions)
            j["uploadOptions"] = *options.uploadOptions;
    }
    void from_json(nlohmann::json const& j, SftpOptions& options)
    {
        if (j.contains("downloadOptions"))
            options.downloadOptions = j["downloadOptions"].get<TransferOptions>();
        if (j.contains("uploadOptions"))
            options.uploadOptions = j["uploadOptions"].get<TransferOptions>();
    }
    void SftpOptions::useDefaultsFrom(SftpOptions const& other)
    {
        if (!downloadOptions)
            downloadOptions = other.downloadOptions;
        if (!uploadOptions)
            uploadOptions = other.uploadOptions;
    }
}