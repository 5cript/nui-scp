#pragma once

#include <backend/sftp/download_operation.hpp>
#include <ssh/mocks/file_stream_mock.hpp>
#include <utility/temporary_directory.hpp>

#include <utility/awaiter.hpp>

#include <gtest/gtest.h>

#include <thread>
#include <string>
#include <memory>
#include <tuple>
#include <vector>

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
            for (int i = 0; i != 10; ++i)
                fakeFileContent_ += "This is a test file content.\n";

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
                onReadResult_ = onRead_({});
                return;
            }

            if (onRead_)
            {
                onReadResult_ = onRead_(std::string_view{fakeFileContent_}.substr(readOffset_, chunkSize));
                readOffset_ += chunkSize;
                if (!onReadResult_)
                {
                    readPromise_.set_value(readOffset_);
                    return;
                }
            }
            else
            {
                throw std::runtime_error("No read callback set.");
            }
        }

        std::string readFile(std::filesystem::path const& path)
        {
            std::ifstream file{path};
            return std::string{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
        }

      protected:
        std::string fakeFileContent_{};
        Utility::TemporaryDirectory isolateDirectory_{programDirectory / "temp", true};
        SecureShell::ProcessingThread processingThread_{};
        std::unique_ptr<SecureShell::ProcessingStrand> strand_{processingThread_.createStrand()};
        std::promise<std::expected<std::size_t, SecureShell::SftpError>> readPromise_{};
        std::function<bool(std::string_view data)> onRead_{};
        std::size_t readOffset_{0};
        bool onReadResult_{true};
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

        EXPECT_TRUE(operation.cancel().has_value());

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

        EXPECT_TRUE(operation.cancel().has_value());

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
        EXPECT_TRUE(operation.cancel().has_value());

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

    TEST_F(DownloadOperationTests, StartCallsReadOnFileStream)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, 42);
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());
    }

    TEST_F(DownloadOperationTests, PrepareReservesSpaceForFile)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .reserveSpace = true,
            .doCleanup = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        EXPECT_TRUE(operation.cancel().has_value());

        EXPECT_EQ(
            std::filesystem::file_size(options.localPath.generic_string() + ".filepart"), fakeFileContent_.size());
    }

    TEST_F(DownloadOperationTests, ReadCycleWritesDataToFile)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        fakeReadCycle(5);
        fakeReadCycle(0); // EOF Cycle

        EXPECT_TRUE(operation.cancel().has_value());

        EXPECT_EQ(std::filesystem::file_size(options.localPath.generic_string() + ".filepart"), 5);
        EXPECT_EQ(readFile(options.localPath.generic_string() + ".filepart"), fakeFileContent_.substr(0, 5));
    }

    TEST_F(DownloadOperationTests, ReadWritesFileThroughMultipleCycles)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        for (std::size_t i = 0; i < fakeFileContent_.size(); ++i)
            fakeReadCycle(1);
        fakeReadCycle(0); // EOF Cycle

        EXPECT_TRUE(operation.cancel().has_value());

        EXPECT_EQ(
            std::filesystem::file_size(options.localPath.generic_string() + ".filepart"), fakeFileContent_.size());
        EXPECT_EQ(readFile(options.localPath.generic_string() + ".filepart"), fakeFileContent_);
    }

    TEST_F(DownloadOperationTests, ReadWritesFileThroughSingleCycle)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        fakeReadCycle(fakeFileContent_.size());
        fakeReadCycle(0); // EOF Cycle

        EXPECT_TRUE(operation.cancel().has_value());

        EXPECT_EQ(
            std::filesystem::file_size(options.localPath.generic_string() + ".filepart"), fakeFileContent_.size());
        EXPECT_EQ(readFile(options.localPath.generic_string() + ".filepart"), fakeFileContent_);
    }

    TEST_F(DownloadOperationTests, CanPauseRead)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        fakeReadCycle(5);

        std::expected<void, DownloadOperation::Error> pauseResult;
        std::promise<void> threadStarted;
        std::thread asyncPause{[&operation, &pauseResult, &threadStarted]() {
            threadStarted.set_value();
            pauseResult = operation.pause();
        }};
        threadStarted.get_future().wait();
        // dont know actually how to wait for the pause, except for burdening the wait on the user.
        std::this_thread::sleep_for(200ms);

        // This one still goes through but now signals a stop to the read.
        fakeReadCycle(5);

        asyncPause.join();

        EXPECT_TRUE(operation.cancel().has_value());

        EXPECT_EQ(onReadResult_, false);
        EXPECT_EQ(std::filesystem::file_size(options.localPath.generic_string() + ".filepart"), 10);
        EXPECT_EQ(readFile(options.localPath.generic_string() + ".filepart"), fakeFileContent_.substr(0, 10));
    }

    TEST_F(DownloadOperationTests, CanContinueReadingAfterPause)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        fakeReadCycle(5);

        std::expected<void, DownloadOperation::Error> pauseResult;
        std::promise<void> threadStarted;
        std::thread asyncPause{[&operation, &pauseResult, &threadStarted]() {
            threadStarted.set_value();
            pauseResult = operation.pause();
        }};
        threadStarted.get_future().wait();
        // dont know actually how to wait for the pause, except for burdening the wait on the user.
        std::this_thread::sleep_for(200ms);

        // This one still goes through but now signals a stop to the read.
        fakeReadCycle(5);

        asyncPause.join();

        EXPECT_EQ(onReadResult_, false);

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        // This one should go through and continue the read.
        fakeReadCycle(fakeFileContent_.size() - 10);
        fakeReadCycle(0); // EOF Cycle

        result = operation.cancel();
        EXPECT_TRUE(result.has_value());

        EXPECT_EQ(onReadResult_, true);
        EXPECT_EQ(
            std::filesystem::file_size(options.localPath.generic_string() + ".filepart"), fakeFileContent_.size());
        EXPECT_EQ(readFile(options.localPath.generic_string() + ".filepart"), fakeFileContent_);
    }

    TEST_F(DownloadOperationTests, CannotFinalizeWhileReading)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        fakeReadCycle(5);

        auto fin = operation.finalize();
        EXPECT_FALSE(fin.has_value());
        EXPECT_EQ(fin.error().type, DownloadOperation::ErrorType::CannotFinalizeDuringRead);
    }

    TEST_F(DownloadOperationTests, FinalizeFailsIfFileExistsButOverwriteIsForbidden)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .mayOverwrite = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        fakeReadCycle(fakeFileContent_.size());
        fakeReadCycle(0); // EOF Cycle

        {
            std::ofstream file{options.localPath};
        }

        auto fin = operation.finalize();
        EXPECT_FALSE(fin.has_value());
        EXPECT_EQ(fin.error().type, DownloadOperation::ErrorType::FileExists);
    }

    TEST_F(DownloadOperationTests, CanFinalizeOperation)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .mayOverwrite = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        fakeReadCycle(fakeFileContent_.size());
        fakeReadCycle(0); // EOF Cycle

        auto fin = operation.finalize();
        EXPECT_TRUE(fin.has_value());

        EXPECT_EQ(readFile(options.localPath), fakeFileContent_);
        EXPECT_FALSE(std::filesystem::exists(options.localPath.generic_string() + ".filepart"));
    }

    TEST_F(DownloadOperationTests, FinalizeSucceedsIfFileExistsButOverwriteIsAllowed)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .localPath = isolateDirectory_.path() / "file.txt",
            .mayOverwrite = true,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        fakeReadCycle(fakeFileContent_.size());
        fakeReadCycle(0); // EOF Cycle

        {
            std::ofstream file{options.localPath};
        }

        auto fin = operation.finalize();
        EXPECT_TRUE(fin.has_value());
    }

    TEST_F(DownloadOperationTests, ProgressCallbackIsCalledDuringRead)
    {
        using namespace SecureShell;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        std::vector<std::tuple<std::int64_t, std::int64_t, std::int64_t>> progressCalls;

        auto options = DownloadOperation::DownloadOperationOptions{
            .progressCallback =
                [&progressCalls](std::int64_t min, std::int64_t max, std::int64_t current) {
                    progressCalls.emplace_back(min, max, current);
                },
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        for (std::size_t i = 0; i < fakeFileContent_.size(); ++i)
            fakeReadCycle(1);
        fakeReadCycle(0); // EOF Cycle

        EXPECT_TRUE(operation.cancel().has_value());

        EXPECT_EQ(progressCalls.size(), fakeFileContent_.size());
        for (std::size_t i = 0; i < fakeFileContent_.size(); ++i)
        {
            EXPECT_EQ(std::get<0>(progressCalls[i]), 0);
            EXPECT_EQ(std::get<1>(progressCalls[i]), fakeFileContent_.size());
            EXPECT_EQ(std::get<2>(progressCalls[i]), i + 1);
        }

        EXPECT_EQ(std::get<0>(progressCalls.back()), 0);
        EXPECT_EQ(std::get<1>(progressCalls.back()), fakeFileContent_.size());
        EXPECT_EQ(std::get<2>(progressCalls.back()), fakeFileContent_.size());
    }

    TEST_F(DownloadOperationTests, CompletionCallbackIsCalled)
    {
        using namespace SecureShell;

        std::promise<bool> completionPromise;

        auto fileStream = makeFileStreamMock();
        giveMockDefaultStat(fileStream, fakeFileContent_.size());
        giveMockExpectedRead(fileStream);

        auto options = DownloadOperation::DownloadOperationOptions{
            .onCompletionCallback =
                [&completionPromise](bool success) {
                    completionPromise.set_value(success);
                },
            .localPath = isolateDirectory_.path() / "file.txt",
            .doCleanup = false,
        };
        DownloadOperation operation{fileStream, options};

        auto result = operation.prepare();
        EXPECT_TRUE(result.has_value());

        result = operation.start();
        EXPECT_TRUE(result.has_value());

        for (std::size_t i = 0; i < fakeFileContent_.size(); ++i)
            fakeReadCycle(1);
        fakeReadCycle(0); // EOF Cycle

        auto future = completionPromise.get_future();
        ASSERT_EQ(future.wait_for(1s), std::future_status::ready);
        EXPECT_TRUE(future.get());
    }
}