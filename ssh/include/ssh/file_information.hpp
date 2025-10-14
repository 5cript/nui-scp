#pragma once

#include <shared_data/directory_entry.hpp>

#include <libssh/sftp.h>

namespace SecureShell
{
    struct FileInformation : public SharedData::DirectoryEntry
    {
        static FileInformation fromSftpAttributes(sftp_attributes attributes);
    };
}