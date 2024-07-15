#include <frontend/password_prompter.hpp>
#include <log/log.hpp>

#include <nui/rpc.hpp>
#include <ui5/components/dialog.hpp>
#include <ui5/components/button.hpp>
#include <ui5/components/input.hpp>
#include <ui5/components/label.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/dom/basic_element.hpp>

struct PasswordPrompter::Implementation
{
    std::weak_ptr<Nui::Dom::BasicElement> dialog;
    std::weak_ptr<Nui::Dom::BasicElement> input;
};

PasswordPrompter::PasswordPrompter()
    : impl_{std::make_unique<Implementation>()}
{
    Nui::RpcClient::registerFunction(
        "PasswordPrompter::prompt", [this](std::string const& whatFor, std::string const& prompt) {
            if (auto diag = impl_->dialog.lock(); diag)
            {
                diag->val().set("header-text", whatFor + ": " + prompt);
                diag->val().set("open", true);
            }
        });
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(PasswordPrompter);

void PasswordPrompter::closeDialog(std::string const& password)
{
    Log::info("PasswordPrompter::closeDialog");
    if (auto diag = impl_->dialog.lock(); diag)
    {
        Log::info("Closing dialog");
        diag->val().set("open", false);
    }
    Nui::RpcClient::call("PasswordPrompter::promptDone", password);
}

void PasswordPrompter::confirm()
{
    if (auto input = impl_->input.lock())
    {
        closeDialog(input->val()["value"].as<std::string>());
        input->val().set("value", "");
    }
    else
        closeDialog("");
}

Nui::ElementRenderer PasswordPrompter::dialog()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;

    // clang-format off
    return ui5::dialog{
        id = "PasswordPrompter", 
        "headerText"_prop = "Enter Password",
        reference = impl_->dialog,
    }(
        section{class_ = "prompter-form"}(
            div{}(
                ui5::label{id = "PasswordPrompterLabel", "for"_prop = "PasswordPrompterInput"}("Password:"),
                ui5::input{
                    id = "PasswordPrompterInput", 
                    "type"_prop = "Password",
                    reference = impl_->input,
                    "change"_event = [this](Nui::val) {
                        confirm();
                    }
                }()
            )
        ),
        div{
            "slot"_prop = "footer",
            style="display: flex; justify-content: flex-end; width: 100%; align-items: center"
        }(
            div{style = "flex: 1;"}(),
            ui5::button{
                "design"_prop = "Emphasized",
                "click"_event = [this](){
                    closeDialog("");
                }
            }("Cancel"),
            ui5::button{
                "design"_prop = "Emphasized",
                "click"_event = [this](){
                    confirm();
                }
            }("Ok")
        )
    );
    // clang-format on
}
