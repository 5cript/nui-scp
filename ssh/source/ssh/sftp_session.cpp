#include <ssh/sftp_session.hpp>
#include <ssh/session.hpp>

#include <fcntl.h>

namespace SecureShell
{
    SftpSession::SftpSession(Session* owner, std::unique_ptr<ProcessingStrand> strand, sftp_session session)
        : owner_{owner}
        , strand_{std::move(strand)}
        , session_{session}
        , fileStreams_{}
    {}
    SftpSession::~SftpSession()
    {
        /* close makes no sense, since this lives in a shared_ptr and will only ever end here when it was already
         * removed */
    }
    bool SftpSession::close(bool isBackElement)
    {
        if (strand_->isFinalized())
            return false;

        return strand_
            ->pushFinalPromiseTask([this, isBackElement]() {
                removeAllFileStreams();
                owner_->sftpSessionRemoveItself(this, isBackElement);
                return true;
            })
            .get();
    }
    void SftpSession::fileStreamRemoveItself(FileStream* stream, bool isBackElement)
    {
        if (isBackElement && fileStreams_.back().get() == stream)
        {
            fileStreams_.pop_back();
        }
        else
        {
            fileStreams_.erase(
                std::remove_if(
                    fileStreams_.begin(),
                    fileStreams_.end(),
                    [stream](auto const& item) {
                        return item.get() == stream;
                    }),
                fileStreams_.end());
        }
    }
    void SftpSession::removeAllFileStreams()
    {
        while (!fileStreams_.empty())
        {
            fileStreams_.back()->close(true);
        }
    }
    SftpSession::DirectoryEntry SftpSession::DirectoryEntry::fromSftpAttributes(sftp_attributes attributes)
    {
        return DirectoryEntry{
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
    }

    std::future<std::expected<std::vector<SftpSession::DirectoryEntry>, SftpSession::Error>>
    SftpSession::listDirectory(std::filesystem::path const& path)
    {
        return performPromise(
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
        return performPromise([this, path = std::move(path), permissions]() -> std::expected<void, SftpSession::Error> {
            auto result = sftp_mkdir(
                session_,
                path.generic_string().c_str(),
                static_cast<unsigned long>(permissions & std::filesystem::perms::mask));
            if (result != SSH_OK)
                return std::unexpected(lastError());
            return {};
        });
    }

    std::future<std::expected<void, SftpSession::Error>>
    SftpSession::createFile(std::filesystem::path const& path, std::filesystem::perms permissions)
    {
        return performPromise([this, path = std::move(path), permissions]() -> std::expected<void, SftpSession::Error> {
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
                return std::unexpected(lastError());

            return {};
        });
    }

    std::future<std::expected<void, SftpSession::Error>> SftpSession::removeFile(std::filesystem::path const& path)
    {
        return performPromise([this, path = std::move(path)]() -> std::expected<void, Error> {
            auto result = sftp_unlink(session_, path.generic_string().c_str());
            if (result != SSH_OK)
                return std::unexpected(lastError());
            return {};
        });
    }

    std::future<std::expected<void, SftpSession::Error>> SftpSession::removeDirectory(std::filesystem::path const& path)
    {
        return performPromise([this, path = std::move(path)]() -> std::expected<void, Error> {
            auto result = sftp_rmdir(session_, path.generic_string().c_str());
            if (result != SSH_OK)
                return std::unexpected(lastError());
            return {};
        });
    }

    std::future<std::expected<SftpSession::DirectoryEntry, SftpSession::Error>>
    SftpSession::stat(std::filesystem::path const& path)
    {
        return performPromise([this, path = std::move(path)]() -> std::expected<DirectoryEntry, Error> {
            std::unique_ptr<sftp_attributes_struct, decltype(&sftp_attributes_free)> attributes{
                sftp_stat(session_, path.generic_string().c_str()), sftp_attributes_free};
            if (attributes == nullptr)
                return std::unexpected(lastError());

            return DirectoryEntry::fromSftpAttributes(attributes.get());
        });
    }

    std::future<std::expected<void, SftpSession::Error>>
    SftpSession::stat(std::filesystem::path const& path, sftp_attributes attributes)
    {
        return performPromise([this, path = std::move(path), attributes]() -> std::expected<void, Error> {
            auto result = sftp_setstat(session_, path.generic_string().c_str(), attributes);
            if (result != SSH_OK)
                return std::unexpected(lastError());
            return {};
        });
    }

    std::future<std::expected<void, SftpSession::Error>>
    SftpSession::rename(std::filesystem::path const& source, std::filesystem::path const& destination)
    {
        return performPromise(
            [this, source = std::move(source), destination = std::move(destination)]() -> std::expected<void, Error> {
                auto s = source.generic_string();
                auto d = destination.generic_string();

                auto result = sftp_rename(session_, s.c_str(), d.c_str());
                if (result != SSH_OK)
                {
                    const auto le = lastError();
                    return std::unexpected(le);
                }
                return {};
            });
    }

    std::future<std::expected<void, SftpSession::Error>>
    SftpSession::chown(std::filesystem::path const& path, uid_t owner, gid_t group)
    {
        return performPromise([this, path = std::move(path), owner, group]() -> std::expected<void, Error> {
            auto result = sftp_chown(session_, path.generic_string().c_str(), owner, group);
            if (result != SSH_OK)
                return std::unexpected(lastError());
            return {};
        });
    }

    std::future<std::expected<void, SftpSession::Error>>
    SftpSession::chmod(std::filesystem::path const& path, std::filesystem::perms perms)
    {
        return performPromise([this, path = std::move(path), perms]() -> std::expected<void, Error> {
            auto result = sftp_chmod(session_, path.generic_string().c_str(), static_cast<mode_t>(perms));
            if (result != SSH_OK)
                return std::unexpected(lastError());
            return {};
        });
    }

    SftpError SftpSession::lastError() const
    {
        return SftpError{
            .message = ssh_get_error(session_),
            .sshError = ssh_get_error_code(session_),
            .sftpError = sftp_get_error(session_),
        };
    }

    std::future<std::expected<sftp_limits_struct, SftpSession::Error>> SftpSession::limits()
    {
        return performPromise([this]() -> std::expected<sftp_limits_struct, Error> {
            auto const* limits = sftp_limits(session_);
            if (limits == nullptr)
                return std::unexpected(lastError());
            return *limits;
        });
    }

    std::future<std::expected<std::weak_ptr<FileStream>, SftpSession::Error>>
    SftpSession::openFile(std::filesystem::path const& path, OpenType openType, std::filesystem::perms permissions)
    {
        return performPromise(
            [this, path = std::move(path), openType, permissions]() -> std::expected<std::weak_ptr<FileStream>, Error> {
                std::unique_ptr<sftp_file_struct, std::function<void(sftp_file_struct*)>> file{
                    sftp_open(
                        session_,
                        path.generic_string().c_str(),
                        static_cast<int>(openType),
                        static_cast<unsigned long>(permissions & std::filesystem::perms::mask)),
                    [&](sftp_file_struct* file) {
                        if (file != nullptr)
                        {
                            sftp_close(file);
                        }
                    }};

                if (!file)
                    return std::unexpected(lastError());

                auto const* limits = sftp_limits(session_);
                if (limits == nullptr)
                    return std::unexpected(lastError());

                auto stream = std::make_shared<FileStream>(shared_from_this(), file.release(), *limits);
                fileStreams_.push_back(stream);

                return std::weak_ptr<FileStream>{stream};
            });
    }
}