#include <persistence/state_holder.hpp>

namespace Persistence
{
    void StateHolder::load(std::function<void(bool, StateHolder&)> const& onLoad)
    {
        Nui::RpcClient::getRemoteCallableWithBackChannel(
            "StateHolder::load", [this, onLoad](Nui::val const& jsonStringOrError) {
                if (jsonStringOrError.hasOwnProperty("error"))
                {
                    onLoad(false, *this);
                    return;
                }

                nlohmann::json::parse(jsonStringOrError.as<std::string>()).get_to(stateCache_);
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