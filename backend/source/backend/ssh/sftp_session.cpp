#include <backend/ssh/sftp_session.hpp>
#include <backend/ssh/session.hpp>

#include <sys/stat.h>

SftpSession::SftpSession(sftp_session session)
    : session_(session)
{}

std::expected<SftpSession, SftpSession::Error> makeSftpSession(Session& session)
{
    auto sess = sftp_new(static_cast<ssh::Session&>(session).getCSession());
    if (sess == nullptr)
    {
        return std::unexpected(SftpSession::Error{
            .message = ssh_get_error(static_cast<ssh::Session&>(session).getCSession()),
            .sshError = ssh_get_error_code(static_cast<ssh::Session&>(session).getCSession()),
            .sftpError = 0,
        });
    }

    auto result = sftp_init(sess);
    if (result != SSH_OK)
    {
        sftp_free(sess);
        return std::unexpected(SftpSession::Error{
            .message = ssh_get_error(static_cast<ssh::Session&>(session).getCSession()),
            .sshError = result,
            .sftpError = sftp_get_error(sess),
        });
    }

    return SftpSession{sess};
}

SftpSession::DirectoryEntry SftpSession::DirectoryEntry::fromSftpAttributes(sftp_attributes attributes)
{
    DirectoryEntry entry;
    entry.path = attributes->name;
    entry.longName = attributes->longname;
    entry.flags = attributes->flags;
    entry.type = attributes->type;
    entry.size = attributes->size;
    entry.uid = attributes->uid;
    entry.gid = attributes->gid;
    entry.owner = attributes->owner;
    entry.group = attributes->group;
    entry.permissions = static_cast<std::filesystem::perms>(attributes->permissions);
    entry.atime = attributes->atime;
    entry.atimeNsec = attributes->atime_nseconds;
    entry.createTime = attributes->createtime;
    entry.createTimeNsec = attributes->createtime_nseconds;
    entry.mtime = attributes->mtime;
    entry.mtimeNsec = attributes->mtime_nseconds;
    entry.acl = std::string{ssh_string_get_char(attributes->acl), ssh_string_len(attributes->acl)};
    return entry;
}

std::expected<std::vector<SftpSession::DirectoryEntry>, SftpSession::Error>
SftpSession::listDirectory(std::filesystem::path const& path)
{
    int closeResult = 0;
    std::vector<DirectoryEntry> entries{};

    {
        std::unique_ptr<sftp_dir_struct, std::function<void(sftp_dir_struct*)>> dir{
            sftp_opendir(session_, path.generic_string().c_str()), [&](sftp_dir_struct* dir) {
                if (dir != nullptr)
                {
                    closeResult = sftp_closedir(dir);
                }
            }};
        if (dir == nullptr)
        {
            return std::unexpected(SftpSession::Error{
                .message = ssh_get_error(session_),
                .sshError = ssh_get_error_code(session_),
                .sftpError = sftp_get_error(session_),
            });
        }

        {
            std::unique_ptr<sftp_attributes_struct, decltype(&sftp_attributes_free)> entry{
                sftp_readdir(session_, dir.get()), sftp_attributes_free};

            for (; entry != nullptr; entry.reset(sftp_readdir(session_, dir.get())))
            {
                entries.push_back(DirectoryEntry::fromSftpAttributes(entry.get()));
            }
        }

        if (!sftp_dir_eof(dir.get()))
        {
            return std::unexpected(SftpSession::Error{
                .message = ssh_get_error(session_),
                .sshError = ssh_get_error_code(session_),
                .sftpError = sftp_get_error(session_),
            });
        }
    }
    if (closeResult != SSH_OK)
    {
        return std::unexpected(SftpSession::Error{
            .message = ssh_get_error(session_),
            .sshError = closeResult,
            .sftpError = sftp_get_error(session_),
        });
    }

    return entries;
}

std::optional<SftpSession::Error> SftpSession::createDirectory(std::filesystem::path const& path)
{
    auto result = sftp_mkdir(session_, path.generic_string().c_str(), S_IRWXU);
    if (result != SSH_OK)
    {
        return SftpSession::Error{
            .message = ssh_get_error(session_),
            .sshError = result,
            .sftpError = sftp_get_error(session_),
        };
    }
    return std::nullopt;
}

SftpSession::~SftpSession()
{
    if (!moveDetector_.wasMoved())
    {
        disconnect();
    }
}

void SftpSession::disconnect()
{
    if (session_ != nullptr)
        sftp_free(session_);
    session_ = nullptr;
}