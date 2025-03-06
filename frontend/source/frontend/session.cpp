#include <persistence/state/terminal_engine.hpp>
#include <frontend/session.hpp>
#include <frontend/terminal/terminal.hpp>
#include <frontend/terminal/executing_engine.hpp>
#include <frontend/terminal/user_control_engine.hpp>
#include <frontend/terminal/ssh_engine.hpp>
#include <frontend/terminal/sftp_file_engine.hpp>
#include <frontend/classes.hpp>
#include <frontend/dialog/input_dialog.hpp>
#include <frontend/session_components/session_options.hpp>
#include <nui-file-explorer/file_grid.hpp>
#include <persistence/state_holder.hpp>
#include <constants/layouts.hpp>
#include <log/log.hpp>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/frontend/element_renderer.hpp>
#include <nui/event_system/listen.hpp>
#include <nui/frontend/api/console.hpp>
#include <nui/frontend/utility/delocalized.hpp>
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

#include <algorithm>
#include <vector>

using namespace Nui;
using namespace Nui::Elements;
using namespace Nui::Attributes;

struct Session::Implementation
{
    // Prop Drill:
    Persistence::StateHolder* stateHolder;
    FrontendEvents* events;

    // Session Ui Tab related:
    std::string initialName;
    std::shared_ptr<Nui::Observed<std::string>> tabTitle;
    std::string id;
    std::function<void(Session const*)> closeSelf;
    Nui::Observed<bool> isVisible;
    Persistence::UiOptions uiOptions;

    // (Ssh, ...)Session / Terminal Engine:
    Persistence::TerminalEngine engine;
    Nui::Observed<Persistence::TerminalOptions> options;

    // Dialogs:
    InputDialog* newItemAskDialog;
    ConfirmDialog* confirmDialog;

    // Operation Queue for File Explorer
    std::shared_ptr<Nui::Dom::Element> operationQueue;

    // File Explorer Things:
    NuiFileExplorer::FileGrid fileGrid;
    std::shared_ptr<Nui::Dom::Element> fileExplorer;
    std::filesystem::path currentPath;
    std::unique_ptr<FileEngine> fileEngine;
    std::filesystem::path preNavigatePath;

    // Layout Engine Related
    std::weak_ptr<Nui::Dom::BasicElement> layoutHost;
    std::optional<std::string> layoutName;
    bool waitingForLayoutHost{false};

    // Channels & Terminal Connection
    Nui::Observed<std::unique_ptr<Terminal>> terminal;
    std::vector<std::shared_ptr<Nui::Dom::Element>> channelElements;

    // Session Options
    std::shared_ptr<Nui::Dom::Element> sessionOptionsElement{};
    SessionOptions sessionOptions;

    // Shutdown:
    std::function<void()> onShutdownComplete{};

    Implementation(
        Persistence::StateHolder* stateHolder,
        FrontendEvents* events,
        Persistence::TerminalEngine engine,
        Persistence::UiOptions uiOptions,
        std::string initialName,
        std::optional<std::string> layoutName,
        InputDialog* newItemAskDialog,
        ConfirmDialog* confirmDialog,
        std::function<void(Session const*)> closeSelf,
        bool visible)
        : stateHolder{stateHolder}
        , events{events}
        , initialName{std::move(initialName)}
        , tabTitle{std::make_shared<Nui::Observed<std::string>>(this->initialName)}
        , id{Nui::val::global("generateId")().as<std::string>()}
        , closeSelf{std::move(closeSelf)}
        , isVisible{visible}
        , uiOptions{uiOptions}
        , engine{std::move(engine)}
        , options{this->engine.terminalOptions.value()}
        , newItemAskDialog{newItemAskDialog}
        , confirmDialog{confirmDialog}
        , fileGrid{{
              .pathBarOnTop = uiOptions.fileGridPathBarOnTop,
          }}
        , fileExplorer{}
        , currentPath{}
        , fileEngine{}
        , preNavigatePath{}
        , layoutHost{}
        , layoutName{std::move(layoutName)}
        , terminal{}
        , channelElements{}
        , sessionOptionsElement{}
        , sessionOptions{stateHolder, events, this->initialName, this->id, confirmDialog}
    {}
};

