#include <frontend/main_page.hpp>
#include <frontend/sidebar.hpp>
#include <frontend/tailwind.hpp>
#include <frontend/toolbar.hpp>
#include <frontend/session_area.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct MainPage::Implementation
{
    Persistence::StateHolder* stateHolder;
    Sidebar sidebar;
    Toolbar toolbar;
    SessionArea session_area;
    Nui::Observed<bool> darkMode;

    Implementation(Persistence::StateHolder* stateHolder)
        : stateHolder{stateHolder}
        , sidebar{stateHolder}
        , toolbar{stateHolder}
        , session_area{stateHolder}
        , darkMode{true}
    {
        Nui::Console::log("MainPage::Implementation()");
    }

    ~Implementation()
    {
        Nui::Console::log("MainPage::Implementation() ~");
    }
};

MainPage::MainPage(Persistence::StateHolder* stateHolder)
    : impl_{std::make_unique<Implementation>(stateHolder)}
{
    Nui::Console::log("MainPage::MainPage()");
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(MainPage);

Nui::ElementRenderer MainPage::render()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    // clang-format off
    return body{
        class_ = observe(impl_->darkMode).generate([this]() {
            std::string res = classes(defaultBgText,
                "w-full h-screen grid grid-cols-[min-content_auto]",
                "grid-rows-[min-content_auto] [grid-template-areas:'Toolbar_Toolbar''Sidebar_SessionArea']"
            );
            if (impl_->darkMode.value())
                res += " dark";
            return res;
        })}
    (
        impl_->toolbar(),
        impl_->sidebar(),
        impl_->session_area()
    );
    // clang-format on
}