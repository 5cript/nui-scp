#include <frontend/dialog/confirm_dialog.hpp>
#include <frontend/components/ui5-text.hpp>
#include <log/log.hpp>

#include <ui5/components/dialog.hpp>
#include <ui5/components/button.hpp>

#include <nui/rpc.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/dom/basic_element.hpp>

struct ConfirmDialog::Implementation
{
    std::string id;
    std::weak_ptr<Nui::Dom::BasicElement> dialog;
    std::function<void(Button)> onClose;
    Nui::Observed<State> state;
    Nui::Observed<std::string> headerText;
    Nui::Observed<std::string> text;
    Nui::Observed<Button> buttons;

    Implementation(std::string id)
        : id{std::move(id)}
        , dialog{}
        , onClose{}
        , state{}
        , headerText{}
        , text{}
    {}
};

ConfirmDialog::ConfirmDialog(std::string id)
    : impl_{std::make_unique<Implementation>(id)}
{}

void ConfirmDialog::open(
    State state,
    std::string const& headerText,
    std::string const& text,
    Button buttons,
    std::function<void(Button buttonPressed)> const& onClose)
{
    impl_->onClose = onClose;
    impl_->state = state;
    impl_->headerText = headerText;
    impl_->text = text;
    impl_->buttons = buttons;
    Nui::globalEventContext.executeActiveEventsImmediately();

    if (auto diag = impl_->dialog.lock(); diag)
    {
        diag->val().set("header-text", headerText);
        diag->val().set("open", true);
    }
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(ConfirmDialog);

void ConfirmDialog::close(Button button)
{
    if (auto diag = impl_->dialog.lock(); diag)
    {
        Log::info("Closing dialog");
        diag->val().set("open", false);
    }
    if (impl_->onClose)
        impl_->onClose(button);
}

Nui::ElementRenderer ConfirmDialog::operator()()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;

    // clang-format off
    return ui5::dialog{
        id = "ConfirmDialog_" + impl_->id,
        "state"_prop = observe(impl_->state).generate([this](){
            switch (impl_->state.value())
            {
                case State::Positive: return "Positive";
                case State::Critical: return "Critical";
                case State::Negative: return "Negative";
                case State::Information: return "Information";
                default: return "None";
            }
        }),
        "headerText"_prop = impl_->headerText,
        reference = impl_->dialog,
    }(
        section{}(
            ui5::text{}(impl_->text)
        ),
        div{
            "slot"_prop = "footer",
            style="display: flex; justify-content: flex-end; width: 100%; align-items: center; gap: 10px; padding: 10px;"
        }(
            div{style = "flex: 1;"}(),
            fragment(
                observe(impl_->buttons),
                [this]() -> Nui::ElementRenderer {
                    if (static_cast<unsigned>(impl_->buttons.value()) & static_cast<unsigned>(Button::Cancel))
                    {
                        return ui5::button{
                            "click"_event = [this](Nui::val) {
                                close(Button::Cancel);
                            }
                        }("Cancel");
                    }
                    return Nui::nil();
                }
            ),
            fragment(
                observe(impl_->buttons),
                [this]() -> Nui::ElementRenderer {
                    if (static_cast<unsigned>(impl_->buttons.value()) & static_cast<unsigned>(Button::Ok))
                    {
                        return ui5::button{
                            "click"_event = [this](Nui::val) {
                                close(Button::Ok);
                            }
                        }("Ok");
                    }
                    return Nui::nil();
                }
            ),
            fragment(
                observe(impl_->buttons),
                [this]() -> Nui::ElementRenderer {
                    if (static_cast<unsigned>(impl_->buttons.value()) & static_cast<unsigned>(Button::Yes))
                    {
                        return ui5::button{
                            "click"_event = [this](Nui::val) {
                                close(Button::Yes);
                            }
                        }("Yes");
                    }
                    return Nui::nil();
                }
            ),
            fragment(
                observe(impl_->buttons),
                [this]() -> Nui::ElementRenderer {
                    if (static_cast<unsigned>(impl_->buttons.value()) & static_cast<unsigned>(Button::No))
                    {
                        return ui5::button{
                            "click"_event = [this](Nui::val) {
                                close(Button::No);
                            }
                        }("No");
                    }
                    return Nui::nil();
                }
            )
        )
    );
    // clang-format on
}
