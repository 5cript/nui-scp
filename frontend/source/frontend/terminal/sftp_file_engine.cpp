#include <frontend/terminal/sftp_file_engine.hpp>
#include <log/log.hpp>

#include <nui/rpc.hpp>
#include <nui/frontend/api/json.hpp>

struct SftpFileEngine::Implementation
{
    bool wasDisposed = false;
    SshTerminalEngine* engine;
    std::optional<Ids::ChannelId> sftpChannelId{std::nullopt};

    Implementation(SshTerminalEngine* engine)
        : engine{engine}
    {}
};

SftpFileEngine::SftpFileEngine(SshTerminalEngine* engine)
    : impl_{std::make_unique<Implementation>(engine)}
{}
SftpFileEngine::~SftpFileEngine()
{
    if (!moveDetector_.wasMoved())
    {
        dispose();
    }
}

std::optional<Ids::ChannelId> SftpFileEngine::release()
{
    return std::move(impl_->sftpChannelId);
}

void SftpFileEngine::dispose()
{
    if (!impl_->wasDisposed)
    {
        if (impl_->sftpChannelId)
        {
            Log::info("Closing sftp channel");
            impl_->engine->closeChannel(impl_->sftpChannelId.value(), []() {});
        }
    }
    impl_->wasDisposed = true;
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(SftpFileEngine);

void SftpFileEngine::lazyOpen(std::function<void(std::optional<Ids::ChannelId> const&)> const& onOpen)
{
    if (impl_->sftpChannelId)
    {
        onOpen(impl_->sftpChannelId);
        return;
    }

    Log::info("Creating sftp channel");
    impl_->engine->createSftpChannel([this, onOpen](auto const& id) {
        impl_->sftpChannelId = id;
        onOpen(id);
    });
}

void SftpFileEngine::listDirectory(
    std::filesystem::path const& path,
    std::function<void(std::optional<std::vector<SharedData::DirectoryEntry>> const&)> onComplete)
{
    lazyOpen([this, path, onComplete = std::move(onComplete)](auto const& channelId) {
        if (!channelId)
        {
            Log::error("Cannot list directory, no sftp channel");
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
            impl_->engine->sshSessionId().value(),
            channelId.value().value(),
            path.generic_string());
    });
}

void SftpFileEngine::createDirectory(std::filesystem::path const& path, std::function<void(bool)> onComplete)
{
    lazyOpen([this, path, onComplete = std::move(onComplete)](auto const& channelId) {
        if (!channelId)
        {
            Log::error("Cannot create directory, no channel");
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
            impl_->engine->sshSessionId().value(),
            channelId.value().value(),
            path.generic_string());
    });
}

void SftpFileEngine::createFile(std::filesystem::path const& path, std::function<void(bool)> onComplete)
{
    lazyOpen([this, path, onComplete = std::move(onComplete)](auto const& channelId) {
        if (!channelId)
        {
            Log::error("Cannot create file, no channel");
            return;
        }

        Log::info("Creating file: {}", path.generic_string());
        Nui::RpcClient::callWithBackChannel(
            "SshSessionManager::sftp::createFile",
            [onComplete = std::move(onComplete)](Nui::val val) {
                Nui::Console::log(val);

                if (val.hasOwnProperty("error"))
                {
                    Log::error("(Frontend) Failed to create file: {}", val["error"].as<std::string>());
                    onComplete(false);
                    return;
                }

                onComplete(true);
            },
            impl_->engine->sshSessionId().value(),
            channelId.value().value(),
            path.generic_string());
    });
}