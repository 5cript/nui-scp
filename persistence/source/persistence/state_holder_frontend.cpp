#include <persistence/state_holder.hpp>

#include <nui/frontend/api/console.hpp>

namespace Persistence
{
    void StateHolder::load(std::function<void(bool, StateHolder&)> const& onLoad)
    {
        Nui::RpcClient::getRemoteCallableWithBackChannel(
            "StateHolder::load", [this, onLoad](Nui::val const& jsonStringOrError) {
                Nui::Console::log(jsonStringOrError);
                if (jsonStringOrError.hasOwnProperty("error"))
                {
                    onLoad(false, *this);
                    return;
                }

                try
                {
                    nlohmann::json::parse(jsonStringOrError.as<std::string>()).get_to(stateCache_);
                }
                catch (std::exception const& e)
                {
                    Nui::Console::log("Failed to parse state from json: {}", e.what());
                    onLoad(false, *this);
                    return;
                }
                onLoad(true, *this);
            })();
    }
    void StateHolder::save(std::function<void()> const& onSaveComplete)
    {
        Nui::RpcClient::getRemoteCallableWithBackChannel("StateHolder::save", [onSaveComplete](Nui::val const&) {
            onSaveComplete();
        })(nlohmann::json(stateCache_).dump());
    }
}