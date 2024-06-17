#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

class Sidebar
{
  public:
    Sidebar();
    ROAR_PIMPL_SPECIAL_FUNCTIONS(Sidebar);

    Nui::ElementRenderer operator()();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};