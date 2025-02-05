#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

#include <memory>

class ConfirmDialog
{
  public:
    ConfirmDialog(std::string id);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(ConfirmDialog);

    enum class Button : unsigned
    {
        Unknown = 0b0000'0000,
        Cancel = 0b0000'0001,
        Ok = 0b0000'0010,
        Yes = 0b0000'0100,
        No = 0b0000'1000,
    };

    enum class State
    {
        None,
        Positive,
        Critical,
        Negative,
        Information
    };

    Nui::ElementRenderer operator()();
    void open(
        State state,
        std::string const& headerText,
        std::string const& text,
        Button buttons = Button::Ok,
        std::function<void(Button buttonPressed)> const& onClose = [](auto) {});

  private:
    void close(Button button);

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};