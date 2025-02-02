#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

#include <memory>

class InputDialog
{
  public:
    InputDialog(std::string id);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(InputDialog);

    Nui::ElementRenderer operator()();
    void open(
        std::string const& whatFor,
        std::string const& prompt,
        std::string const& headerText,
        bool isPassword,
        std::function<void(std::optional<std::string> const&)> const& onConfirm);

  private:
    void closeDialog(std::optional<std::string> const& password);
    void confirm();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};