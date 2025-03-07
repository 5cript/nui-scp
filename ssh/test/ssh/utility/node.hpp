#pragma once

#include "./temporary_directory.hpp"

#include <boost/asio.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#include <boost/process/v2.hpp>
#pragma clang diagnostic pop

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <optional>

namespace SecureShell::Test
{
    constexpr static auto processKillTimer = std::chrono::seconds{10};

    struct NodeProcessResult
    {
        boost::asio::deadline_timer timer;
        boost::asio::writable_pipe stdinPipe;
        boost::asio::readable_pipe stdoutPipe;
        boost::asio::readable_pipe stderrPipe;
        std::unique_ptr<boost::process::v2::process> mainModule;
        std::atomic_bool killed = false;
        unsigned short port = 0;
        int code = 0;

        void command(std::string const& command);
        void terminate();
    };

    void npmInstall(
        boost::asio::any_io_executor executor,
        std::filesystem::path const& directory,
        nlohmann::json const& packageJson);

    std::shared_ptr<NodeProcessResult> nodeProcess(
        boost::asio::any_io_executor executor,
        TemporaryDirectory const& isolateDirectory,
        std::string const& program);
}