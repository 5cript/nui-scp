#pragma once

#include <nui/backend/rpc_hub.hpp>
#include <nui/utility/scope_exit.hpp>

#include <filesystem>

class WatchContext
{
  public:
    WatchContext(Nui::RpcHub& hub, std::filesystem::path const& programDir, std::filesystem::path const& sourceDir);
    ~WatchContext();
    WatchContext(WatchContext const&) = delete;
    WatchContext& operator=(WatchContext const&) = delete;
    WatchContext(WatchContext&&) = delete;
    WatchContext& operator=(WatchContext&&) = delete;

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};

std::unique_ptr<WatchContext>
setupLiveWatch(Nui::RpcHub& hub, std::filesystem::path const& programDir, std::filesystem::path const& sourceDir);