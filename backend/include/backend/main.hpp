#pragma once

#include "ssh/ssh_session_manager.hpp"
#include <backend/process/process_store.hpp>
#include <backend/ssh/ssh_session_manager.hpp>
#include <persistence/state_holder.hpp>
#include <backend/password/password_prompter.hpp>

#include <boost/asio/steady_timer.hpp>
#include <nui/core.hpp>
#include <nui/rpc.hpp>
#include <nui/window.hpp>
#include <roar/mime_type.hpp>
#include <efsw/efsw.hpp>

#include <filesystem>
#include <atomic>

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
    void startChildSignalTimer();

  private:
    std::filesystem::path programDir_;
    Persistence::StateHolder stateHolder_;
    Nui::Window window_;
    Nui::RpcHub hub_;
    ProcessStore processes_;
    PasswordPrompter prompter_;
    SshSessionManager sshSessionManager_;
    std::atomic_bool shuttingDown_;
    boost::asio::steady_timer childSignalTimer_;
};