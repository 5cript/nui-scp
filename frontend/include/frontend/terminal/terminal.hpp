#pragma once

#include <frontend/terminal/terminal_engine.hpp>
#include <persistence/state/terminal_options.hpp>
#include <ids/ids.hpp>

#include <nui/frontend/val.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

class TerminalChannel
{
  public:
    TerminalChannel(MultiChannelTerminalEngine* engine, Ids::ChannelId channelId);

    void open(
        Nui::val host,
        Persistence::TerminalOptions const& options,
        std::function<void(bool, std::string const&)> onOpen);
    bool isOpen() const;
    void write(std::string const& data, bool isUserInput);
    void writeStderr(std::string const& data, bool isUserInput);
    void focus();
    void dispose();
    std::string stealTerminal();
    ROAR_PIMPL_SPECIAL_FUNCTIONS(TerminalChannel);

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};

class Terminal
{
  public:
    Terminal(std::unique_ptr<TerminalEngine> engine, bool isMultiChannel);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(Terminal);

    void open(std::function<void(bool, std::string const&)> onOpen);

    void createChannel(
        Nui::val host,
        Persistence::TerminalOptions const& options,
        std::function<void(std::optional<Ids::ChannelId> /*channelId*/, std::string const& info)> onChannelCreated);
    TerminalChannel* channel(Ids::ChannelId const& channelId);
    void closeChannel(Ids::ChannelId const& channelId);

    void
    iterateAllChannels(std::function<bool(Ids::ChannelId const& channelId, TerminalChannel& channel)> const& handler);

    void dispose();
    TerminalEngine& engine();

    // Focusses the first terminal channel if it exists
    void focus();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};