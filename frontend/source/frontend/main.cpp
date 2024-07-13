#include <frontend/main_page.hpp>
#include <persistence/state_holder.hpp>

#include <nui/core.hpp>
#include <nui/window.hpp>
#include <nui/frontend/api/console.hpp>

// #define NUI_ENABLE_LIVE_RELOAD
// #include <nui/frontend/live_styling.hpp>
#include <nui/frontend/dom/dom.hpp>

#include <memory>

static std::unique_ptr<Persistence::StateHolder> persistence{};
static std::unique_ptr<FrontendEvents> frontendEvents{};
static std::unique_ptr<MainPage> mainPage{};
static std::unique_ptr<Nui::Dom::Dom> dom{};

extern "C" void frontendMain()
{
    persistence = std::make_unique<Persistence::StateHolder>();
    frontendEvents = std::make_unique<FrontendEvents>();
    mainPage = std::make_unique<MainPage>(persistence.get(), frontendEvents.get());
    dom = std::make_unique<Nui::Dom::Dom>();
    dom->setBody(mainPage->render());

    // Nui::registerLiveReloadListener();
}

EMSCRIPTEN_BINDINGS(nui_example_frontend)
{
    emscripten::function("main", &frontendMain);
}
#include <nui/frontend/bindings.hpp>