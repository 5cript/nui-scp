#include <persistence/state_holder.hpp>
#include <persistence/state.hpp>
#include <constants/persistence.hpp>
#include <log/log.hpp>

#include <fmt/chrono.h>
#include <roar/filesystem/special_paths.hpp>

#include <filesystem>
#include <fstream>

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

    void StateHolder::load(std::function<void(std::optional<State> const&)> const& onLoad)
    {
        setupPersistence();
        const auto path = Roar::resolvePath(Constants::persistencePath);

        auto onLoadFailure = [this, &path]() {
            // copy config to backup:
            const auto backupFileName = [&path]() {
                const auto now = std::chrono::system_clock::now();
                const auto time = fmt::format("{:%Y-%m-%d_%H-%M-%S}", now);

                return path.parent_path() / (path.filename().string() + ".backup_" + time);
            }();
            std::filesystem::copy_file(path, backupFileName);

            Log::info("Copied config file to backup: {}", backupFileName.string());
            save({});
        };

        try
        {
            const auto before = [&path, &onLoadFailure]() {
                std::ifstream reader{path, std::ios_base::binary};
                if (!reader.good())
                {
                    onLoadFailure();
                    return nlohmann::json(nullptr);
                }

                return nlohmann::json::parse(reader);
            }();
            if (before.is_null())
                return onLoad(std::nullopt);

            State state;
            before.get_to(state);
            const auto after = nlohmann::json(state);

            if (!nlohmann::json::diff(before, after).empty())
            {
                Log::warn("Config file misses some defaults, writing them back to disk.");
                save(state);
            }

            onLoad(state);
        }
        catch (std::exception const& e)
        {
            Log::error("Failed to load config file: {}", e.what());
            onLoad(std::nullopt);
        }
    }
    void StateHolder::save(State const& state, std::function<void()> const& onSaveComplete)
    {
        setupPersistence();
        const auto path = Roar::resolvePath(Constants::persistencePath);

        try
        {
            std::ofstream writer{path, std::ios_base::binary};
            writer << nlohmann::json(state).dump(4);
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

            load([responseId, &hub](std::optional<State> const& state) {
                if (!state)
                {
                    hub.callRemote(
                        responseId,
                        nlohmann::json{
                            {"error", "Failed to load state from disk."},
                        });
                    return;
                }
                Log::debug("State loaded from disk.");
                hub.callRemote(responseId, nlohmann::json(*state).dump());
            });
        });

        hub.registerFunction("StateHolder::save", [&hub, this](std::string responseId, std::string const& state) {
            Log::debug("Received state save request from frontend state holder.");

            try
            {
                save(nlohmann::json::parse(state).get<State>(), [&hub, responseId]() {
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
}