#pragma once

#include <libssh/libsshpp.hpp>
#include <libssh/sftp.h>
#include <ssh/async/processing_thread.hpp>
#include <shared_data/directory_entry.hpp>

#include <memory>
#include <string>
#include <future>
#include <optional>
#include <expected>
#include <filesystem>

namespace SecureShell
{
    class Session;

    class SftpSession : public std::enable_shared_from_this<SftpSession>
    {
      public:
        struct Error
        {
            std::string message;
            int sshError = 0;
            int sftpError = 0;
        };

        SftpSession(Session* owner, sftp_session session);
        ~SftpSession();
        SftpSession(SftpSession const&) = delete;
        SftpSession& operator=(SftpSession const&) = delete;
        SftpSession(SftpSession&&) = delete;
        SftpSession& operator=(SftpSession&&) = delete;

        operator sftp_session()
        {
            return session_;
        }

        void close();

        struct DirectoryEntry : public SharedData::DirectoryEntry
        {
            static DirectoryEntry fromSftpAttributes(sftp_attributes attributes);
        };

        std::future<std::expected<std::vector<DirectoryEntry>, Error>> listDirectory(std::filesystem::path const& path);
        std::future<std::expected<void, Error>> createDirectory(
            std::filesystem::path const& path,
            std::filesystem::perms permissions = std::filesystem::perms::owner_all);
        std::future<std::expected<void, Error>> createFile(
            std::filesystem::path const& path,
            std::filesystem::perms permissions = std::filesystem::perms::owner_read |
                std::filesystem::perms::owner_write);
        std::future<std::expected<void, Error>> remove(std::filesystem::path const& path);
        std::future<std::expected<DirectoryEntry, Error>> stat(std::filesystem::path const& path);

      private:
        std::mutex ownerMutex_;
        Session* owner_;
        sftp_session session_;
        // std::optional<ProcessingThread::PermanentTaskId> readTaskId_{std::nullopt};
    };
}