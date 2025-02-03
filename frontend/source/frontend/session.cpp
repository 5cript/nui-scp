#include <persistence/state/terminal_engine.hpp>
#include <frontend/session.hpp>
#include <frontend/terminal/terminal.hpp>
#include <frontend/terminal/executing_engine.hpp>
#include <frontend/terminal/user_control_engine.hpp>
#include <frontend/terminal/ssh_engine.hpp>
#include <frontend/terminal/sftp_file_engine.hpp>
#include <frontend/classes.hpp>
#include <frontend/input_dialog.hpp>
#include <nui-file-explorer/file_grid.hpp>
#include <persistence/state_holder.hpp>
#include <log/log.hpp>

#include <fmt/format.h>
#include <nui/event_system/event_context.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/listen.hpp>
#include <nui/frontend/api/console.hpp>
#include <nui/frontend/utility/delocalized.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

#include <algorithm>

using namespace Nui;
using namespace Nui::Elements;
using namespace Nui::Attributes;

struct Session::Implementation
{
    Persistence::StateHolder* stateHolder;
    Persistence::TerminalEngine engine;
    Nui::Observed<Persistence::TerminalOptions> options;
    Nui::Observed<std::unique_ptr<Terminal>> terminal;
    std::string initialName;
    std::shared_ptr<Nui::Observed<std::string>> tabTitle;
    std::string id;
    std::function<void(Session const& self)> closeSelf;
    Nui::Observed<bool> isVisible;
    std::shared_ptr<Nui::Dom::Element> terminalElement;
    NuiFileExplorer::FileGrid fileGrid;
    std::shared_ptr<Nui::Dom::Element> fileExplorer;
    std::filesystem::path currentPath;
    std::unique_ptr<FileEngine> fileEngine;
    InputDialog* newItemAskDialog;
    std::filesystem::path preNavigatePath;

    Implementation(
        Persistence::StateHolder* stateHolder,
        Persistence::TerminalEngine engine,
        Persistence::UiOptions uiOptions,
        std::string initialName,
        InputDialog* newItemAskDialog,
        std::function<void(Session const& self)> closeSelf,
        bool visible)
        : stateHolder{stateHolder}
        , engine{std::move(engine)}
        , options{this->engine.terminalOptions.value()}
        , terminal{}
        , initialName{std::move(initialName)}
        , tabTitle{std::make_shared<Nui::Observed<std::string>>(this->initialName)}
        , id{Nui::val::global("generateId")().as<std::string>()}
        , closeSelf{std::move(closeSelf)}
        , isVisible{visible}
        , terminalElement{}
        , fileGrid{{
              .pathBarOnTop = uiOptions.fileGridPathBarOnTop,
          }}
        , fileExplorer{}
        , currentPath{}
        , fileEngine{}
        , newItemAskDialog{newItemAskDialog}
        , preNavigatePath{}
    {}
};

auto Session::makeTerminalElement() -> Nui::ElementRenderer
{
    using Nui::Elements::div; // because of the global div.

    // clang-format off
    return div{}(
        observe(impl_->terminal),
        [this]() -> Nui::ElementRenderer {
            return div{
                style = "height: 100%; width: 100%",
                reference.onMaterialize([this](Nui::val element) {
                    Log::info("Terminal materialized");
                    if (impl_->terminal.value())
                    {
                        impl_->terminal.value()->open(
                            element,
                            *impl_->options,
                            std::bind(&Session::onOpen, this, std::placeholders::_1, std::placeholders::_2));
                    }
                })
            }();
        }
    );
    // clang-format on
}

auto Session::makeFileExplorerElement() -> Nui::ElementRenderer
{
    using Nui::Elements::div; // because of the global div.
    using namespace Nui::Attributes;

    // clang-format off
    return div{
        style = "width: 100%; height: auto; display: block",
    }(
        impl_->fileGrid()
    );
    // clang-format on
}

