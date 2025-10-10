#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>

namespace SharedData
{
    struct DirectoryEntry
    {
        std::filesystem::path path = "";
        std::filesystem::path longName = "";
        std::uint32_t flags = 0;
        /*
            SSH_FILEXFER_TYPE_REGULAR          1
            SSH_FILEXFER_TYPE_DIRECTORY        2
            SSH_FILEXFER_TYPE_SYMLINK          3
            SSH_FILEXFER_TYPE_SPECIAL          4
            SSH_FILEXFER_TYPE_UNKNOWN          5
            SSH_FILEXFER_TYPE_SOCKET           6
            SSH_FILEXFER_TYPE_CHAR_DEVICE      7
            SSH_FILEXFER_TYPE_BLOCK_DEVICE     8
            SSH_FILEXFER_TYPE_FIFO             9
         */
        std::uint8_t type = 0;
        std::uint64_t size = 0;
        std::uint32_t uid = 0;
        std::uint32_t gid = 0;
        std::string owner = "";
        std::string group = "";
        std::filesystem::perms permissions = std::filesystem::perms::unknown;
        std::uint64_t atime = 0;
        std::uint32_t atimeNsec = 0;
        std::uint64_t createTime = 0;
        std::uint32_t createTimeNsec = 0;
        std::uint64_t mtime = 0;
        std::uint32_t mtimeNsec = 0;
        std::string acl = "";

        bool isDirectory() const
        {
            return type == 2;
        }
        bool isRegularFile() const
        {
            return type == 1;
        }
        bool isSymlink() const
        {
            return type == 3;
        }
        bool isSpecial() const
        {
            return type == 4;
        }
        bool isUnknown() const
        {
            return type == 5;
        }
        bool isSocket() const
        {
            return type == 6;
        }
        bool isCharDevice() const
        {
            return type == 7;
        }
        bool isBlockDevice() const
        {
            return type == 8;
        }
        bool isFifo() const
        {
            return type == 9;
        }
    };

    void to_json(nlohmann::json& j, DirectoryEntry const& entry);
    void from_json(nlohmann::json const& j, DirectoryEntry& entry);
}