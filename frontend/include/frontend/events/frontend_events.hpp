#pragma once

#include <events/app_event_context.hpp>
#include <events/app_wide_events.hpp>

struct FrontendEvents : public AppWideEvents
{
    Nui::Observed<std::string> onNewSession;
    Nui::Observed<bool> onLayoutsChanged;

    FrontendEvents()
        : onNewSession{}
        , onLayoutsChanged{false}
    {}
};