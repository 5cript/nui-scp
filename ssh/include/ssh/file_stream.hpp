#pragma once

#include <ssh/sftp_error.hpp>

#include <libssh/sftp.h>

#include <memory>
#include <functional>
#include <future>
#include <expected>

namespace SecureShell
{
    class Session;
    class SftpSession;

    class FileStream : public std::enable_shared_from_this<FileStream>
    {
      public:
        FileStream(std::shared_ptr<SftpSession> sftp, sftp_file file, sftp_limits_struct limits);
        ~FileStream();
        FileStream(FileStream const&) = delete;
        FileStream& operator=(FileStream const&) = delete;
        FileStream(FileStream&&);
        FileStream& operator=(FileStream&&);

        /**
         * @brief Moves the file pointer to the specified position.
         *
         * @param pos Where to move the file pointer to.
         */
        std::future<std::expected<void, SftpError>> seek(std::size_t pos);

        /**
         * @brief Tells the current position in the file.
         *
         * @return std::future<std::expected<std::size_t, SftpError>> The current position or an error.
         */
        std::future<std::expected<std::size_t, SftpError>> tell();

        /**
         * @brief Rewind the file to the beginning.
         */
        std::future<std::expected<void, SftpError>> rewind();

        /**
         * @brief Reads some bytes from the file. Not necessarily fills the buffer. bufferSize MUST be less than or
         * equal to the read limit.
         *
         * @param buffer
         * @param bufferSize
         * @return std::future<std::expected<std::size_t, SftpError>>
         */
        std::future<std::expected<std::size_t, SftpError>> read(std::byte* buffer, std::size_t bufferSize);

        /**
         * @brief Reads all bytes from the file.
         *
         * @param onRead Let this function return false to stop reading.
         * @return std::future<std::expected<std::size_t, SftpError>> The number of bytes read or an error.
         */
        std::future<std::expected<std::size_t, SftpError>> read(std::function<bool(std::string_view data)> onRead);

        /**
         * @brief Writes some bytes to the file.
         * Makes sure that all data is written even if the data is larger than the write limit by breaking it into
         * smaller parts.
         *
         * @param buffer
         * @param bufferSize
         * @return std::future<std::expected<void, SftpError>>
         */
        std::future<std::expected<void, SftpError>> write(std::string_view data);

        /**
         * @brief Returns the maximum number of bytes that can be written in a single pure write operation.
         * This limit is not necessary to uphold for the write function of this class.
         *
         * @return std::size_t
         */
        std::size_t writeLengthLimit() const;

        /**
         * @brief Returns the maximum number of bytes that can be read in a single pure read operation.
         *
         * @return std::size_t
         */
        std::size_t readLengthLimit() const;

        /**
         * @brief Brings this class into an invalid state and returns the sftp_file. The ownership of the file is
         * transferred to the caller.
         *
         * @return sftp_file
         */
        sftp_file release();

        /**
         * @brief Closes the file and removes itself from the sftp session.
         */
        void close(bool isBackElement = false);

      private:
        std::function<void(sftp_file)> makeFileDeleter();

        template <typename FunctionT>
        void perform(FunctionT&& func);

        template <typename FunctionT>
        auto performPromise(FunctionT&& func);

        SftpError lastError() const;

        void writePart(std::string_view toWrite, std::function<void(std::expected<void, SftpError>&&)> onWriteComplete);

      private:
        std::weak_ptr<SftpSession> sftp_;
        std::unique_ptr<sftp_file_struct, std::function<void(sftp_file)>> file_;
        sftp_limits_struct limits_;
    };
}