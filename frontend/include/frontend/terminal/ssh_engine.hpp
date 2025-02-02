#pragma once

#include <frontend/terminal/terminal_engine.hpp>
#include <roar/detail/pimpl_special_functions.hpp>
#include <persistence/state/terminal_engine.hpp>
#include <nui/utility/move_detector.hpp>

#include <memory>
#include <string>

// TODO: https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/

class SshTerminalEngine : public TerminalEngine
{
  public:
    struct Settings
    {
        Persistence::SshTerminalEngine engineOptions;
        std::function<void()> onExit;
    };

  public:
    SshTerminalEngine(Settings settings);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(SshTerminalEngine);

    void open(std::function<void(bool, std::string const&)> onOpen, bool fileMode = false) override;
    void dispose() override;
    void write(std::string const& data) override;
    void resize(int cols, int rows) override;

    void setStdoutHandler(std::function<void(std::string const&)> handler) override;
    void setStderrHandler(std::function<void(std::string const&)> handler) override;

    std::string sshSessionId() const;

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
    Nui::MoveDetector moveDetector_;
};