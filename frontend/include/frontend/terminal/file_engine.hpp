#pragma once

#include <shared_data/directory_entry.hpp>
#include <ids/ids.hpp>

#include <filesystem>
#include <vector>
#include <optional>

class FileEngine
{
  public:
    virtual ~FileEngine() = default;
    FileEngine() = default;
    FileEngine(const FileEngine& other) = default;
    FileEngine(FileEngine&& other) noexcept = default;
    FileEngine& operator=(const FileEngine& other) = default;
    FileEngine& operator=(FileEngine&& other) noexcept = default;

    virtual void listDirectory(
        std::filesystem::path const& path,
        std::function<void(std::optional<std::vector<SharedData::DirectoryEntry>> const&)> onComplete) = 0;

    virtual void createDirectory(std::filesystem::path const& path, std::function<void(bool)> onComplete) = 0;
    virtual void createFile(std::filesystem::path const& path, std::function<void(bool)> onComplete) = 0;

    /**hub.registerFunction(
    "SessionManager::sftp::addDownload",
    [this, hub = &hub](
        std::string const& responseId,
        std::string const& sessionIdString,
        std::string const& channelIdString,
        std::string const& newOperationIdString,
        std::string const& sshSessionOptionsKey,
        std::string const& localPath,
        std::string const& remotePath) */

    // TODO:
    // virtual Ids::OperationId
    // addDownload(std::filesystem::path const& remotePath, std::filesystem::path const& localPath) = 0;

    virtual void dispose() = 0;
};