#include <persistence/state_holder.hpp>
#include <persistence/state.hpp>
#include <constants/persistence.hpp>
#include <log/log.hpp>

#include <fmt/chrono.h>
#include <roar/filesystem/special_paths.hpp>

#include <filesystem>
#include <fstream>
#include <chrono>

namespace Persistence
{
    namespace
    {
        void setupPersistence()
        {
            const auto path = Roar::resolvePath(Constants::persistencePath);
            const auto parentPath = std::filesystem::path{path}.parent_path().string();

            if (!std::filesystem::exists(parentPath))
                std::filesystem::create_directories(parentPath);

            if (!std::filesystem::exists(path))
            {
                std::ofstream writer{path, std::ios_base::binary};
                writer << nlohmann::json(State{}).dump(4);
            }
        }
    }

    void StateHolder::load(std::function<void(bool, StateHolder&)> const& onLoad)
    {
        setupPersistence();
        const auto path = Roar::resolvePath(Constants::persistencePath);

        auto makeBackup = [&path]() {
            const auto backupFileName = [&path]() {
                const auto now = std::chrono::system_clock::now();
                const auto time = fmt::format("{:%Y-%m-%d_%H-%M-%S}", now);

                return path.parent_path() / (path.filename().string() + ".backup_" + time);
            }();

            {
                std::ifstream reader{path, std::ios_base::binary};
                std::ofstream writer{backupFileName, std::ios_base::binary};

                writer << reader.rdbuf();
            }
            Log::info("Copied config file to backup: {}", backupFileName.string());
        };

        try
        {
            const auto before = [this, &path, &makeBackup]() {
                try
                {
                    std::ifstream reader{path, std::ios_base::binary};
                    if (!reader.good())
                        return nlohmann::json(nullptr);
                    return nlohmann::json::parse(reader);
                }
                catch (std::exception const& e)
                {
                    Log::error("Failed to parse config file: {}", e.what());
                    makeBackup();
                    save();
                    return nlohmann::json(nullptr);
                }
            }();

            if (before.is_null())
            {
                onLoad(true, *this);
                return;
            }

            before.get_to(stateCache_);
            const auto after = nlohmann::json(stateCache_);

            if (!nlohmann::json::diff(before, after).empty())
            {
                Log::warn("Config file misses some defaults, writing them back to disk.");
                makeBackup();
                save();
            }

            onLoad(true, *this);
        }
        catch (std::exception const& e)
        {
            Log::error("Failed to load config file: {}", e.what());
            onLoad(false, *this);
        }
    }
    void StateHolder::save(std::function<void()> const& onSaveComplete)
    {
        setupPersistence();
        const auto path = Roar::resolvePath(Constants::persistencePath);

        try
        {
            std::ofstream writer{path, std::ios_base::binary};
            writer << nlohmann::json(stateCache_).dump(4);
            onSaveComplete();
        }
        catch (std::exception const& e)
        {
            Log::error("Failed to save config file: {}", e.what());
            throw;
        }
    }

    void StateHolder::registerRpc(Nui::RpcHub& hub)
    {
        hub.registerFunction("StateHolder::load", [&hub, this](std::string responseId) {
            Log::debug("Received state load request from frontend state holder.");

            load([responseId, &hub](bool success, StateHolder& holder) {
                if (!success)
                {
                    hub.callRemote(
                        responseId,
                        nlohmann::json{
                            {"error", "Failed to load state from disk."},
                        });
                    return;
                }
                Log::debug("State loaded from disk.");
                hub.callRemote(responseId, nlohmann::json(holder.stateCache_).dump());
            });
        });

        hub.registerFunction("StateHolder::save", [&hub, this](std::string responseId, std::string const& state) {
            Log::debug("Received state save request from frontend state holder.");

            try
            {
                stateCache_ = nlohmann::json::parse(state).get<State>();
                save([&hub, responseId]() {
                    Log::debug("State saved to disk.");
                    hub.callRemote(responseId, nlohmann::json{{"success", true}});
                });
            }
            catch (std::exception const& e)
            {
                hub.callRemote(
                    responseId,
                    nlohmann::json{
                        {"error", fmt::format("Failed to save state to disk: {}", e.what())},
                    });
            }
        });
    }

    void StateHolder::initializeOsDefaults()
    {
        load([this](bool success, StateHolder&) {
            if (!success)
                return;

            if (stateCache_.terminalEngines.empty())
            {
#ifdef _WIN32
                stateCache_.terminalEngines.push_back({
                    .type = "msys2",
                    .name = "msys2 default",
                    .engine = defaultMsys2TerminalEngine(),
                });
#elif __APPLE__
// nothing
#else
                stateCache_.terminalEngines.push_back({
                    .type = "shell",
                    .name = "bash default",
                    .engine = defaultBashTerminalEngine(),
                });
#endif
                save();
            }
        });
    }
}