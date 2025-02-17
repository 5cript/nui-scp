#include <ssh/sftp_session.hpp>

namespace SecureShell
{
    SftpSession::SftpSession(Session* owner, sftp_session session)
        : ownerMutex_{}
        , owner_{owner}
        , session_{session}
    {}
    SftpSession::~SftpSession()
    {}

    void SftpSession::close()
    {
        //     if (session_ != nullptr)
        //     sftp_free(session_);
        // session_ = nullptr;
        throw std::runtime_error("Not implemented");
    }

    SftpSession::DirectoryEntry SftpSession::DirectoryEntry::fromSftpAttributes(sftp_attributes attributes)
    {
        DirectoryEntry entry{
            SharedData::DirectoryEntry{
                .path = attributes->name,
                .longName = attributes->longname,
                .flags = attributes->flags,
                .type = attributes->type,
                .size = attributes->size,
                .uid = attributes->uid,
                .gid = attributes->gid,
                .owner = attributes->owner,
                .group = attributes->group,
                .permissions = static_cast<std::filesystem::perms>(attributes->permissions),
                .atime = attributes->atime,
                .atimeNsec = attributes->atime_nseconds,
                .createTime = attributes->createtime,
                .createTimeNsec = attributes->createtime_nseconds,
                .mtime = attributes->mtime,
                .mtimeNsec = attributes->mtime_nseconds,
                .acl =
                    std::string{
                        ssh_string_get_char(attributes->acl),
                        ssh_string_len(attributes->acl),
                    },
            },
        };

        return entry;
    }

    std::future<std::expected<std::vector<SftpSession::DirectoryEntry>, SftpSession::Error>>
    SftpSession::listDirectory(std::filesystem::path const& path)
    {
        // int closeResult = 0;
        // std::vector<DirectoryEntry> entries{};

        // {
        //     std::unique_ptr<sftp_dir_struct, std::function<void(sftp_dir_struct*)>> dir{
        //         sftp_opendir(session_, path.generic_string().c_str()), [&](sftp_dir_struct* dir) {
        //             if (dir != nullptr)
        //             {
        //                 closeResult = sftp_closedir(dir);
        //             }
        //         }};
        //     if (dir == nullptr)
        //     {
        //         return std::unexpected(SftpSession::Error{
        //             .message = ssh_get_error(session_),
        //             .sshError = ssh_get_error_code(session_),
        //             .sftpError = sftp_get_error(session_),
        //         });
        //     }

        //     {
        //         std::unique_ptr<sftp_attributes_struct, decltype(&sftp_attributes_free)> entry{
        //             sftp_readdir(session_, dir.get()), sftp_attributes_free};

        //         for (; entry != nullptr; entry.reset(sftp_readdir(session_, dir.get())))
        //         {
        //             entries.push_back(DirectoryEntry::fromSftpAttributes(entry.get()));
        //         }
        //     }

        //     if (!sftp_dir_eof(dir.get()))
        //     {
        //         return std::unexpected(SftpSession::Error{
        //             .message = ssh_get_error(session_),
        //             .sshError = ssh_get_error_code(session_),
        //             .sftpError = sftp_get_error(session_),
        //         });
        //     }
        // }
        // if (closeResult != SSH_OK)
        // {
        //     return std::unexpected(SftpSession::Error{
        //         .message = ssh_get_error(session_),
        //         .sshError = closeResult,
        //         .sftpError = sftp_get_error(session_),
        //     });
        // }

        // return entries;

        throw std::runtime_error("Not implemented");
    }
    std::future<std::optional<SftpSession::Error>> SftpSession::createDirectory(std::filesystem::path const& path)
    {
        // auto result = sftp_mkdir(session_, path.generic_string().c_str(), S_IRWXU);
        // if (result != SSH_OK)
        // {
        //     return SftpSession::Error{
        //         .message = ssh_get_error(session_),
        //         .sshError = result,
        //         .sftpError = sftp_get_error(session_),
        //     };
        // }
        // return std::nullopt;
        throw std::runtime_error("Not implemented");
    }
}