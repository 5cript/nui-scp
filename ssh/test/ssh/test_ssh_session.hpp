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

static const char sshServerNoop[] = {
#embed "./test_ssh2_servers/noop.mjs"
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
            std::thread processThread{[&]() {
                result = nodeProcess(pool_.get_executor(), isolateDirectory_, std::string{sshServerNoop}, std::nullopt);
                processResultAvailable.set_value();
                result->code = result->mainModule->wait();
            }};
            processResultAvailable.get_future().wait();
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
                },
            },
        });
    };

    TEST_F(SshSessionTests, CanCreateSshSession)
    {
        SecureShell::Session client{};
    }

    TEST_F(SshSessionTests, CanConnectToSshServer)
    {
        auto [result, processThread] = createSshServer();
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

        channel->write("exit\r");
        ASSERT_EQ(awaitChannelExit(), std::future_status::ready);

        channel->close();

        EXPECT_EQ(result->killed, false);
        EXPECT_EQ(result->code, 0);
    }
}