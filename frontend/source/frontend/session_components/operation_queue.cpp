#include <frontend/session_components/operation_queue.hpp>
#include <frontend/components/progress_bar.hpp>

#include <log/log.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/rpc.hpp>
#include <fmt/format.h>

struct OperationQueue::Implementation
{
    Persistence::StateHolder* stateHolder;
    FrontendEvents* events;
    std::string persistenceSessionName;
    std::string sessionId;
    ConfirmDialog* confirmDialog;
    FileEngine* fileEngine;

    Nui::RpcClient::AutoUnregister onUpdate;

    Implementation(
        Persistence::StateHolder* stateHolder,
        FrontendEvents* events,
        std::string persistenceSessionName,
        std::string sessionId,
        ConfirmDialog* confirmDialog)
        : stateHolder{stateHolder}
        , events{events}
        , persistenceSessionName{std::move(persistenceSessionName)}
        , sessionId{std::move(sessionId)}
        , confirmDialog{confirmDialog}
        , fileEngine{nullptr}
    {
        onUpdate =
            Nui::RpcClient::autoRegisterFunction("OperationQueue::" + sessionId + "::onOperationAdded", [](Nui::val) {
                // TODO:
            });
    }
};

OperationQueue::OperationQueue(
    Persistence::StateHolder* stateHolder,
    FrontendEvents* events,
    std::string persistenceSessionName,
    std::string sessionId,
    ConfirmDialog* confirmDialog)
    : impl_{
          std::make_unique<Implementation>(
              stateHolder,
              events,
              std::move(persistenceSessionName),
              std::move(sessionId),
              confirmDialog),
      }
{}
ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL(OperationQueue);

void OperationQueue::setFileEngine(FileEngine* fileEngine)
{
    impl_->fileEngine = fileEngine;
}

Nui::ElementRenderer OperationQueue::operator()()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;

    return div{
        style = "width: 100%; height: auto; display: block",
    }("OPERATION QUEUE HERE NEW");
}

void OperationQueue::enqueueDownload(
    std::filesystem::path const& remotePath,
    std::filesystem::path const& localPath,
    std::function<void(std::optional<Ids::OperationId> const&)> onComplete)
{
    if (!impl_->fileEngine)
    {
        Log::error("No file engine set for operation queue, cannot enqueue download");
        onComplete(std::nullopt);
        return;
    }

    Log::info("Frontend Operation Queue download: {} -> {}", remotePath.generic_string(), localPath.generic_string());
    impl_->fileEngine->addDownload(remotePath, localPath, std::move(onComplete));
}
void OperationQueue::enqueueUpload(
    std::filesystem::path const& localPath,
    std::filesystem::path const& remotePath,
    std::function<void(std::optional<Ids::OperationId> const&)> onComplete)
{
    if (!impl_->fileEngine)
    {
        Log::error("No file engine set for operation queue, cannot enqueue download");
        onComplete(std::nullopt);
        return;
    }
    // TODO: Implement
}
void OperationQueue::enqueueRename(
    std::filesystem::path const& oldPath,
    std::filesystem::path const& newPath,
    std::function<void(std::optional<Ids::OperationId> const&)> onComplete)
{
    if (!impl_->fileEngine)
    {
        Log::error("No file engine set for operation queue, cannot enqueue download");
        onComplete(std::nullopt);
        return;
    }
    // TODO: Implement
}
void OperationQueue::enqueueDelete(
    std::filesystem::path const& path,
    std::function<void(std::optional<Ids::OperationId> const&)> onComplete)
{
    if (!impl_->fileEngine)
    {
        Log::error("No file engine set for operation queue, cannot enqueue download");
        onComplete(std::nullopt);
        return;
    }
    // TODO: Implement
}