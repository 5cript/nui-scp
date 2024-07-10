#include <frontend/main_page.hpp>
#include <frontend/sidebar.hpp>
#include <frontend/tailwind.hpp>
#include <frontend/toolbar.hpp>
#include <frontend/session_area.hpp>
#include <nui/frontend/api/timer.hpp>
#include <log/log.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

struct MainPage::Implementation
{
    Persistence::StateHolder* stateHolder;
    Sidebar sidebar;
    Toolbar toolbar;
    SessionArea sessionArea;
    Nui::Observed<bool> darkMode;
    Nui::TimerHandle setupWait;

    Implementation(Persistence::StateHolder* stateHolder)
        : stateHolder{stateHolder}
        , sidebar{stateHolder}
        , toolbar{stateHolder}
        , sessionArea{stateHolder}
        , darkMode{true}
        , setupWait{}
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
    Log::setupFrontendLogger(
        [](std::chrono::system_clock::time_point const&, Log::Level, std::string const&) {

        },
        [this, once = false](Log::Level) mutable {
            Nui::Console::info("Log available");
            if (!once)
            {
                once = true;

                Nui::setInterval(
                    200,
                    [this]() {
                        if (!Nui::val::global("terminalUtility").isUndefined() &&
                            !Nui::val::global("nanoid").isUndefined())
                        {
                            if (impl_->setupWait.hasActiveTimer())
                                impl_->setupWait.stop();
                            onSetupComplete();
                        }
                        else
                        {
                            Log::info("Waiting for terminalUtility and nanoid to be available.");
                        }
                    },
                    [this](Nui::TimerHandle&& t) {
                        impl_->setupWait = std::move(t);
                    });
            }
        });

    Nui::Console::log("MainPage::MainPage()");
}

void MainPage::onSetupComplete()
{
    Log::info("Setup is complete.");

#ifdef _WIN32
    impl_->sessionArea.addSession("msys2_default");
#else
    impl_->sessionArea.addSession("bash_default");
    impl_->sessionArea.addSession("bash_default2");
#endif
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(MainPage);

Nui::ElementRenderer MainPage::render()
{
    using namespace Nui;
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div; // because of the global div.

    Nui::Console::log("MainPage::render()");

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
        impl_->sessionArea()
    );
    // clang-format on
}