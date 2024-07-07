#pragma once

#include <frontend/terminal/terminal_engine.hpp>
#include <roar/detail/pimpl_special_functions.hpp>
#include <persistence/state/terminal_engine.hpp>

#include <memory>
#include <string>
#include <string_view>

// TODO: https://devblogs.microsoft.com/commandline/windows-command-line-introducing-the-windows-pseudo-console-conpty/

class Msys2Terminal : public TerminalEngine
{
  public:
    struct Settings
    {
        Persistence::ExecutingTerminalEngine engineOptions;
    };

  public:
    Msys2Terminal(Settings settings);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(Msys2Terminal);

    void open(std::function<void(bool)> onOpen) override;
    void dispose() override;
    void write(std::string const& data) override;
    void resize(int cols, int rows) override;

    void setStdoutHandler(std::function<void(std::string const&)> handler) override;
    void setStderrHandler(std::function<void(std::string const&)> handler) override;

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};