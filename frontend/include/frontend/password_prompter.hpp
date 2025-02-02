#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

#include <memory>

class PasswordPrompter
{
  public:
    PasswordPrompter();
    ROAR_PIMPL_SPECIAL_FUNCTIONS(PasswordPrompter);

    Nui::ElementRenderer dialog();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};