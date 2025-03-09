#pragma once

#include <ssh/file_stream_interface.hpp>

#include <gmock/gmock.h>

#include <future>
#include <optional>
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <memory>

namespace SecureShell::Test
{
    class FileStreamMock : public SecureShell::IFileStream
    {
      public:
        MOCK_METHOD((std::future<std::expected<void, SftpError>>), seek, (std::size_t), (override));
        MOCK_METHOD((std::future<std::expected<std::size_t, SftpError>>), tell, (), (override));
        MOCK_METHOD((std::future<std::expected<FileInformation, SftpError>>), stat, (), (override));
        MOCK_METHOD((std::future<std::expected<void, SftpError>>), rewind, (), (override));
        MOCK_METHOD(
            (std::future<std::expected<std::size_t, SftpError>>),
            read,
            (std::byte * buffer, std::size_t bufferSize),
            (override));
        MOCK_METHOD(
            (std::future<std::expected<std::size_t, SftpError>>),
            read,
            (std::function<bool(std::string_view data)> onRead),
            (override));
        MOCK_METHOD((std::future<std::expected<void, SftpError>>), write, (std::string_view data), (override));
        MOCK_METHOD(std::size_t, writeLengthLimit, (), (const, override));
        MOCK_METHOD(std::size_t, readLengthLimit, (), (const, override));
        MOCK_METHOD(sftp_file, release, (), (override));
        MOCK_METHOD(void, close, (bool isBackElement), (override));
        MOCK_METHOD(ProcessingStrand*, strand, (), (const, override));
    };
    ;
}