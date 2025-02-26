#include <ssh/sftp_session.hpp>
#include <ssh/session.hpp>

#include <fcntl.h>

namespace SecureShell
{
    SftpSession::SftpSession(Session* owner, sftp_session session)
        : ownerMutex_{}
        , owner_{owner}
        , session_{session}
    {}
    SftpSession::~SftpSession()
    {
        /* close makes no sense, since this lives in a shared_ptr and will only ever end here when it was already
         * removed */
    }
    void SftpSession::close()
    {
        std::scoped_lock lock{ownerMutex_};
        if (owner_)
        {
            owner_->sftpSessionRemoveItself(this);
            owner_ = nullptr;
        }
    }
    SftpSession::DirectoryEntry SftpSession::DirectoryEntry::fromSftpAttributes(sftp_attributes attributes)
    {
        DirectoryEntry entry{
            SharedData::DirectoryEntry{
                .path = attributes->name ? std::string{attributes->name} : std::string{},
                .longName = attributes->longname ? std::string{attributes->longname} : std::string{},
                .flags = attributes->flags,
                .type = attributes->type,
                .size = attributes->size,
                .uid = attributes->uid,
                .gid = attributes->gid,
                .owner = attributes->owner ? std::string{attributes->owner} : std::string{},
                .group = attributes->group ? std::string{attributes->group} : std::string{},
                .permissions = static_cast<std::filesystem::perms>(attributes->permissions),
                .atime = attributes->atime,
                .atimeNsec = attributes->atime_nseconds,
                .createTime = attributes->createtime,
                .createTimeNsec = attributes->createtime_nseconds,
                .mtime = attributes->mtime,
                .mtimeNsec = attributes->mtime_nseconds,
                .acl = attributes->acl
                    ? std::string{ssh_string_get_char(attributes->acl), ssh_string_len(attributes->acl)}
                    : std::string{},
            },
        };

        return entry;
    }

    std::future<std::expected<std::vector<SftpSession::DirectoryEntry>, SftpSession::Error>>
    SftpSession::listDirectory(std::filesystem::path const& path)
    {
        std::scoped_lock lock{ownerMutex_};
        return owner_->processingThread_.pushPromiseTask(
            [this,
             path = std::move(path)]() -> std::expected<std::vector<SftpSession::DirectoryEntry>, SftpSession::Error> {
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
            });
    }
    std::future<std::expected<void, SftpSession::Error>>
    SftpSession::createDirectory(std::filesystem::path const& path, std::filesystem::perms permissions)
    {
        std::scoped_lock lock{ownerMutex_};
        return owner_->processingThread_.pushPromiseTask(
            [this, path = std::move(path), permissions]() -> std::expected<void, SftpSession::Error> {
                auto result = sftp_mkdir(
                    session_,
                    path.generic_string().c_str(),
                    static_cast<unsigned long>(permissions & std::filesystem::perms::mask));
                if (result != SSH_OK)
                {
                    return std::unexpected(SftpSession::Error{
                        .message = ssh_get_error(session_),
                        .sshError = result,
                        .sftpError = sftp_get_error(session_),
                    });
                }
                return {};
            });
    }

    std::future<std::expected<void, SftpSession::Error>>
    SftpSession::createFile(std::filesystem::path const& path, std::filesystem::perms permissions)
    {
        std::scoped_lock lock{ownerMutex_};
        return owner_->processingThread_.pushPromiseTask(
            [this, path = std::move(path), permissions]() -> std::expected<void, SftpSession::Error> {
                std::unique_ptr<sftp_file_struct, std::function<void(sftp_file_struct*)>> file{
                    sftp_open(
                        session_,
                        path.generic_string().c_str(),
                        O_CREAT,
                        static_cast<unsigned long>(permissions & std::filesystem::perms::mask)),
                    [&](sftp_file_struct* file) {
                        if (file != nullptr)
                        {
                            sftp_close(file);
                        }
                    }};

                if (file == nullptr)
                {
                    return std::unexpected(SftpSession::Error{
                        .message = ssh_get_error(session_),
                        .sshError = ssh_get_error_code(session_),
                        .sftpError = sftp_get_error(session_),
                    });
                }

                return {};
            });
    }

    std::future<std::expected<void, SftpSession::Error>> SftpSession::remove(std::filesystem::path const& path)
    {
        std::scoped_lock lock{ownerMutex_};
        return owner_->processingThread_.pushPromiseTask(
            [this, path = std::move(path)]() -> std::expected<void, Error> {
                auto result = sftp_unlink(session_, path.generic_string().c_str());
                if (result != SSH_OK)
                {
                    return std::unexpected(Error{
                        .message = ssh_get_error(session_),
                        .sshError = result,
                        .sftpError = sftp_get_error(session_),
                    });
                }
                return {};
            });
    }

    std::future<std::expected<SftpSession::DirectoryEntry, SftpSession::Error>>
    SftpSession::stat(std::filesystem::path const& path)
    {
        std::scoped_lock lock{ownerMutex_};
        return owner_->processingThread_.pushPromiseTask(
            [this, path = std::move(path)]() -> std::expected<DirectoryEntry, Error> {
                std::unique_ptr<sftp_attributes_struct, decltype(&sftp_attributes_free)> attributes{
                    sftp_stat(session_, path.generic_string().c_str()), sftp_attributes_free};
                if (attributes == nullptr)
                {
                    return std::unexpected(Error{
                        .message = ssh_get_error(session_),
                        .sshError = ssh_get_error_code(session_),
                        .sftpError = sftp_get_error(session_),
                    });
                }

                return DirectoryEntry::fromSftpAttributes(attributes.get());
            });
    }
}