#include <persistence/state_holder.hpp>

namespace Persistence
{
    void StateHolder::load(std::function<void(std::optional<State> const&)> const& onLoad)
    {
        Nui::RpcClient::getRemoteCallableWithBackChannel(
            "StateHolder::load", [onLoad](Nui::val const& jsonStringOrError) {
                if (jsonStringOrError.hasOwnProperty("error"))
                {
                    onLoad(std::nullopt);
                    return;
                }

                State state;
                nlohmann::json::parse(jsonStringOrError.as<std::string>()).get_to(state);
                onLoad(std::move(state));
            })();
    }
    void StateHolder::save(State const& state, std::function<void()> const& onSaveComplete)
    {
        Nui::RpcClient::getRemoteCallableWithBackChannel("StateHolder::save", [onSaveComplete](Nui::val const&) {
            onSaveComplete();
        })(nlohmann::json(state).dump());
    }
}