Session::Session(
    Persistence::StateHolder* stateHolder,
    Persistence::TerminalEngine engine,
    Persistence::UiOptions uiOptions,
    std::string initialName,
    InputDialog* newItemAskDialog,
    std::function<void(Session const& self)> closeSelf,
    bool visible)
    : impl_{std::make_unique<Implementation>(
          stateHolder,
          std::move(engine),
          std::move(uiOptions),
          std::move(initialName),
          newItemAskDialog,
          std::move(closeSelf),
          visible)}
{
    if (std::holds_alternative<Persistence::ExecutingTerminalEngine>(impl_->engine.engine))
    {
        impl_->terminal =
            std::make_unique<Terminal>(std::make_unique<ExecutingTerminalEngine>(ExecutingTerminalEngine::Settings{
                .engineOptions = std::get<Persistence::ExecutingTerminalEngine>(impl_->engine.engine),
                .termios = impl_->engine.termios.value(),
                .onProcessChange =
                    [this](std::string const& cmdline) {
                        Log::info("Tab title changed: {}", cmdline);
                        *impl_->tabTitle = cmdline;
                        Nui::globalEventContext.executeActiveEventsImmediately();
                    },
            }));
    }
    else if (std::holds_alternative<Persistence::SshTerminalEngine>(impl_->engine.engine))
    {
        impl_->terminal = std::make_unique<Terminal>(std::make_unique<SshTerminalEngine>(SshTerminalEngine::Settings{
            .engineOptions = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine),
            .onExit = std::bind(&Session::onTerminalConnectionClose, this),
        }));

        const auto user = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine)
                              .sshSessionOptions.value()
                              .user.value_or("__todo_default__");
        auto host = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine).sshSessionOptions.value().host;
        const auto port =
            std::get<Persistence::SshTerminalEngine>(impl_->engine.engine).sshSessionOptions.value().port.value_or(22);

        // assume ipv6 when finding ':' in host
        if (host.find(":") != std::string::npos)
            host = "[" + host + "]";
        *impl_->tabTitle = user + "@" + host + ":" + std::to_string(port);
    }
    else
    {
        Log::error("Unsupported terminal engine type");
        return;
    }

    impl_->fileGrid.onActivateItem([this](auto const& item) {
        // TODO: what about files?:
        if (item.type != NuiFileExplorer::FileGrid::Item::Type::Directory)
            return;

        if (item.path == ".")
            return;
        if (item.path == "..")
            return navigateTo(impl_->currentPath.parent_path());

        navigateTo(impl_->currentPath / item.path);
    });

    impl_->fileGrid.onNewItem([this](auto type) {
        Log::info("New item requested: {}", static_cast<int>(type));
        if (type == NuiFileExplorer::FileGrid::Item::Type::Directory)
        {
            // impl_->fileEngine->createDirectory(impl_->currentPath);
            impl_->newItemAskDialog->open(
                "New directory",
                "Enter the name of the new directory",
                "Create a new directory",
                false,
                [this](std::optional<std::string> const& name) {
                    if (!name)
                        return;

                    Log::info("Creating new directory: {}", *name);
                    if (name->find('/') != std::string::npos)
                    {
                        Log::error("Invalid directory name (cannot contain slashes): {}", *name);
                        return;
                    }
                    impl_->fileEngine->createDirectory(impl_->currentPath / *name, [this](bool success) {
                        if (!success)
                        {
                            Log::error("Failed to create directory");
                            return;
                        }
                        // Refresh list from server:
                        navigateTo(impl_->currentPath);
                    });
                });
        }
        else
        {
            // TODO:
        }
    });

    impl_->fileGrid.onRefresh([this]() {
        navigateTo(impl_->currentPath);
    });

    impl_->fileGrid.onPathChange([this](std::filesystem::path const& path) {
        navigateTo(path);
    });

    Nui::globalEventContext.executeActiveEventsImmediately();
}

Session::~Session()
{
    if (impl_->terminalElement)
    {
        Nui::val::global("contentPanelManager").call<void>("removePanel", impl_->id);
    }
}

ROAR_PIMPL_SPECIAL_FUNCTIONS_IMPL_NO_DTOR(Session);

void Session::onDirectoryListing(std::optional<std::vector<SharedData::DirectoryEntry>> directoryEntries)
{
    if (!directoryEntries)
    {
        Log::error("Failed to list directory");
        // undo the navigation:
        impl_->currentPath = impl_->preNavigatePath;
        navigateTo(impl_->currentPath);
        return;
    }

    std::erase_if(*directoryEntries, [](auto const& entry) {
        return entry.path.filename() == ".";
    });

    std::vector<NuiFileExplorer::FileGrid::Item> items{};
    std::transform(begin(*directoryEntries), end(*directoryEntries), std::back_inserter(items), [](auto const& entry) {
        return NuiFileExplorer::FileGrid::Item{
            .path = entry.path,
            .icon =
                [&entry]() {
                    const auto type = static_cast<NuiFileExplorer::FileGrid::Item::Type>(entry.type);
                    if (type == NuiFileExplorer::FileGrid::Item::Type::Directory)
                        return "nui://app.example/icons/folder_main.png";
                    if (type == NuiFileExplorer::FileGrid::Item::Type::BlockDevice)
                        return "nui://app.example/icons/hard_drive.png";

                    if (entry.path.extension() == ".cpp")
                        return "nui://app.example/icons/cpp_file.png";
                    // TODO: more icons

                    return "nui://app.example/icons/file.png";
                }(),
            .type = static_cast<NuiFileExplorer::FileGrid::Item::Type>(entry.type),
            .permissions = entry.permissions,
            .ownerId = entry.uid,
            .groupId = entry.gid,
            .atime = entry.atime,
            .size = entry.size,
        };
    });

    impl_->fileGrid.items(items);
}

void Session::navigateTo(std::filesystem::path path)
{
    path = path.lexically_normal();

    Log::info("Navigating to: {}", path.generic_string());
    impl_->preNavigatePath = impl_->currentPath;
    impl_->currentPath = path;
    impl_->fileEngine->listDirectory(
        impl_->currentPath, std::bind(&Session::onDirectoryListing, this, std::placeholders::_1));
    impl_->fileGrid.path(path.generic_string());
}

