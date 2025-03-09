#pragma once

#include <backend/sftp/download_operation.hpp>
#include <ssh/mocks/file_stream_mock.hpp>
#include <utility/temporary_directory.hpp>

#include <utility/awaiter.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace std::string_literals;

extern std::filesystem::path programDirectory;

namespace Test
{
    class DownloadOperationTests : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            processingThread_.start(5ms);
        }

        void TearDown() override
        {
            processingThread_.stop();
        }

        std::shared_ptr<::testing::NiceMock<SecureShell::Test::FileStreamMock>> makeFileStreamMock()
        {
            auto mock = std::make_shared<::testing::NiceMock<SecureShell::Test::FileStreamMock>>();

            ON_CALL(*mock, strand()).WillByDefault([this]() -> SecureShell::ProcessingStrand* {
                return strand_.get();
            });

            return mock;
        }

        void giveMockDefaultStat(
            std::shared_ptr<::testing::NiceMock<SecureShell::Test::FileStreamMock>> const& mock,
            std::optional<std::size_t> size = std::nullopt)
        {
            if (!size)
                size = fakeFileContent_.size();

            ON_CALL(*mock, stat())
                .WillByDefault(
                    [size = size.value()]()
                        -> std::future<std::expected<SecureShell::FileInformation, SecureShell::SftpError>> {
                        std::promise<std::expected<SecureShell::FileInformation, SecureShell::SftpError>> promise;
                        promise.set_value(SecureShell::FileInformation{{
                            .size = size,
                            .permissions = std::filesystem::perms::owner_all,
                        }});
                        return promise.get_future();
                    });
        }

        void giveMockExpectedRead(std::shared_ptr<::testing::NiceMock<SecureShell::Test::FileStreamMock>> const& mock)
        {
            EXPECT_CALL(*mock, read(testing::_))
                .WillRepeatedly(
                    [this](std::function<bool(std::string_view data)> cb)
                        -> std::future<std::expected<std::size_t, SecureShell::SftpError>> {
                        onRead_ = std::move(cb);
                        readPromise_ = {};
                        return readPromise_.get_future();
                    });
        }

        void fakeReadCycle(std::optional<std::size_t> chunkSizeOpt)
        {
            auto chunkSize = fakeFileContent_.size();
            if (chunkSizeOpt)
                chunkSize = chunkSizeOpt.value();

            if (readOffset_ + chunkSize > fakeFileContent_.size())
                chunkSize = fakeFileContent_.size() - readOffset_;

            // EOF:
            if (chunkSize == 0)
            {
                readPromise_.set_value(readOffset_);
                onRead_({});
                return;
            }

            if (onRead_)
            {
                onRead_(std::string_view{fakeFileContent_}.substr(readOffset_, chunkSize));
                readOffset_ += chunkSize;
            }
            else
            {
                throw std::runtime_error("No read callback set.");
            }
        }

      protected:
        std::string fakeFileContent_{"Hello, World!"};
        Utility::TemporaryDirectory isolateDirectory_{programDirectory / "temp", true};
        SecureShell::ProcessingThread processingThread_{};
        std::unique_ptr<SecureShell::ProcessingStrand> strand_{processingThread_.createStrand()};
        std::promise<std::expected<std::size_t, SecureShell::SftpError>> readPromise_{};
        std::function<bool(std::string_view data)> onRead_{};
        std::size_t readOffset_{0};
    };

    TEST_F(DownloadOperationTests, CanCreateDownloadOperation)
    {
        auto fileStream = makeFileStreamMock();
        auto options = DownloadOperation::DownloadOperationOptions{};
        DownloadOperation operation{fileStream, options};
    }

    TEST_F(DownloadOperationTests, DownloadPreparationFailsWithLocalPathEmpty)
    {
        auto fileStream = makeFileStreamMock();
        auto options = DownloadOperation::DownloadOperationOptions{};
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error().type, DownloadOperation::ErrorType::InvalidPath);
    }

    TEST_F(DownloadOperationTests, DownloadPreparationFailsWhenFileExistsAndMayNotBeOverwritten)
    {
        auto fileStream = makeFileStreamMock();
        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .mayOverwrite = false,
        };
        std::ofstream file{options.localPath};
        file.close();

        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error().type, DownloadOperation::ErrorType::FileExists);
    }

    TEST_F(DownloadOperationTests, DownloadPreparationFailsWhenStreamIsExpired)
    {
        std::weak_ptr<SecureShell::IFileStream> fileWeak;
        {
            auto fileStream = makeFileStreamMock();
            fileWeak = fileStream;
        }
        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
        };
        DownloadOperation operation{fileWeak, options};

        auto result = operation.prepare();
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error().type, DownloadOperation::ErrorType::FileStreamExpired);
    }

    TEST_F(DownloadOperationTests, DownloadPreparationFailsWhenStattingTheFileFails)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();

        EXPECT_CALL(*fileStream, stat()).WillOnce([]() -> std::future<std::expected<FileInformation, SftpError>> {
            std::promise<std::expected<FileInformation, SftpError>> promise;
            promise.set_value(std::unexpected(SftpError{
                .message = "Stat failed",
            }));
            return promise.get_future();
        });

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error().type, DownloadOperation::ErrorType::FileStatFailed);
    }

    // TODO: Test continuation download

    TEST_F(DownloadOperationTests, DownloadPreparationSucceedsWithEmptyRemoteFile)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();

        EXPECT_CALL(*fileStream, stat())
            .WillOnce([]() -> std::future<std::expected<FileInformation, SecureShell::SftpError>> {
                std::promise<std::expected<FileInformation, SecureShell::SftpError>> promise;
                promise.set_value(FileInformation{{
                    .size = 0,
                }});
                return promise.get_future();
            });

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());
    }

    TEST_F(DownloadOperationTests, DownloadPreparationSucceedsWithNonEmptyRemoteFile)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();

        EXPECT_CALL(*fileStream, stat())
            .WillOnce([]() -> std::future<std::expected<FileInformation, SecureShell::SftpError>> {
                std::promise<std::expected<FileInformation, SecureShell::SftpError>> promise;
                promise.set_value(FileInformation{{
                    .size = 42,
                }});
                return promise.get_future();
            });

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());
    }

    TEST_F(DownloadOperationTests, DownloadPreparationCreatesPartFile)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();

        EXPECT_CALL(*fileStream, stat())
            .WillOnce([]() -> std::future<std::expected<FileInformation, SecureShell::SftpError>> {
                std::promise<std::expected<FileInformation, SecureShell::SftpError>> promise;
                promise.set_value(FileInformation{{
                    .size = 42,
                }});
                return promise.get_future();
            });

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());
        EXPECT_TRUE(std::filesystem::exists(options.localPath.generic_string() + ".filepart"));
    }

    TEST_F(DownloadOperationTests, DestructorCleansUpIfOptionIsTrue)
    {
        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = true,
        };

        {
            using namespace SecureShell;

            auto fileStream = makeFileStreamMock();

            EXPECT_CALL(*fileStream, stat())
                .WillOnce([]() -> std::future<std::expected<FileInformation, SecureShell::SftpError>> {
                    std::promise<std::expected<FileInformation, SecureShell::SftpError>> promise;
                    promise.set_value(FileInformation{{
                        .size = 42,
                    }});
                    return promise.get_future();
                });

            DownloadOperation operation{fileStream, options};

            auto result = operation.prepare();
            EXPECT_TRUE(result.has_value());
            EXPECT_TRUE(std::filesystem::exists(options.localPath.generic_string() + ".filepart"));
        }

        EXPECT_FALSE(std::filesystem::exists(options.localPath.generic_string() + ".filepart"));
    }

    TEST_F(DownloadOperationTests, DestructorDoesNotCleanUpIfOptionIsFalse)
    {
        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = false,
        };

        {
            using namespace SecureShell;

            auto fileStream = makeFileStreamMock();

            EXPECT_CALL(*fileStream, stat())
                .WillOnce([]() -> std::future<std::expected<FileInformation, SecureShell::SftpError>> {
                    std::promise<std::expected<FileInformation, SecureShell::SftpError>> promise;
                    promise.set_value(FileInformation{{
                        .size = 42,
                    }});
                    return promise.get_future();
                });

            DownloadOperation operation{fileStream, options};

            auto result = operation.prepare();
            EXPECT_TRUE(result.has_value());
            EXPECT_TRUE(std::filesystem::exists(options.localPath.generic_string() + ".filepart"));
        }

        EXPECT_TRUE(std::filesystem::exists(options.localPath.generic_string() + ".filepart"));
    }

    TEST_F(DownloadOperationTests, CancelDoesNotRemoveTheFileIfCleanIsFalse)
    {
        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = false,
        };

        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream);

        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());
        EXPECT_TRUE(std::filesystem::exists(options.localPath.generic_string() + ".filepart"));

        operation.cancel();

        EXPECT_TRUE(std::filesystem::exists(options.localPath.generic_string() + ".filepart"));
    }

    TEST_F(DownloadOperationTests, CancelDoesRemoveTheFileIfCleanIsTrue)
    {
        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = true,
        };

        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();

        EXPECT_CALL(*fileStream, stat())
            .WillOnce([]() -> std::future<std::expected<FileInformation, SecureShell::SftpError>> {
                std::promise<std::expected<FileInformation, SecureShell::SftpError>> promise;
                promise.set_value(FileInformation{{
                    .size = 42,
                }});
                return promise.get_future();
            });

        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());
        EXPECT_TRUE(std::filesystem::exists(options.localPath.generic_string() + ".filepart"));

        operation.cancel();

        EXPECT_FALSE(std::filesystem::exists(options.localPath.generic_string() + ".filepart"));
    }

    TEST_F(DownloadOperationTests, CancelCallsTheCompletionHandler)
    {
        using namespace SecureShell;

        std::promise<bool> completionPromise;

        auto options = DownloadOperation::DownloadOperationOptions{
            .onCompletionCallback =
                [&completionPromise](bool success) {
                    completionPromise.set_value(success);
                },
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = true,
        };

        const auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream);

        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());
        operation.cancel();

        auto future = completionPromise.get_future();
        ASSERT_EQ(future.wait_for(1s), std::future_status::ready);
        // Cancel is a failure
        EXPECT_FALSE(future.get());
    }

    TEST_F(DownloadOperationTests, StartWithEmptyRemoteFileSucceeds)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, 0);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());
    }

    TEST_F(DownloadOperationTests, StartWithoutPrepareFails)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.start();
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error().type, DownloadOperation::ErrorType::OperationNotPrepared);
    }

    TEST_F(DownloadOperationTests, StartFailsWithExpiredFileStream)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value()) << boost::describe::enum_to_string(result.error().type, "INVALID_ENUM_VALUE");

        fileStream.reset();

        result = operation.start();
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error().type, DownloadOperation::ErrorType::FileStreamExpired);
    }
}