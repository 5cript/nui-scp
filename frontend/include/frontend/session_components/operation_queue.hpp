#pragma once

#include <persistence/state_holder.hpp>
#include <frontend/dialog/confirm_dialog.hpp>
#include <frontend/events/frontend_events.hpp>
#include <frontend/terminal/file_engine.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

class OperationQueue
{
  public:
    OperationQueue(
        Persistence::StateHolder* stateHolder,
        FrontendEvents* events,
        std::string persistenceSessionName,
        std::string id,
        ConfirmDialog* confirmDialog,
        FileEngine* fileEngine);

    ROAR_PIMPL_SPECIAL_FUNCTIONS(OperationQueue);

    void enqueueDownload(std::filesystem::path const& remotePath, std::filesystem::path const& localPath);
    void enqueueUpload(std::filesystem::path const& localPath, std::filesystem::path const& remotePath);
    void enqueueRename(std::filesystem::path const& oldPath, std::filesystem::path const& newPath);
    void enqueueDelete(std::filesystem::path const& path);

    Nui::ElementRenderer operator()();

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};