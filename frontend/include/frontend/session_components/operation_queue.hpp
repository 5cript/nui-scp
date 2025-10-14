#pragma once

#include <persistence/state_holder.hpp>
#include <frontend/dialog/confirm_dialog.hpp>
#include <frontend/events/frontend_events.hpp>
#include <frontend/terminal/file_engine.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

#include <chrono>

class OperationQueue
{
  public:
    constexpr static std::chrono::seconds autoRemoveTime{5};

    OperationQueue(
        Persistence::StateHolder* stateHolder,
        FrontendEvents* events,
        std::string persistenceSessionName,
        ConfirmDialog* confirmDialog);

    ROAR_PIMPL_SPECIAL_FUNCTIONS(OperationQueue);

    void activate(FileEngine* fileEngine, Ids::SessionId sessionId);

    void enqueueDownload(
        std::filesystem::path const& remotePath,
        std::filesystem::path const& localPath,
        std::function<void(std::optional<Ids::OperationId> const&)> onComplete);
    void enqueueUpload(
        std::filesystem::path const& localPath,
        std::filesystem::path const& remotePath,
        std::function<void(std::optional<Ids::OperationId> const&)> onComplete);
    void enqueueRename(
        std::filesystem::path const& oldPath,
        std::filesystem::path const& newPath,
        std::function<void(std::optional<Ids::OperationId> const&)> onComplete);
    void enqueueDelete(
        std::filesystem::path const& path,
        std::function<void(std::optional<Ids::OperationId> const&)> onComplete);

    Nui::ElementRenderer operator()();

  private:
    template <typename OperationCard>
    void cancelOperation(OperationCard const& operation);

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};