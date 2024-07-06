#pragma once

#include <nui/core.hpp>
#include <nui/rpc.hpp>

#include <roar/detail/pimpl_special_functions.hpp>

#include <persistence/state.hpp>

#include <functional>
#include <memory>

namespace Persistence
{
    class StateHolder
    {
      public:
        StateHolder();
        ROAR_PIMPL_SPECIAL_FUNCTIONS(StateHolder);

        void load(std::function<void(std::optional<State> const&)> const& onLoad);
        void save(State const& state, std::function<void()> const& onSaveComplete = []() {});

#ifdef NUI_BACKEND
        void registerRpc(Nui::RpcHub& rpcHub);
#endif

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
} // namespace Persistence