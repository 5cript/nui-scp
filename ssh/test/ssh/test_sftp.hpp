#pragma once

#include "common_fixture.hpp"
#include <ssh/session.hpp>
#include <nui/utility/scope_exit.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;
using namespace std::string_literals;

namespace SecureShell::Test
{
    class SftpTests : public CommonFixture
    {
      public:
        static constexpr std::chrono::seconds connectTimeout{2};

        void TearDown() override
        {
            if (std::filesystem::exists(programDirectory / "temp" / "log.txt"))
            {
                std::filesystem::rename(
                    programDirectory / "temp" / "log.txt",
                    programDirectory / "temp" /
                        ("log_"s + ::testing::UnitTest::GetInstance()->current_test_info()->test_case_name() + "_"s +
                         ::testing::UnitTest::GetInstance()->current_test_info()->name() + ".txt"));
            }
        }

        std::pair<std::unique_ptr<Session>, std::shared_ptr<SftpSession>> createSftpSession(unsigned short port)
        {
            auto session = makePasswordTestSession(port);
            if (!session.has_value())
                throw std::runtime_error("Failed to create session");

            (*session)->start();
            auto sftpFuture = (*session)->createSftpSession();
            if (sftpFuture.wait_for(1s) != std::future_status::ready)
                throw std::runtime_error("Failed to create sftp session");

            return std::make_pair(std::move(session).value(), sftpFuture.get().value().lock());
        }

      protected:
    };

    TEST_F(SftpTests, CanCreateSftpSession)
    {
        CREATE_SERVER_AND_JOINER(Sftp);

        auto client = makePasswordTestSession(serverStartResult->port);
        ASSERT_TRUE(client.has_value());
        (*client)->start();
        auto sftpFuture = (*client)->createSftpSession();
        ASSERT_TRUE(sftpFuture.wait_for(1s) == std::future_status::ready);
        auto sftp = sftpFuture.get();
        EXPECT_TRUE(sftp.has_value());
    }

    TEST_F(SftpTests, CanListDirectory)
    {
        CREATE_SERVER_AND_JOINER(Sftp);
        auto [_, sftp] = createSftpSession(serverStartResult->port);

        auto fut = sftp->listDirectory("/home/test");
        ASSERT_EQ(fut.wait_for(1s), std::future_status::ready);
        auto result = fut.get();

        ASSERT_TRUE(result.has_value());
        EXPECT_GT(result.value().size(), 0);
        auto it = std::find_if(result.value().begin(), result.value().end(), [](const auto& entry) {
            return entry.path == "file1.txt";
        });
        EXPECT_NE(it, result.value().end());
    }

    TEST_F(SftpTests, ListingNonExistantDirectoryMayFail)
    {
        CREATE_SERVER_AND_JOINER(Sftp);
        auto [_, sftp] = createSftpSession(serverStartResult->port);

        auto fut = sftp->listDirectory("/home/test/nonexistant");
        ASSERT_EQ(fut.wait_for(1s), std::future_status::ready);
        auto result = fut.get();
        EXPECT_FALSE(result.has_value());
    }

    TEST_F(SftpTests, CanCreateDirectory)
    {
        CREATE_SERVER_AND_JOINER(Sftp);
        auto [_, sftp] = createSftpSession(serverStartResult->port);

        auto fut = sftp->createDirectory("/home/test/newdir");
        ASSERT_EQ(fut.wait_for(1s), std::future_status::ready);
        auto result = fut.get();
        ASSERT_TRUE(result.has_value());

        auto listFut = sftp->listDirectory("/home/test");
        ASSERT_EQ(listFut.wait_for(1s), std::future_status::ready);
        auto listResult = listFut.get();
        ASSERT_TRUE(listResult.has_value());

        auto it = std::find_if(listResult.value().begin(), listResult.value().end(), [](const auto& entry) {
            return entry.path == "newdir";
        });
        EXPECT_NE(it, listResult.value().end());
    }

    TEST_F(SftpTests, CreatingDirectoryThatAlreadyExistsMayFail)
    {
        CREATE_SERVER_AND_JOINER(Sftp);
        auto [_, sftp] = createSftpSession(serverStartResult->port);

        auto fut = sftp->createDirectory("/home/test");
        ASSERT_EQ(fut.wait_for(1s), std::future_status::ready);
        auto result = fut.get();
        EXPECT_FALSE(result.has_value());
    }

    TEST_F(SftpTests, CanCreateFile)
    {
        CREATE_SERVER_AND_JOINER(Sftp);
        auto [_, sftp] = createSftpSession(serverStartResult->port);

        auto fut = sftp->createFile("/home/test/newfile.txt");
        ASSERT_EQ(fut.wait_for(1s), std::future_status::ready);
        auto result = fut.get();
        ASSERT_TRUE(result.has_value());

        auto listFut = sftp->listDirectory("/home/test");
        ASSERT_EQ(listFut.wait_for(1s), std::future_status::ready);
        auto listResult = listFut.get();
        ASSERT_TRUE(listResult.has_value());

        auto it = std::find_if(listResult.value().begin(), listResult.value().end(), [](const auto& entry) {
            return entry.path == "newfile.txt";
        });
        EXPECT_NE(it, listResult.value().end());
    }

    TEST_F(SftpTests, CreatingFileInNonExistantDirectoryMayFail)
    {
        CREATE_SERVER_AND_JOINER(Sftp);
        auto [_, sftp] = createSftpSession(serverStartResult->port);

        auto fut = sftp->createFile("/home/test/nonexistant/newfile.txt");
        ASSERT_EQ(fut.wait_for(1s), std::future_status::ready);
        auto result = fut.get();
        EXPECT_FALSE(result.has_value());
    }

    TEST_F(SftpTests, CanRemoveFile)
    {
        CREATE_SERVER_AND_JOINER(Sftp);
        auto [_, sftp] = createSftpSession(serverStartResult->port);

        auto fut = sftp->remove("/home/test/file1.txt");
        ASSERT_EQ(fut.wait_for(1s), std::future_status::ready);
        auto result = fut.get();
        ASSERT_TRUE(result.has_value());

        auto listFut = sftp->listDirectory("/home/test");
        ASSERT_EQ(listFut.wait_for(1s), std::future_status::ready);
        auto listResult = listFut.get();
        ASSERT_TRUE(listResult.has_value());

        auto it = std::find_if(listResult.value().begin(), listResult.value().end(), [](const auto& entry) {
            return entry.path == "file1.txt";
        });
        EXPECT_EQ(it, listResult.value().end());
    }

    TEST_F(SftpTests, RemovingNonExistantFileMayFail)
    {
        CREATE_SERVER_AND_JOINER(Sftp);
        auto [_, sftp] = createSftpSession(serverStartResult->port);

        auto fut = sftp->remove("/home/test/nonexistant.txt");
        ASSERT_EQ(fut.wait_for(1s), std::future_status::ready);
        auto result = fut.get();
        EXPECT_FALSE(result.has_value());
    }
}