void Session::openSftp()
{
    if (impl_->terminal.value() && impl_->terminal.value()->engine().engineName() == "ssh")
    {
        auto const& opts = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine).sshSessionOptions.value();
        if (opts.openSftpByDefault)
        {
            Log::info("Opening SFTP by default");
            impl_->fileEngine = std::make_unique<SftpFileEngine>(SshTerminalEngine::Settings{
                .engineOptions = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine),
                .onExit = std::bind(&Session::onFileExplorerConnectionClose, this),
            });
            navigateTo(opts.defaultDirectory.value_or("/"));
        }
    }
    else
    {
        Log::error("Cannot open SFTP for non-ssh terminal");
    }
}

void Session::onOpen(bool success, std::string const& info)
{
    if (!success)
    {
        Log::info("Failed to create instance: {}", info);
        impl_->terminal = std::make_unique<Terminal>(std::make_unique<UserControlEngine>(UserControlEngine::Settings{
            .onInput =
                [this](std::string const& input) {
                    if (input == "\u0003" && impl_->closeSelf)
                        impl_->closeSelf(*this);
                },
        }));
        impl_->terminal.value()->write(
            fmt::format("\033[1;31mFailed to create instance: {}.\r\nPress Ctrl+C do close this tab.\033[00m", info),
            false);
        Nui::globalEventContext.executeActiveEventsImmediately();
    }
    else
    {
        Log::info("Terminal opened successfully");
        impl_->terminal.value()->focus();

        if (impl_->terminal.value() && impl_->terminal.value()->engine().engineName() == "ssh")
            openSftp();
    }
}

void Session::onTerminalConnectionClose()
{
    if (impl_->fileEngine)
    {
        impl_->fileEngine.reset();
    }

    // TODO: this is harsh, when the connection dropped unexpectedly, so keep the terminal open and
    // print a disconnect warning.
    if (impl_->closeSelf)
        impl_->closeSelf(*this);
}

void Session::onFileExplorerConnectionClose()
{
    // IF YOU CHANGE THIS IN THE FUTURE: dont forget to close the terminal connection too, which is now implicit by
    // closeSelf.

    // TODO: this is harsh, when the connection dropped unexpectedly, so keep the terminal open and
    // print a disconnect warning.
    if (impl_->closeSelf)
        impl_->closeSelf(*this);
}

std::optional<std::string> Session::getProcessIdIfExecutingEngine() const
{
    if (std::holds_alternative<Persistence::ExecutingTerminalEngine>(impl_->engine.engine))
        return static_cast<ExecutingTerminalEngine&>(impl_->terminal.value()->engine()).id();
    return std::nullopt;
}

std::string Session::name() const
{
    return impl_->initialName;
}

std::weak_ptr<Nui::Observed<std::string>> Session::tabTitle() const
{
    return impl_->tabTitle;
}

bool Session::visible() const
{
    return impl_->isVisible.value();
}

void Session::visible(bool value)
{
    impl_->isVisible = value;
    Nui::globalEventContext.executeActiveEventsImmediately();
    if (value)
        impl_->terminal.value()->focus();
}

Nui::ElementRenderer Session::operator()(bool visible)
{
    using Nui::Elements::div; // because of the global div.
    Log::info("Session::operator()");

    impl_->isVisible = visible;

    // clang-format off
    return div{
        class_ = observe(impl_->isVisible).generate([this]() {
            return classes("terminal-session", impl_->isVisible.value() ? "terminal-session-visible" : "terminal-session-hidden");
        }),
        style = Style{
            "background-color"_style = observe(impl_->options).generate([this]() -> std::string {
                if (impl_->options->theme && impl_->options->theme->background)
                    return *impl_->options->theme->background;
                return "#202020";
            }),
        },
        !reference.onMaterialize([this](Nui::val element){
            Nui::val::global("contentPanelManager").call<void>("addPanel", element, impl_->id, Nui::bind([this]() -> Nui::val {
                Nui::Console::log("terminal factory content panel manager");
                if (impl_->terminalElement)
                {
                    Log::critical("Terminal element already exists - make sure that the session is not recreated");
                    return Nui::val::undefined();
                }
                impl_->terminalElement = Nui::Dom::makeStandaloneElement(makeTerminalElement());
                return impl_->terminalElement->val();
            }), Nui::bind([this]() -> Nui::val {
                // OpenFileExplorer
                if (impl_->fileExplorer)
                {
                    Log::warn("There is already a file explorer, cannot open another one");
                    return Nui::val::undefined();
                }
                impl_->fileExplorer = Nui::Dom::makeStandaloneElement(makeFileExplorerElement());
                return impl_->fileExplorer->val();
            }), Nui::bind([this]() -> Nui::val {
                // Remove FileExplorer
                if (!impl_->fileExplorer)
                {
                    Log::warn("There is no file explorer to remove");
                    return Nui::val::undefined();
                }
                impl_->fileExplorer.reset();
                return Nui::val::undefined();
            }));
        })
    }(
    );
    // clang-format on
}