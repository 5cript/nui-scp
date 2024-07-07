#pragma once

#include <persistence/state_holder.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

class Toolbar
{
  public:
    Toolbar(Persistence::StateHolder* stateHolder);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(Toolbar);

    Nui::ElementRenderer operator()();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};