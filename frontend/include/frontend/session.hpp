#pragma once

#include <persistence/state_holder.hpp>
#include <persistence/state/terminal_engine.hpp>
#include <persistence/state/termios.hpp>
#include <persistence/state/terminal_options.hpp>
#include <frontend/input_dialog.hpp>
#include <shared_data/directory_entry.hpp>

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/utility/stabilize.hpp>
#include <nui/utility/move_detector.hpp>
#include <roar/detail/pimpl_special_functions.hpp>

class Session
{
  public:
    Session(
        Persistence::StateHolder* stateHolder,
        Persistence::TerminalEngine engine,
        std::string initialName,
        InputDialog* newItemAskDialog,
        std::function<void(Session const& self)> closeSelf,
        bool visible);
    ROAR_PIMPL_SPECIAL_FUNCTIONS(Session);

    Nui::ElementRenderer operator()(bool visible);

    std::string name() const;
    std::weak_ptr<Nui::Observed<std::string>> tabTitle() const;
    void visible(bool value);
    bool visible() const;

    std::optional<std::string> getProcessIdIfExecutingEngine() const;
    auto makeTerminalElement() -> Nui::ElementRenderer;
    auto makeFileExplorerElement() -> Nui::ElementRenderer;

  private:
    void onOpen(bool success, std::string const& info);
    void onFileExplorerConnectionClose();
    void onTerminalConnectionClose();
    void onDirectoryListing(std::optional<std::vector<SharedData::DirectoryEntry>>);
    void navigateTo(std::filesystem::path const& path);

  private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
    Nui::MoveDetector moveDetector_;
};