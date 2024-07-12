#pragma once

#include <frontend/terminal/terminal_engine.hpp>
#include <persistence/state/terminal_options.hpp>

#include <nui/frontend/val.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

class Terminal
{
  public:
    Terminal(std::unique_ptr<TerminalEngine> engine);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(Terminal);

    bool isOpen() const;
    void open(Nui::val element, Persistence::TerminalOptions const& options);
    void dispose();
    void write(std::string const& data, bool isUserInput);

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};