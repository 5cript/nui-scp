#include <shared_data/file_operations/download_progress.hpp>

namespace SharedData
{
    void to_json(nlohmann::json& j, DownloadProgress const& downloadProgress)
    {
        j = nlohmann::json{
            {"operationId", downloadProgress.operationId},
            {"min", downloadProgress.min},
            {"max", downloadProgress.max},
            {"current", downloadProgress.current},
        };
    }
    void from_json(nlohmann::json const& j, DownloadProgress& downloadProgress)
    {
        j.at("operationId").get_to(downloadProgress.operationId);
        j.at("min").get_to(downloadProgress.min);
        j.at("max").get_to(downloadProgress.max);
        j.at("current").get_to(downloadProgress.current);
    }
}