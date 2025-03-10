#include "node.hpp"

#include <boost/asio/deadline_timer.hpp>

#include <fstream>
#include <iostream>

constexpr static std::string_view node = NODE_EXECUTABLE;
constexpr static std::string_view npm = NPM_EXECUTABLE;
using namespace std::string_literals;

namespace SecureShell::Test
{
    namespace bp2 = boost::process::v2;

    void NodeProcessResult::command(std::string const& command)
    {
        if (port == 0 || killed || code != 0)
            return;

        nlohmann::json j = {
            {"command", command},
        };
#ifdef _WIN32
        const std::string commandWithNewline = j.dump() + "\r\n";
#else
        const std::string commandWithNewline = j.dump() + "\n";
#endif
        boost::system::error_code ec;
        boost::asio::write(stdinPipe, boost::asio::buffer(commandWithNewline), ec);
        if (ec)
        {
            std::cerr << "Failed to write command: " << ec.message() << std::endl;
            return;
        }
    }

    void NodeProcessResult::terminate()
    {
        if (killed)
            return;
        killed = true;
        try
        {
            mainModule->terminate();
        }
        catch (std::exception const& e)
        {
            std::cerr << "Failed to terminate process: " << e.what() << std::endl;
        }
    }

    void npmInstall(
        boost::asio::any_io_executor executor,
        std::filesystem::path const& directory,
        nlohmann::json const& packageJson)
    {
        auto packageJsonPath = directory / "package.json";
        {
            std::ofstream packageJsonFile{packageJsonPath};
            packageJsonFile << packageJson.dump(4);
        }

        const auto command = "\"cd "s + directory.generic_string() + " && npm install\"";
#ifdef _WIN32
        const auto shell = MSYS2_BASH;
        const auto npmShellArgs = std::vector<std::string>{
            "--login",
            "-i",
            "-c",
            std::string{command},
        };
#else
        const auto shell = "/bin/bash";
        const auto npmShellArgs = std::vector<std::string>{
            "-c",
            std::string{command},
        };
#endif

        auto child = boost::process::v2::process{
            executor,
            shell,
            npmShellArgs,
            bp2::process_environment{bp2::environment::current()},
            bp2::process_start_dir{directory.generic_string()}};
        child.wait();
    }

    std::shared_ptr<NodeProcessResult> nodeProcess(
        boost::asio::any_io_executor executor,
        Utility::TemporaryDirectory const& isolateDirectory,
        std::string const& program)
    {
        using namespace std::string_literals;

        {
            std::ofstream programFile{isolateDirectory.path() / "main.mjs", std::ios_base::binary};
            programFile.write(program.data(), program.size());
        }

        const auto nodeExecutable = boost::process::v2::filesystem::path{std::string{node}};

        auto result = std::make_shared<NodeProcessResult>(
            boost::asio::deadline_timer{executor, boost::posix_time::seconds{processKillTimer.count()}},
            boost::asio::writable_pipe{executor},
            boost::asio::readable_pipe{executor},
            boost::asio::readable_pipe{executor},
            nullptr);

#ifdef _WIN32
        const auto nodeShell = MSYS2_BASH;
        const auto nodeCommandArgs = std::vector<std::string>{
            "--login",
            "-i",
            "-c",
            "\"cd "s + isolateDirectory.path().generic_string() + " && node ./main.mjs --log-file=./log.txt\"",
        };
#else
        const auto nodeShell = "/bin/bash";
        const auto nodeCommandArgs = std::vector<std::string>{
            "-c",
            "cd "s + isolateDirectory.path().generic_string() + " && node ./main.mjs --log-file=./log.txt",
        };
#endif

        result->mainModule = std::make_unique<boost::process::v2::process>(
            executor,
            node,
            std::vector<std::string>{"main.mjs", "--log-file=./log.txt"},
            bp2::process_environment{bp2::environment::current()},
            bp2::process_start_dir{isolateDirectory.path().generic_string()},
            bp2::process_stdio{
                .in = result->stdinPipe,
                .out = result->stdoutPipe,
                .err = result->stderrPipe,
            });

        result->timer.async_wait([weak = std::weak_ptr{result}](boost::system::error_code const& ec) {
            if (ec)
                return;
            if (auto result = weak.lock())
            {
                result->killed = true;
                try
                {
                    result->mainModule->terminate();
                }
                catch (std::exception const& e)
                {
                    std::cerr << "Failed to terminate process: " << e.what() << std::endl;
                }
            }
        });

        nlohmann::json portObject;
        try
        {
            std::string buffer(1024, '\0');
            boost::system::error_code ec;
            int readAmount = result->stdoutPipe.read_some(boost::asio::buffer(buffer), ec);
            if (ec)
            {
                std::cerr << "Failed to read port: " << ec.message() << std::endl;
                return result;
            }
            if (readAmount == 0)
            {
                std::cerr << "Failed to read port: No data" << std::endl;
                return result;
            }
            portObject = nlohmann::json::parse(buffer);
            result->port = portObject["port"].get<unsigned short>();
        }
        catch (std::exception const& e)
        {
            std::cerr << "Failed to parse port object: " << e.what() << std::endl;
            return result;
        }

        return result;
    }
}