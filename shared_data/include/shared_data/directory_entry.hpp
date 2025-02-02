#pragma once

#include <nlohmann/json.hpp>
#include <boost/describe.hpp>

#include <filesystem>
#include <string>

namespace SharedData
{
    struct DirectoryEntry
    {
        std::filesystem::path path;
        std::filesystem::path longName;
        std::uint32_t flags;
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
        std::uint8_t type;
        std::uint64_t size;
        std::uint32_t uid;
        std::uint32_t gid;
        std::string owner;
        std::string group;
        std::filesystem::perms permissions;
        std::uint64_t atime;
        std::uint32_t atimeNsec;
        std::uint64_t createTime;
        std::uint32_t createTimeNsec;
        std::uint64_t mtime;
        std::uint32_t mtimeNsec;
        std::string acl;
    };

    void to_json(nlohmann::json& j, DirectoryEntry const& entry);
    void from_json(nlohmann::json const& j, DirectoryEntry& entry);
}