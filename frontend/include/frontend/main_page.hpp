#pragma once

#include <nui/frontend/element_renderer.hpp>

#include <roar/detail/pimpl_special_functions.hpp>

class MainPage
{
  public:
    MainPage();
    ROAR_PIMPL_SPECIAL_FUNCTIONS(MainPage);

    Nui::ElementRenderer render();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};