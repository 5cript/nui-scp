#pragma once

#include <backend/process/process_store.hpp>
#include <persistence/state_holder.hpp>

#include <nui/core.hpp>
#include <nui/rpc.hpp>
#include <nui/window.hpp>
#include <roar/mime_type.hpp>
#include <efsw/efsw.hpp>

class Main
{
  public:
    Main(int const argc, char const* const* argv);
    ~Main();

    Main(Main const&) = delete;
    Main& operator=(Main const&) = delete;
    Main(Main&&) = delete;
    Main& operator=(Main&&) = delete;

    void registerRpc();
    void show();

  private:
    std::filesystem::path programDir_;
    Persistence::StateHolder stateHolder_;
    Nui::Window window_;
    Nui::RpcHub hub_;
    ProcessStore processes_;
};