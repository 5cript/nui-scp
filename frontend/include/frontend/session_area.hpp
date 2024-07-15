#pragma once

#include <frontend/session.hpp>
#include <frontend/events/frontend_events.hpp>
#include <persistence/state_holder.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

class SessionArea
{
  public:
    SessionArea(Persistence::StateHolder* stateHolder, FrontendEvents* events);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(SessionArea);

    Nui::ElementRenderer operator()();

    void addSession(std::string const& name);
    void registerRpc();
    void removeSession(std::function<bool(Session const&)> const& predicate);

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};