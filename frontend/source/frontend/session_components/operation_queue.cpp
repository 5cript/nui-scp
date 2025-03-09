#include <frontend/session_components/operation_queue.hpp>

#include <nui/frontend/attributes.hpp>
#include <nui/frontend/elements.hpp>
#include <fmt/format.h>

struct OperationQueue::Implementation
{
    Persistence::StateHolder* stateHolder;
    FrontendEvents* events;
    std::string persistenceSessionName;
    std::string id;
    ConfirmDialog* confirmDialog;

    Implementation(
        Persistence::StateHolder* stateHolder,
        FrontendEvents* events,
        std::string persistenceSessionName,
        std::string id,
        ConfirmDialog* confirmDialog)
        : stateHolder{stateHolder}
        , events{events}
        , persistenceSessionName{std::move(persistenceSessionName)}
        , id{std::move(id)}
        , confirmDialog{confirmDialog}
    {}
};

OperationQueue::OperationQueue(
    Persistence::StateHolder* stateHolder,
    FrontendEvents* events,
    std::string persistenceSessionName,
    std::string id,
    ConfirmDialog* confirmDialog)
    : impl_{
          std::make_unique<Implementation>(
              stateHolder,
              events,
              std::move(persistenceSessionName),
              std::move(id),
              confirmDialog),
      }
{}
ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(OperationQueue);

Nui::ElementRenderer OperationQueue::operator()()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;

    return div{
        style = "width: 100%; height: auto; display: block",
    }("OPERATION QUEUE HERE NEW");
}