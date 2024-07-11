#pragma once

#include <persistence/state_holder.hpp>
#include <persistence/state/terminal_engine.hpp>
#include <persistence/state/termios.hpp>
#include <persistence/state/common_terminal_options.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/utility/stabilize.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

class Session
{
  public:
    Session(
        Persistence::StateHolder* stateHolder,
        Persistence::TerminalEngine engine,
        Persistence::Termios termios,
        Persistence::CommonTerminalOptions options);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(Session);

    Nui::ElementRenderer operator()();

    std::string name() const;

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};