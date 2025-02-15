#pragma once

#include "utility/node.hpp"
#include <ssh/session.hpp>
#include <nui/utility/scope_exit.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

extern std::filesystem::path programDirectory;

using namespace std::chrono_literals;
using namespace std::string_literals;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"

static const char sshServerBashEmulator[] = {
#embed "./test_ssh2_servers/bash_emulator.mjs"
    ,
    '\0',
};

static const char privateKey[] = {
#embed "./test_ssh2_servers/key.private"
    ,
    '\0',
};

static const char publicKey[] = {
#embed "./test_ssh2_servers/key.public"
    ,
    '\0',
};

#pragma clang diagnostic pop

namespace SecureShell::Test
{
    class SshSessionTests : public ::testing::Test
    {
      protected:
        void SetUp() override
        {
            if (!std::filesystem::exists(programDirectory / "node_modules"))
                npmInstall(pool_.get_executor(), programDirectory, defaultPackageJson_);

            std::ofstream privateKeyFile{programDirectory / "temp" / "key.private", std::ios_base::binary};
            privateKeyFile.write(privateKey, std::strlen(privateKey));
        }

        void TearDown() override
        {}

        std::pair<std::shared_ptr<NodeProcessResult>, std::thread> createSshServer()
        {
            std::shared_ptr<NodeProcessResult> result{};
            std::promise<void> processResultAvailable{};
            std::thread processThread{[this, &result, &processResultAvailable]() mutable {
                result = nodeProcess(pool_.get_executor(), isolateDirectory_, std::string{sshServerBashEmulator});
                auto resultShareCopy = result;
                processResultAvailable.set_value();
                if (resultShareCopy->mainModule)
                    resultShareCopy->code = resultShareCopy->mainModule->wait();
                else
                {
                    // ???
                    throw std::runtime_error("No main module, why");
                }
            }};
            processResultAvailable.get_future().wait();
            if (result->port == 0)
            {
                if (processThread.joinable())
                    processThread.join();
                return {nullptr, {}};
            }
            return {result, std::move(processThread)};
        }

        auto makePasswordTestSession(unsigned short port)
        {
            auto options = Persistence::SshTerminalEngine{};
            options.isPty = true;
            options.sshSessionOptions = Persistence::SshSessionOptions{
                .host = "127.0.0.1",
                .port = port,
                .user = "test",
                .sshOptions =
                    Persistence::SshOptions{
                        .connectTimeoutSeconds = 2,
                    },
            };

            return makeSession(
                Persistence::SshTerminalEngine{options},
                +[](char const*, char* buf, std::size_t length, int, int, void*) {
                    std::strncpy(buf, "test", std::min(4ull, length - 1));
                    return 0;
                },
                nullptr,
                nullptr,
                nullptr);
        }

        void channelStartReading(
            std::shared_ptr<SecureShell::Channel> const& channel,
            std::function<void(std::string const&)> onStdout = {},
            std::function<void(std::string const&)> onStderr = {},
            std::function<void()> onExit = {})
        {
            channel->startReading(
                onStdout ? onStdout : [](std::string const&) {},
                onStderr ? onStderr : [](std::string const) {},
                [this, onExit = std::move(onExit)]() {
                    if (onExit)
                        onExit();
                    if (!onExitAwaiterSet_.exchange(true))
                        onExitAwaiter_.set_value();
                });
        }

        auto awaitChannelExit()
        {
            return onExitAwaiter_.get_future().wait_for(10s);
        }

        std::atomic_bool onExitAwaiterSet_ = false;
        std::promise<void> onExitAwaiter_{};

        TemporaryDirectory isolateDirectory_{programDirectory / "temp", false};
        boost::asio::thread_pool pool_{1};
        nlohmann::json defaultPackageJson_ = nlohmann::json({
            {"name", "test"},
            {"version", "1.0.0"},
            {"description", "test"},
            {"main", "main.mjs"},
            {
                "dependencies",
                {
                    {"ssh2", "1.16.0"},
                    {"blessed", "0.1.81"},
                    {"nanoid", "5.1.0"},
                    {"@xterm/headless", "5.6.0-beta.98"},
                    {"minimist", "1.2.8"},
                },
            },
        });
    };

    TEST_F(SshSessionTests, CanCreateSshSession)
    {
        SecureShell::Session client{};
    }

    TEST_F(SshSessionTests, CanStartAndStopSession)
    {
        auto [result, processThread] = createSshServer();
        ASSERT_TRUE(result);
        auto joiner = Nui::ScopeExit{[&]() noexcept {
            result->command("exit");
            if (processThread.joinable())
                processThread.join();
        }};

        auto expectedSession = makePasswordTestSession(result->port);

        ASSERT_TRUE(expectedSession.has_value());
        auto session = std::move(expectedSession).value();
        session->start();
        EXPECT_TRUE(session->isRunning());
        session->stop();
        EXPECT_FALSE(session->isRunning());
    }

    TEST_F(SshSessionTests, CanConnectToSshServer)
    {
        auto [result, processThread] = createSshServer();
        ASSERT_TRUE(result);
        auto joiner = Nui::ScopeExit{[&]() noexcept {
            if (processThread.joinable())
                processThread.join();
        }};

        auto expectedSession = makePasswordTestSession(result->port);

        ASSERT_TRUE(expectedSession.has_value());
        auto session = std::move(expectedSession).value();
        session->start();

        auto expectedChannel = session->createPtyChannel({}).get();
        ASSERT_TRUE(expectedChannel.has_value());
        auto channel = expectedChannel.value().lock();

        channelStartReading(channel);

        channel->write("exit");
        channel->write("\r");
        ASSERT_EQ(awaitChannelExit(), std::future_status::ready);

        channel->close();

        EXPECT_EQ(result->killed, false);
        EXPECT_EQ(result->code, 0);
    }

    TEST_F(SshSessionTests, CanConnectToSshServerAndDestructWithoutCrash)
    {
        auto [result, processThread] = createSshServer();
        ASSERT_TRUE(result);
        auto joiner = Nui::ScopeExit{[&]() noexcept {
            if (processThread.joinable())
                processThread.join();
        }};

        auto sessionScope = [this, &result]() {
            auto expectedSession = makePasswordTestSession(result->port);

            ASSERT_TRUE(expectedSession.has_value());
            auto session = std::move(expectedSession).value();
            session->start();

            auto expectedChannel = session->createPtyChannel({}).get();
            ASSERT_TRUE(expectedChannel.has_value());
            auto channel = expectedChannel.value().lock();
        };

        ASSERT_NO_FATAL_FAILURE(sessionScope());
    }
}