#pragma once

#include <frontend/terminal/terminal_engine.hpp>
#include <frontend/terminal/ssh_channel.hpp>
#include <roar/detail/pimpl_special_functions.hpp>
#include <persistence/state/terminal_engine.hpp>
#include <frontend/terminal/ssh_channel.hpp>
#include <nui/utility/move_detector.hpp>
#include <ids/id.hpp>

#include <memory>
#include <string>

// TODO: https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/

class SshTerminalEngine : public MultiChannelTerminalEngine
{
  public:
    struct Settings
    {
        Persistence::SshTerminalEngine engineOptions;
        std::function<void()> onExit;
        std::function<void()> onBeforeExit = {};
    };

  public:
    SshTerminalEngine(Settings settings);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(SshTerminalEngine);

    void open(std::function<void(bool, std::string const&)> onOpen, bool fileMode = false) override;

    void createChannel(
        std::function<void(std::string const&)> handler,
        std::function<void(std::string const&)> errorHandler,
        std::function<void(std::optional<Ids::ChannelId> const&)> onCreated) override;

    void closeChannel(Ids::ChannelId const& channelId, std::function<void()> onChannelClosed = []() {}) override;
    SshChannel* channel(Ids::ChannelId const& channelId) override;
    std::string stealChannelTerminal(Ids::ChannelId const& channelId);

    void dispose(std::function<void()> onDisposeComplete) override;
    std::string engineName() const override
    {
        return "ssh";
    }

    Ids::SessionId sshSessionId() const;

  private:
    void disconnect(std::function<void()> onDisconnect, bool fromDtor = false);
    // Usually indicates that the entire session is closed
    void onChannelDeath();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
    Nui::MoveDetector moveDetector_;
};