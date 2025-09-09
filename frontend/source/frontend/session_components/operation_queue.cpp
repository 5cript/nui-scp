#include <frontend/session_components/operation_queue.hpp>
#include <frontend/components/progress_bar.hpp>

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
    FileEngine* fileEngine;

    Implementation(
        Persistence::StateHolder* stateHolder,
        FrontendEvents* events,
        std::string persistenceSessionName,
        std::string id,
        ConfirmDialog* confirmDialog,
        FileEngine* fileEngine)
        : stateHolder{stateHolder}
        , events{events}
        , persistenceSessionName{std::move(persistenceSessionName)}
        , id{std::move(id)}
        , confirmDialog{confirmDialog}
        , fileEngine{fileEngine}
    {}
};

OperationQueue::OperationQueue(
    Persistence::StateHolder* stateHolder,
    FrontendEvents* events,
    std::string persistenceSessionName,
    std::string id,
    ConfirmDialog* confirmDialog,
    FileEngine* fileEngine)
    : impl_{
          std::make_unique<Implementation>(
              stateHolder,
              events,
              std::move(persistenceSessionName),
              std::move(id),
              confirmDialog,
              fileEngine),
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

void OperationQueue::enqueueDownload(std::filesystem::path const& remotePath, std::filesystem::path const& localPath)
{
    // fileEngine->
}
void OperationQueue::enqueueUpload(std::filesystem::path const& localPath, std::filesystem::path const& remotePath)
{}
void OperationQueue::enqueueRename(std::filesystem::path const& oldPath, std::filesystem::path const& newPath)
{}
void OperationQueue::enqueueDelete(std::filesystem::path const& path)
{}
void OperationQueue::enqueueDownloadSet(
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> const& downloads)
{
    // TODO: Do this as a group operation. For now, just iterate and enqueue each one.
    for (const auto& [remotePath, localPath] : downloads)
    {
        enqueueDownload(remotePath, localPath);
    }
}
void OperationQueue::enqueueUploadSet(
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> const& uploads)
{
    // TODO: Do this as a group operation. For now, just iterate and enqueue each one.
    for (const auto& [remotePath, localPath] : uploads)
    {
        enqueueDownload(remotePath, localPath);
    }
}