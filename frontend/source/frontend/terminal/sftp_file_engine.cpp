#include <frontend/terminal/sftp_file_engine.hpp>
#include <log/log.hpp>

#include <nui/rpc.hpp>
#include <nui/frontend/api/json.hpp>

struct SftpFileEngine::Implementation
{
    SshTerminalEngine underlyingEngine;

    Implementation(SshTerminalEngine::Settings settings)
        : underlyingEngine{std::move(settings)}
    {}
};

SftpFileEngine::SftpFileEngine(SshTerminalEngine::Settings settings)
    : impl_{std::make_unique<Implementation>(std::move(settings))}
{}
SftpFileEngine::~SftpFileEngine()
{
    if (!moveDetector_.wasMoved())
    {
        dispose();
    }
}

void SftpFileEngine::dispose()
{
    if (impl_->underlyingEngine.sshSessionId().empty())
    {
        return;
    }
    impl_->underlyingEngine.dispose();
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(SftpFileEngine);

void SftpFileEngine::lazyOpen(std::function<void(std::optional<std::string> const&)> const& onOpen)
{
    if (!impl_->underlyingEngine.sshSessionId().empty())
    {
        onOpen(impl_->underlyingEngine.sshSessionId());
        return;
    }

    impl_->underlyingEngine.open(
        [this, onOpen](bool success, std::string const& error) {
            if (!success)
            {
                Log::error("Failed to open SFTP: {}", error);
                onOpen(std::nullopt);
                return;
            }

            onOpen(impl_->underlyingEngine.sshSessionId());
        },
        true);
}

void SftpFileEngine::listDirectory(
    std::filesystem::path const& path,
    std::function<void(std::optional<std::vector<SharedData::DirectoryEntry>> const&)> onComplete)
{
    lazyOpen([path, onComplete = std::move(onComplete)](auto const& sshSessionId) {
        if (!sshSessionId)
        {
            Log::error("Cannot list directory, no ssh session");
            return;
        }

        Log::info("Listing directory: {}", path.generic_string());
        Nui::RpcClient::callWithBackChannel(
            "SshSessionManager::sftp::listDirectory",
            [onComplete = std::move(onComplete)](Nui::val val) {
                Nui::Console::log(val);

                if (val.hasOwnProperty("error") || !val.hasOwnProperty("entries"))
                {
                    if (val.hasOwnProperty("error"))
                    {
                        Log::error("(Frontend) Failed to list directory: {}", val["error"].as<std::string>());
                    }
                    else
                    {
                        Log::error("(Frontend) Failed to list directory: no entries");
                    }
                    onComplete(std::nullopt);
                    return;
                }

                onComplete(nlohmann::json::parse(Nui::JSON::stringify(val))["entries"]);
            },
            *sshSessionId,
            path.generic_string());
    });
}

void SftpFileEngine::createDirectory(std::filesystem::path const& path, std::function<void(bool)> onComplete)
{
    lazyOpen([path, onComplete = std::move(onComplete)](auto const& sshSessionId) {
        if (!sshSessionId)
        {
            Log::error("Cannot create directory, no ssh session");
            return;
        }

        Log::info("Creating directory: {}", path.generic_string());
        Nui::RpcClient::callWithBackChannel(
            "SshSessionManager::sftp::createDirectory",
            [onComplete = std::move(onComplete)](Nui::val val) {
                Nui::Console::log(val);

                if (val.hasOwnProperty("error"))
                {
                    Log::error("(Frontend) Failed to create directory: {}", val["error"].as<std::string>());
                    onComplete(false);
                    return;
                }

                onComplete(true);
            },
            *sshSessionId,
            path.generic_string());
    });
}