auto Session::makeChannelElement() -> Nui::ElementRenderer
{
    using Nui::Elements::div; // because of the global div.

    // clang-format off
    return div{}(
        observe(impl_->terminal),
        [this]() -> Nui::ElementRenderer {
            return div{
                style = "height: 100%; width: 100%",
                class_ = "terminal-channel",
                reference.onMaterialize([this](Nui::val element) {
                    Log::info("Channel terminal materialized");
                    if (impl_->terminal.value())
                    {
                        impl_->terminal.value()->createChannel(element, *impl_->options, std::bind(&Session::onOpenChannel, this, std::placeholders::_1, std::placeholders::_2));
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
    FrontendEvents* events,
    Persistence::TerminalEngine engine,
    Persistence::UiOptions uiOptions,
    std::string initialName,
    std::optional<std::string> layoutName,
    InputDialog* newItemAskDialog,
    ConfirmDialog* confirmDialog,
    std::function<void(Session const*)> closeSelf,
    bool visible)
    : impl_{std::make_unique<Implementation>(
          stateHolder,
          events,
          std::move(engine),
          std::move(uiOptions),
          std::move(initialName),
          std::move(layoutName),
          newItemAskDialog,
          confirmDialog,
          std::move(closeSelf),
          visible)}
{
    if (std::holds_alternative<Persistence::ExecutingTerminalEngine>(impl_->engine.engine))
    {
        createExecutingEngine();
        // what about files?
    }
    else if (std::holds_alternative<Persistence::SshTerminalEngine>(impl_->engine.engine))
    {
        createSshEngine();
        setupFileGrid();
    }
    else
    {
        Log::error("Unsupported terminal engine type");
        return;
    }

    Nui::globalEventContext.executeActiveEventsImmediately();
}

void Session::setupFileGrid()
{
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
            // TODO: create file
        }
    });

    impl_->fileGrid.onRefresh([this]() {
        navigateTo(impl_->currentPath);
    });

    impl_->fileGrid.onPathChange([this](std::filesystem::path const& path) {
        navigateTo(path);
    });
}

void Session::createSshEngine()
{
    Log::info("Creating SSH engine");

    impl_->terminal = std::make_unique<Terminal>(
        std::make_unique<SshTerminalEngine>(SshTerminalEngine::Settings{
            .engineOptions = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine),
            .onExit = std::bind(&Session::onTerminalConnectionClose, this),
            .onBeforeExit = std::bind(&Session::onBeforeTerminalConnectionClose, this),
        }),
        true);

    impl_->terminal.value()->open(
        std::bind(&Session::onOpenSession, this, std::placeholders::_1, std::placeholders::_2));
}

void Session::onBeforeTerminalConnectionClose()
{
    // TODO:
    // if (impl_->terminal.value())
    // {
    //     impl_->terminal.value()->iterateAllChannels([](std::string const& /*channelId*/, TerminalChannel& channel) {
    //         std::string id = channel.stealTerminal();
    //         return true;
    //     });
    // }
}

void Session::createExecutingEngine()
{
    impl_->terminal = std::make_unique<Terminal>(
        std::make_unique<ExecutingTerminalEngine>(ExecutingTerminalEngine::Settings{
            .engineOptions = std::get<Persistence::ExecutingTerminalEngine>(impl_->engine.engine),
            .termios = impl_->engine.termios.value(),
            .onProcessChange =
                [this](std::string const& cmdline) {
                    Log::info("Tab title changed: {}", cmdline);
                    *impl_->tabTitle = cmdline;
                    Nui::globalEventContext.executeActiveEventsImmediately();
                },
        }),
        false);

    impl_->terminal.value()->open(
        std::bind(&Session::onOpenSession, this, std::placeholders::_1, std::placeholders::_2));
}

Session::~Session() = default;

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
    std::transform(
        begin(*directoryEntries), end(*directoryEntries), std::back_inserter(items), [this](auto const& entry) {
            return NuiFileExplorer::FileGrid::Item{
                .path = entry.path,
                .icon = [&entry, this]() -> std::string {
                    const auto type = static_cast<NuiFileExplorer::FileGrid::Item::Type>(entry.type);
                    if (type == NuiFileExplorer::FileGrid::Item::Type::Directory)
                        return "nui://app.example/icons/folder_main.png";
                    if (type == NuiFileExplorer::FileGrid::Item::Type::BlockDevice)
                        return "nui://app.example/icons/hard_drive.png";

                    if (impl_->uiOptions.fileGridExtensionIcons.contains(entry.path.extension().string()))
                    {
                        return "nui://app.example/" +
                            impl_->uiOptions.fileGridExtensionIcons.at(entry.path.extension().string());
                    }

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
    Log::warn("SFTP is not implemented yet");
    // if (impl_->terminal.value() && impl_->terminal.value()->engine().engineName() == "ssh")
    // {
    //     auto const& opts = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine).sshSessionOptions.value();
    //     if (opts.openSftpByDefault)
    //     {
    //         Log::info("Opening SFTP by default");
    //         impl_->fileEngine = std::make_unique<SftpFileEngine>(SshTerminalEngine::Settings{
    //             .engineOptions = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine),
    //             .onExit = std::bind(&Session::onFileExplorerConnectionClose, this),
    //         });
    //         navigateTo(opts.defaultDirectory.value_or("/"));
    //     }
    // }
    // else
    // {
    //     Log::error("Cannot open SFTP for non-ssh terminal");
    // }
}

void Session::fallbackToUserControlEngine()
{
    // TODO:

    // impl_->terminal = std::make_unique<Terminal>(
    //     std::make_unique<UserControlEngine>(UserControlEngine::Settings{
    //         .onInput =
    //             [this](std::string const& input) {
    //                 if (input == "\u0003" && impl_->closeSelf)
    //                     impl_->closeSelf(*this);
    //             },
    //     }),
    //     false);

    // impl_->terminal.value()->open([](bool success, std::string const& info) {
    //     if (!success)
    //     {
    //         Log::error("Failed to open user control terminal: {}", info);
    //         return;
    //     }
    //     Log::info("User control terminal opened successfully");
    // });

    // impl_->terminal.value()->write(
    //     fmt::format("\033[1;31mFailed to create instance: {}.\r\nPress Ctrl+C do close this tab.\033[00m", info),
    //     false);
    // Nui::globalEventContext.executeActiveEventsImmediately();

    // New layout?:
    // initializeLayout();
}

void Session::onOpenSession(bool success, std::string const& info)
{
    if (!success)
    {
        Log::info("Failed to create session instance: {}", info);
        fallbackToUserControlEngine();
    }
    else
    {
        Log::info("Session opened successfully: {}", info);
        if (impl_->terminal.value() && impl_->terminal.value()->engine().engineName() == "ssh")
        {
            // TODO: __todo_default__ is probably something that should be replaced with a proper default value
            const auto user = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine)
                                  .sshSessionOptions.value()
                                  .user.value_or("__todo_default__");
            auto host = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine).sshSessionOptions.value().host;
            const auto port = std::get<Persistence::SshTerminalEngine>(impl_->engine.engine)
                                  .sshSessionOptions.value()
                                  .port.value_or(22);

            // assume ipv6 when finding ':' in host
            if (host.find(":") != std::string::npos)
                host = "[" + host + "]";
            *impl_->tabTitle = user + "@" + host + ":" + std::to_string(port);

            openSftp();
        }

        impl_->terminal.value()->focus();
        initializeLayout();
    }
}

void Session::onOpenChannel(std::optional<Ids::ChannelId> channelId, std::string const& info)
{
    if (!channelId)
    {
        Log::error("Failed to open channel: {}", info);
        return;
    }

    Log::info("Channel opened successfully: {}", channelId->value());
}

void Session::onTerminalConnectionClose()
{
    // TODO: this is harsh, when the connection dropped unexpectedly, so keep the terminal open and
    // print a disconnect warning.

    Log::debug("onTerminalConnectionClose");

    impl_->terminal.value().reset();

    if (impl_->fileEngine)
    {
        impl_->fileEngine->dispose();
    }
    else
    {
        closeSelf();
    }
}

void Session::onFileExplorerConnectionClose()
{
    // TODO: this is harsh, when the connection dropped unexpectedly, so keep the terminal open and
    // print a disconnect warning.

    Log::debug("onFileExplorerConnectionClose");

    impl_->fileEngine.reset();

    if (impl_->terminal.value())
    {
        impl_->terminal.value()->dispose();
    }
    else
    {
        closeSelf();
    }
}

void Session::managerShutdown(std::function<void()> onShutdown)
{
    const bool isExecutingEngine = std::holds_alternative<Persistence::ExecutingTerminalEngine>(impl_->engine.engine);

    if (!isExecutingEngine && (impl_->terminal.value() || impl_->fileEngine))
    {
        if (impl_->terminal.value())
        {
            Log::info("Waiting for terminal engine to close");
        }
        if (impl_->fileEngine)
        {
            Log::info("Waiting for file engine to close");
        }
        impl_->onShutdownComplete = std::move(onShutdown);
    }
    else
    {
        Log::info("Session shutdown is already complete");
        Nui::val::global("contentPanelManager").call<void>("removePanel", impl_->id);
        onShutdown();
    }
}

void Session::closeSelf()
{
    if (impl_->onShutdownComplete)
    {
        Log::info("Session shutdown complete");
        Nui::val::global("contentPanelManager").call<void>("removePanel", impl_->id);

        impl_->onShutdownComplete();
        return;
    }

    if (impl_->closeSelf)
    {
        impl_->closeSelf(this);
        return;
    }
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

auto Session::makeOperationQueueElement() -> Nui::ElementRenderer
{
    using Nui::Elements::div; // because of the global div.
    using namespace Nui::Attributes;

    // clang-format off
    return div{
        style = "width: 100%; height: auto; display: block",
    }(
        "OPERATION QUEUE HERE"
    );
    // clang-format on
}

void Session::onChannelClosedByUser(Ids::ChannelId const& channelId)
{
    using namespace std::string_literals;

    // Removing element from vector:
    std::erase_if(impl_->channelElements, [channelId](auto elem) -> bool {
        const auto val = elem->val();
        if (val.template call<bool>("hasAttribute", "data-channelid"s))
        {
            const auto id = val.template call<std::string>("getAttribute", "data-channelid"s);
            return id == channelId.value();
        }
        else
        {
            // FIXME: I can see this warning, which should not happen, but it does.
            Log::warn("Channel element does not have a channel id attribute");
        }
        return false;
    });

    // Removing channel in frontend:
    using namespace std::string_literals;
    impl_->terminal.value()->closeChannel(channelId);
}

void Session::initializeLayout()
{
    Nui::val element;
    if (auto host = impl_->layoutHost.lock(); host)
    {
        element = host->val();
    }
    else
    {
        Log::info("Waiting for layout host");
        impl_->waitingForLayoutHost = true;
        return;
    }

    std::optional<std::string> layout = std::nullopt;

    if (impl_->layoutName != Constants::defaultLayoutName)
    {
        if (impl_->engine.layouts && impl_->layoutName)
        {
            if (auto iter = impl_->engine.layouts->find(*impl_->layoutName); iter != impl_->engine.layouts->end())
            {
                layout = iter->second.dump();
            }
            else
            {
                Log::warn("Layout name not found: {}", *impl_->layoutName);
            }
        }
    }

    Log::info("Initializing layout with name: {}", layout.value_or("(none)"));

    Nui::val::global("contentPanelManager")
        .call<void>(
            "addPanel",
            element,
            impl_->id,
            layout.value_or(""),
            Nui::bind([this]() -> Nui::val {
                Nui::Console::log("Channel factory content panel manager");
                auto elem = Nui::Dom::makeStandaloneElement(makeChannelElement());
                impl_->channelElements.push_back(elem);
                return elem->val();
            }),
            Nui::bind(
                [this](Nui::val channelIdVal) -> Nui::val {
                    Nui::Console::log(channelIdVal);

                    if (channelIdVal.isUndefined())
                    {
                        Log::critical("Channel id is undefined");
                        return Nui::val::undefined();
                    }

                    if (channelIdVal.isString())
                    {
                        Ids::ChannelId channelId = Ids::makeChannelId(channelIdVal.as<std::string>());
                        if (!channelId.isValid())
                        {
                            Log::critical("Channel id is not valid");
                            return Nui::val::undefined();
                        }

                        onChannelClosedByUser(channelId);
                    }
                    else
                    {
                        Log::critical("Channel id is not a string");
                    }
                    return Nui::val::undefined();
                },
                std::placeholders::_1),
            Nui::bind([this]() -> Nui::val {
                // OpenFileExplorer
                if (impl_->fileExplorer)
                {
                    Log::warn("There is already a file explorer, cannot open another one");
                    return Nui::val::undefined();
                }
                impl_->fileExplorer = Nui::Dom::makeStandaloneElement(makeFileExplorerElement());
                return impl_->fileExplorer->val();
            }),
            Nui::bind([this]() -> Nui::val {
                // Remove FileExplorer
                if (!impl_->fileExplorer)
                {
                    Log::warn("There is no file explorer to remove");
                    return Nui::val::undefined();
                }
                impl_->fileExplorer.reset();
                return Nui::val::undefined();
            }),
            Nui::bind([this]() -> Nui::val {
                if (impl_->operationQueue)
                {
                    Log::warn("There is already an operation queue, cannot open another one");
                    return Nui::val::undefined();
                }
                impl_->operationQueue = Nui::Dom::makeStandaloneElement(makeOperationQueueElement());
                return impl_->operationQueue->val();
            }),
            Nui::bind([this]() -> Nui::val {
                if (!impl_->operationQueue)
                {
                    Log::warn("There is no operation queue to remove");
                    return Nui::val::undefined();
                }
                impl_->operationQueue.reset();
                return Nui::val::undefined();
            }),
            Nui::bind([this]() -> Nui::val {
                if (impl_->sessionOptionsElement)
                {
                    Log::warn("There are already session options, cannot open another one");
                    return Nui::val::undefined();
                }
                impl_->sessionOptionsElement = Nui::Dom::makeStandaloneElement(impl_->sessionOptions());
                return impl_->sessionOptionsElement->val();
            }),
            Nui::bind([this]() -> Nui::val {
                if (!impl_->sessionOptionsElement)
                {
                    Log::warn("There are no session options to remove");
                    return Nui::val::undefined();
                }
                impl_->sessionOptionsElement.reset();
                return Nui::val::undefined();
            }));
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
        !(reference = [this](
            std::weak_ptr<Nui::Dom::BasicElement>&& elem
        ){
            impl_->layoutHost = elem.lock();
            if (impl_->waitingForLayoutHost) {
                initializeLayout();
                impl_->waitingForLayoutHost = false;
            }
        })
    }(
    );
    // clang-format on
}