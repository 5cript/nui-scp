#pragma once

#include <shared_data/directory_entry.hpp>

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

    virtual void dispose() = 0;
};