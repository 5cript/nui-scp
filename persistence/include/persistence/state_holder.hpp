#pragma once

#include <nui/core.hpp>
#include <nui/rpc.hpp>

#include <roar/detail/pimpl_special_functions.hpp>

#include <persistence/state/state.hpp>

#include <functional>

namespace Persistence
{
    class StateHolder
    {
      public:
        StateHolder();
        ROAR_PIMPL_SPECIAL_FUNCTIONS(StateHolder);

        void load(std::function<void(bool, StateHolder&)> const& onLoad);
        void save(std::function<void()> const& onSaveComplete = []() {});

        State& stateCache();

#ifdef NUI_BACKEND
        void registerRpc(Nui::RpcHub& rpcHub);
        void dataFixer(nlohmann::json const& before);
#endif

      private:
        State stateCache_;
    };
} // namespace Persistence