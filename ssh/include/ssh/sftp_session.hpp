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

        /**
         * @brief Lists the contents of a directory.
         *
         * @param path
         * @return std::future<std::expected<std::vector<DirectoryEntry>, Error>>
         */
        std::future<std::expected<std::vector<DirectoryEntry>, Error>> listDirectory(std::filesystem::path const& path);

        /**
         * @brief Create a directory.
         *
         * @param path
         * @param permissions
         * @return std::future<std::expected<void, Error>>
         */
        std::future<std::expected<void, Error>> createDirectory(
            std::filesystem::path const& path,
            std::filesystem::perms permissions = std::filesystem::perms::owner_all);

        /**
         * @brief Opens a file, creating it if it does not exist, then closes it.
         *
         * @param path
         * @param permissions
         * @return std::future<std::expected<void, Error>>
         */
        std::future<std::expected<void, Error>> createFile(
            std::filesystem::path const& path,
            std::filesystem::perms permissions = std::filesystem::perms::owner_read |
                std::filesystem::perms::owner_write);

        /**
         * @brief Removes a file.
         *
         * @param path
         * @return std::future<std::expected<void, Error>>
         */
        std::future<std::expected<void, Error>> removeFile(std::filesystem::path const& path);

        /**
         * @brief Removes a directory.
         *
         * @param path
         * @return std::future<std::expected<void, Error>>
         */
        std::future<std::expected<void, Error>> removeDirectory(std::filesystem::path const& path);

        /**
         * @brief Gets the attributes of a file or directory.
         *
         * @param path
         * @return std::future<std::expected<DirectoryEntry, Error>>
         */
        std::future<std::expected<DirectoryEntry, Error>> stat(std::filesystem::path const& path);

        /**
         * @brief Sets the attributes of a file or directory.
         *
         * @param path
         * @param attributes
         * @return std::future<std::expected<DirectoryEntry, Error>>
         */
        std::future<std::expected<void, Error>> stat(std::filesystem::path const& path, sftp_attributes attributes);

        /**
         * @brief Sets the owner of a file or directory.
         *
         * @param path
         * @param owner
         * @param group
         * @return std::future<std::expected<void, Error>>
         */
        std::future<std::expected<void, Error>> chown(std::filesystem::path const& path, uid_t owner, gid_t group);

        /**
         * @brief Sets the permissions of a file or directory.
         *
         * @param path
         * @param mode
         * @return std::future<std::expected<void, Error>>
         */
        std::future<std::expected<void, Error>> chmod(std::filesystem::path const& path, std::filesystem::perms perms);

        /**
         * @brief Move a file or directory.
         */
        std::future<std::expected<void, Error>>
        rename(std::filesystem::path const& source, std::filesystem::path const& destination);

      private:
        std::mutex ownerMutex_;
        Session* owner_;
        sftp_session session_;
        // std::optional<ProcessingThread::PermanentTaskId> readTaskId_{std::nullopt};
    };
}