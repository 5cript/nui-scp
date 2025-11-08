#pragma once

#include <shared_data/directory_entry.hpp>

#include <libssh/sftp.h>

namespace SecureShell
{
    using FileInformation = SharedData::DirectoryEntry;
    FileInformation fromSftpAttributes(sftp_attributes attributes);
}