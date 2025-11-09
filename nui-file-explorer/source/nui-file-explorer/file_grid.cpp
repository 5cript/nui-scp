#include "nui/event_system/event_context.hpp"
#include <nui-file-explorer/file_grid.hpp>
#include <nui-file-explorer/dropdown_menu.hpp>

#include <nui/event_system/observed_value.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/api/console.hpp>

using namespace std::string_literals;

namespace NuiFileExplorer
{
    namespace
    {
        struct ItemWithInternals
        {
            FileGrid::Item item;
            std::shared_ptr<Nui::Observed<bool>> selected;

            explicit ItemWithInternals(FileGrid::Item const& item)
                : item{item}
                , selected{std::make_shared<Nui::Observed<bool>>(false)}
            {}
        };
    }

    std::string fileGridFlavorToString(FileGridFlavor value)
    {
        switch (value)
        {
            case FileGridFlavor::Icons:
                return "icons";
            case FileGridFlavor::Table:
                return "table";
            case FileGridFlavor::Tiles:
                return "tiles";
        }
    }
    FileGridFlavor fileGridFlavorFromString(std::string const& value)
    {
        if (value == "icons")
            return FileGridFlavor::Icons;
        if (value == "table")
            return FileGridFlavor::Table;
        if (value == "tiles")
            return FileGridFlavor::Tiles;
        return FileGridFlavor::Icons;
    }

    struct FileGrid::Implementation
    {
        Nui::Observed<std::vector<ItemWithInternals>> items{};
        Nui::Observed<FileGridFlavor> flavor{FileGridFlavor::Icons};
        Nui::Observed<unsigned int> iconSize{static_cast<unsigned int>(IconSize::Medium)};
        Nui::Observed<unsigned int> iconSpacing{32u};
        DropdownMenu newItemMenu{
            {
                "File",
                "Folder",
                // Soft Link ?
                // Hard Link ?
            },
            [this](std::string const& item) {
                Nui::Console::log("New clicked: ", item);
                if (item == "File")
                {
                    if (onNewItem)
                        onNewItem(Item::Type::Regular);
                }
                else if (item == "Folder")
                {
                    if (onNewItem)
                        onNewItem(Item::Type::Directory);
                }
            },
            [this]() {
                sortMenu.close();
                viewMenu.close();
            },
            "New",
        };
        DropdownMenu sortMenu{
            {
                "Name",
                // More...
            },
            [this](std::string const& item) {
                if (item == "Name")
                {
                    sortItems();
                    this->items.modifyNow();
                }
            },
            [this]() {
                newItemMenu.close();
                viewMenu.close();
            },
            "Sort",
        };
        DropdownMenu viewMenu{
            {
                "Icons",
                "Table",
                "Tiles",
            },
            [this](std::string const& item) {
                if (item == "Icons")
                    flavor = FileGridFlavor::Icons;
                if (item == "Table")
                    flavor = FileGridFlavor::Table;
                if (item == "Tiles")
                    flavor = FileGridFlavor::Tiles;
                Nui::globalEventContext.executeActiveEventsImmediately();
            },
            [this]() {
                newItemMenu.close();
                sortMenu.close();
            },
            "View",
        };

        std::function<void(Item const&)> onActivateItem{};
        std::function<void(Item::Type)> onNewItem{};
        std::weak_ptr<Nui::Dom::BasicElement> contextMenuView{};
        Nui::Observed<std::filesystem::path> path{};
        std::function<void(std::filesystem::path const&)> onPathChange{};
        std::function<void()> onRefresh{};
        std::function<void(std::vector<Item> const&)> onDelete{};
        std::function<void(Item const&)> onRename{};
        std::function<void(std::vector<Item> const&)> onDownload{};
        std::function<void(std::filesystem::path destinationDir, std::vector<Item> const&)> onUpload{};
        std::function<void(std::string const&)> onError{};
        std::function<void(Item const&)> onProperties{};
        Settings settings{};
        std::vector<Item> contextMenuClickItems{};

        void sortItems()
        {
            auto& items = this->items.value();
            std::sort(items.begin(), items.end(), [](auto const& lhs, auto const& rhs) {
                if (lhs.item.type != rhs.item.type)
                    return lhs.item.type > rhs.item.type;
                return lhs.item.path.filename().string() < rhs.item.path.filename().string();
            });
        }

        Implementation(Settings settings)
            : settings{std::move(settings)}
        {}
    };

    FileGrid::FileGrid(Settings settings)
        : impl_(std::make_unique<Implementation>(std::move(settings)))
    {}
    FileGrid::~FileGrid() = default;
    FileGrid::FileGrid(FileGrid&&) = default;
    FileGrid& FileGrid::operator=(FileGrid&&) = default;

    void FileGrid::items(const std::vector<FileGrid::Item>& items, bool sorted)
    {
        impl_->items.value().clear();
        std::transform(items.begin(), items.end(), std::back_inserter(impl_->items.value()), [](auto const& item) {
            return ItemWithInternals{item};
        });
        if (sorted)
            impl_->sortItems();

        impl_->items.modifyNow();
    }

    void FileGrid::flavor(FileGridFlavor value)
    {
        impl_->flavor = value;
        Nui::globalEventContext.executeActiveEventsImmediately();
    }
    FileGridFlavor FileGrid::flavor() const
    {
        return impl_->flavor.value();
    }

    void FileGrid::onDelete(std::function<void(std::vector<Item> const&)> const& callback)
    {
        impl_->onDelete = callback;
    }
    void FileGrid::onRename(std::function<void(Item const&)> const& callback)
    {
        impl_->onRename = callback;
    }
    void FileGrid::onDownload(std::function<void(std::vector<Item> const&)> const& callback)
    {
        impl_->onDownload = callback;
    }
    void FileGrid::onUpload(
        std::function<void(std::filesystem::path desinationDir, std::vector<Item> const&)> const& callback)
    {
        impl_->onUpload = callback;
    }
    void FileGrid::onError(std::function<void(std::string const&)> const& callback)
    {
        impl_->onError = callback;
    }

    void FileGrid::iconSize(unsigned int value)
    {
        impl_->iconSize = value;
        Nui::globalEventContext.executeActiveEventsImmediately();
    }
    unsigned int FileGrid::iconSize() const
    {
        return impl_->iconSize.value();
    }
    void FileGrid::iconSpacing(unsigned int value)
    {
        impl_->iconSpacing = value;
        Nui::globalEventContext.executeActiveEventsImmediately();
    }
    unsigned int FileGrid::iconSpacing() const
    {
        return impl_->iconSpacing.value();
    }

    Nui::ElementRenderer FileGrid::tableFlavor()
    {
        using namespace Nui;
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        return Nui::Elements::div{}();
    }

    void FileGrid::deselectAll(bool rerender)
    {
        for (auto& item : impl_->items.value())
            *item.selected = false;
        if (rerender)
            Nui::globalEventContext.executeActiveEventsImmediately();
    }

    void FileGrid::selectAll(bool rerender)
    {
        for (auto& item : impl_->items.value())
            *item.selected = true;
        if (rerender)
            Nui::globalEventContext.executeActiveEventsImmediately();
    }

    void FileGrid::onUneventfulClick()
    {
        deselectAll();
        closeMenus();
    }

    std::vector<FileGrid::Item> FileGrid::selectedItems() const
    {
        std::vector<Item> result{};
        for (auto const& item : impl_->items.value())
        {
            if (item.selected->value())
                result.push_back(item.item);
        }
        return result;
    }

    void FileGrid::closeMenus()
    {
        impl_->newItemMenu.close();
        impl_->sortMenu.close();
        impl_->viewMenu.close();

        if (const auto menu = impl_->contextMenuView.lock(); menu)
            menu->val()["style"].set("display", "none");
    }

    void FileGrid::onActivateItem(std::function<void(Item const&)> const& callback)
    {
        impl_->onActivateItem = callback;
    }

    void FileGrid::onContextMenu(std::optional<Item> const& item, Nui::val event)
    {
        using namespace std::string_literals;

        event.call<void>("stopPropagation");
        event.call<void>("preventDefault");
        if (const auto menu = impl_->contextMenuView.lock(); menu)
        {
            const int targetOffsetTop = event["target"]["offsetTop"].as<int>();
            const int targetOffsetLeft = event["target"]["offsetLeft"].as<int>();
            const int offsetY = event["offsetY"].as<int>();
            const int offsetX = event["offsetX"].as<int>();

            const auto left = targetOffsetLeft + offsetX;
            const auto top = targetOffsetTop + offsetY;

            if (item)
            {
                Nui::Console::log("Context menu item: ", item->path.string());
                auto selected = selectedItems();
                if (std::find_if(selected.begin(), selected.end(), [&item](auto const& i) {
                        return i.path == item->path;
                    }) == selected.end())
                {
                    deselectAll();
                    impl_->contextMenuClickItems = {item.value()};
                }

                impl_->contextMenuClickItems = selected;
            }
            else
            {
                Nui::Console::log("Context menu item: none");
                impl_->contextMenuClickItems = selectedItems();
            }
            // filter ".." from context menu click items:
            impl_->contextMenuClickItems.erase(
                std::remove_if(
                    impl_->contextMenuClickItems.begin(),
                    impl_->contextMenuClickItems.end(),
                    [](auto const& item) {
                        return item.path.filename() == "..";
                    }),
                impl_->contextMenuClickItems.end());

            // contextMenuClickItems

            menu->val()["style"].set("display", "block");
            menu->val()["style"].set("top", std::to_string(top) + "px");
            menu->val()["style"].set("left", std::to_string(left) + "px");
            return;
        }
    }

    void FileGrid::path(std::filesystem::path const& path)
    {
        impl_->path = path;
        // Do NOT call onPathChange here. That is intentional.
    }

    std::vector<std::filesystem::path> FileGrid::selectedPaths() const
    {
        std::vector<Item> selectedItems = this->selectedItems();
        std::vector<std::filesystem::path> result(selectedItems.size());
        std::transform(selectedItems.begin(), selectedItems.end(), result.begin(), [](auto const& item) {
            return item.path;
        });
        return result;
    }

    void FileGrid::onPathChange(std::function<void(std::filesystem::path const&)> const& callback)
    {
        impl_->onPathChange = callback;
    }

    void FileGrid::onRefresh(std::function<void()> const& callback)
    {
        impl_->onRefresh = callback;
    }

    void FileGrid::onProperties(std::function<void(Item const&)> const& callback)
    {
        impl_->onProperties = callback;
    }

    Nui::ElementRenderer FileGrid::contextMenu()
    {
        using namespace Nui;
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        // clang-format off
        return div{
            reference = impl_->contextMenuView,
            class_ = "nui-file-grid-context-menu",
        }(
            div{
                class_ = "nui-file-grid-context-menu-item",
                onClick = [this](Nui::val event){
                    event.call<void>("stopPropagation");
                    closeMenus();
                    if (impl_->contextMenuClickItems.empty())
                        impl_->onError("No items selected"s);
                    if (impl_->onDownload)
                        impl_->onDownload(impl_->contextMenuClickItems);
                    impl_->contextMenuClickItems = {};
                }
            }(
                "Download"
            ),
            div{
                class_ = "nui-file-grid-context-menu-item",
                onClick = [this](Nui::val event){
                    event.call<void>("stopPropagation");
                    closeMenus();
                    if (impl_->contextMenuClickItems.empty())
                        impl_->onError("No items selected"s);
                    if (impl_->onDelete)
                        impl_->onDelete(impl_->contextMenuClickItems);
                    impl_->contextMenuClickItems = {};
                }
            }(
                "Delete"
            ),
            div{
                class_ = "nui-file-grid-context-menu-item",
                onClick = [this](Nui::val event){
                    event.call<void>("stopPropagation");
                    closeMenus();
                    const auto& items = impl_->contextMenuClickItems;
                    if (items.empty())
                        impl_->onError("No items selected"s);
                    if (items.size() > 1 && impl_->onError) {
                        impl_->onError("Cannot rename multiple items at once"s);
                    } else if (items.size() == 1) {
                        if (impl_->onRename)
                            impl_->onRename(items.front());
                    }
                    impl_->contextMenuClickItems = {};
                }
            }(
                "Rename"
            ),
            div{
                class_ = "nui-file-grid-context-menu-item",
                onClick = [this](Nui::val event){
                    event.call<void>("stopPropagation");
                    closeMenus();
                    auto const& items = impl_->contextMenuClickItems;
                    if (items.empty())
                        impl_->onError("No items selected"s);
                    if (items.size() > 1 && impl_->onError) {
                        impl_->onError("Cannot view properties of multiple items at once"s);
                    } else if (items.size() == 1) {
                        if (impl_->onProperties)
                            impl_->onProperties(items.front());
                    }
                    impl_->contextMenuClickItems = {};
                }
            }(
                "Properties"
            )
        );
        // clang-format on
    }

    Nui::ElementRenderer FileGrid::iconFlavor()
    {
        using namespace Nui;
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        // clang-format off
        return div {
            class_ = "nui-file-grid-icons",
            style = Style{
                "grid-template-columns"_style = observe(impl_->iconSize, impl_->iconSpacing).generate([this]() {
                    return "repeat(auto-fill, minmax(" + std::to_string(impl_->iconSize.value() + impl_->iconSpacing.value()) +
                        "px, 1fr))";
                })
            },
            onClick = [this](Nui::val) {
                onUneventfulClick();
            },
            "contextmenu"_event = [this](Nui::val event) {
                onContextMenu(std::nullopt, event);
            }
        }(
            impl_->items.map([this](auto, auto const& item){
                return div{
                    class_ = observe(item.selected).generate([&item](){
                        if (item.selected->value())
                            return "nui-file-grid-item-icons selected";
                        return "nui-file-grid-item-icons";
                    }),
                    onDblClick = [this, &item](Nui::val event){
                        event.call<void>("stopPropagation");
                        closeMenus();
                        if (impl_->onActivateItem)
                            impl_->onActivateItem(item.item);
                    },
                    "contextmenu"_event = [this, item = item.item](Nui::val event){
                        onContextMenu(item, event);
                    },
                    onClick = [this, &item](Nui::val event){
                        event.call<void>("stopPropagation");
                        closeMenus();

                        if (event["ctrlKey"].as<bool>())
                        {
                            *item.selected = !item.selected->value();
                        }
                        else if (event["shiftKey"].as<bool>())
                        {
                            // find self:
                            auto self = std::find_if(impl_->items.value().begin(), impl_->items.value().end(), [&item](auto const& i){
                                return i.item.path == item.item.path;
                            });

                            // go backwards and forwards until we find another selected item:
                            auto forwardSeeker = self + 1;
                            auto backwardSeeker = self - 1;
                            decltype(self) otherSelected = impl_->items.value().end();

                            if (self == impl_->items.value().end())
                                return;
                            if (self == impl_->items.value().begin())
                                backwardSeeker = impl_->items.value().begin();
                            else if (self == impl_->items.value().end() - 1)
                                forwardSeeker = impl_->items.value().end() - 1;

                            while (forwardSeeker != impl_->items.value().end() && backwardSeeker != impl_->items.value().begin())
                            {
                                if (forwardSeeker->selected->value())
                                {
                                    otherSelected = forwardSeeker;
                                    break;
                                }
                                if (backwardSeeker->selected->value())
                                {
                                    otherSelected = backwardSeeker;
                                    break;
                                }
                                ++forwardSeeker;
                                --backwardSeeker;
                            }

                            if (otherSelected == impl_->items.value().end() && forwardSeeker != impl_->items.value().end())
                            {
                                for (;forwardSeeker != impl_->items.value().end(); ++forwardSeeker)
                                {
                                    if (forwardSeeker->selected->value())
                                    {
                                        otherSelected = forwardSeeker;
                                        break;
                                    }
                                }
                            }
                            else if (otherSelected == impl_->items.value().end() && backwardSeeker != impl_->items.value().begin())
                            {
                                for (;backwardSeeker != impl_->items.value().begin(); --backwardSeeker)
                                {
                                    if (backwardSeeker->selected->value())
                                    {
                                        otherSelected = backwardSeeker;
                                        break;
                                    }
                                }
                                if (backwardSeeker == impl_->items.value().begin() && otherSelected == impl_->items.value().end() && backwardSeeker->selected->value())
                                    otherSelected = impl_->items.value().begin();
                            }

                            for (auto& item : impl_->items.value())
                                *item.selected = false;
                            if (otherSelected != impl_->items.value().end())
                            {
                                auto start = std::min(self, otherSelected);
                                auto end = std::max(self, otherSelected);
                                auto offset = (end != impl_->items.value().end()) ? 1 : 0;
                                for (auto it = start; it != end + offset; ++it)
                                    *it->selected = true;
                            }
                            else
                            {
                                selectAll();
                            }
                        }
                        else
                        {
                            deselectAll();
                            *item.selected = true;
                        }
                    }
                }(
                    img{
                        src = item.item.icon,
                        alt = "???",
                        width = observe(impl_->iconSize).generate([this](){
                            return std::to_string(impl_->iconSize.value());
                        }),
                        height = observe(impl_->iconSize).generate([this](){
                            return std::to_string(impl_->iconSize.value());
                        }),
                        style = item.item.type == FileGrid::Item::Type::Directory ? "filter: hue-rotate(120deg)" : "filter: invert(100%) brightness(2)",
                    }(),
                    div{
                    }(item.item.path.filename().string())
                );
            })
        );
        // clang-format on
    }

    Nui::ElementRenderer FileGrid::operator()(std::vector<Nui::Attribute>&& attributes)
    {
        using namespace std::string_literals;
        using namespace Nui;
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        attributes.emplace_back(onClick = [this](Nui::val) {
            onUneventfulClick();
        });
        attributes.emplace_back(class_ = "nui-file-grid");

        // clang-format off
        return div {
            std::move(attributes)
        }(
            [this]() -> Nui::ElementRenderer {
                if (impl_->settings.pathBarOnTop)
                    return pathBar();
                return nil();
            }(),
            headMenu(),
            div{
                style = "width: 100%; flex-grow: 1; position: relative; overflow-y: scroll"
            }(
                contextMenu(),
                div{
                    style = "width: 100%; height: 100%",
                    "contextmenu"_event = [this](Nui::val event) {
                        onContextMenu(std::nullopt, event);
                    }
                }(
                    observe(impl_->flavor),
                    [this]() -> Nui::ElementRenderer {
                        if (impl_->flavor.value() == FileGridFlavor::Icons)
                            return iconFlavor();
                        if (impl_->flavor.value() == FileGridFlavor::Table)
                            return tableFlavor();
                        return div{}();
                    }
                )
            ),
            [this]() -> Nui::ElementRenderer {
                if (!impl_->settings.pathBarOnTop)
                    return pathBar();
                return nil();
            }()
        );
    }

    void FileGrid::onNewItem(std::function<void(Item::Type)> const& callback)
    {
        impl_->onNewItem = callback;
    }

    Nui::ElementRenderer FileGrid::headMenu()
    {
        using namespace Nui;
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        // clang-format off
        return div {
            class_ = "nui-file-grid-head"
        }(
            impl_->newItemMenu(),
            impl_->sortMenu(),
            impl_->viewMenu(),
            filter()
        );
        // clang-format on
    }

    Nui::ElementRenderer FileGrid::pathBar()
    {
        using namespace Nui;
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        // clang-format off
        return div {
            class_ = "nui-file-grid-path-bar",
            style = [this](){
                if (impl_->settings.pathBarOnTop)
                    return "border-bottom: 1px solid var(--nui-file-grid-border-color)";
                return "border-top: 1px solid var(--nui-file-grid-border-color)";
            }()
        }(
            div{
                /*refresh*/
                onClick = [this](){
                    if (impl_->onRefresh)
                        impl_->onRefresh();
                }
            }(),
            input{
                type = "text",
                "value"_prop = observe(impl_->path).generate([this](){
                    return impl_->path->generic_string();
                }),
                "keyup"_event = [this](Nui::val event){
                    if (event["key"].as<std::string>() == "Enter")
                    {
                        if (impl_->onPathChange)
                            impl_->onPathChange(std::filesystem::path{event["target"]["value"].as<std::string>()});
                    }
                }
            }()
        );
        // clang-format on
    }

    Nui::ElementRenderer FileGrid::filter()
    {
        using namespace Nui;
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        // clang-format off
        return div{
            class_ = "nui-file-grid-filter"
        }(
            img{
                src = "data:image/svg+xml,%3C%3Fxml version='1.0' %3F%3E%3C!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 1.1//EN' 'http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd'%3E%3Csvg height='512px' id='Layer_1' style='enable-background:new 0 0 512 512;' version='1.1' viewBox='0 0 512 512' width='512px' xml:space='preserve' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'%3E%3Cpath d='M344.5,298c15-23.6,23.8-51.6,23.8-81.7c0-84.1-68.1-152.3-152.1-152.3C132.1,64,64,132.2,64,216.3 c0,84.1,68.1,152.3,152.1,152.3c30.5,0,58.9-9,82.7-24.4l6.9-4.8L414.3,448l33.7-34.3L339.5,305.1L344.5,298z M301.4,131.2 c22.7,22.7,35.2,52.9,35.2,85c0,32.1-12.5,62.3-35.2,85c-22.7,22.7-52.9,35.2-85,35.2c-32.1,0-62.3-12.5-85-35.2 c-22.7-22.7-35.2-52.9-35.2-85c0-32.1,12.5-62.3,35.2-85c22.7-22.7,52.9-35.2,85-35.2C248.5,96,278.7,108.5,301.4,131.2z'/%3E%3C/svg%3E",
                alt = "Filter",
                style = "filter: invert(100%) sepia(3%) saturate(183%) hue-rotate(281deg) brightness(120%) contrast(100%)",
            }(),
            input{
                type = "text",
                placeHolder = "Filter"
            }()
        );
        // clang-format on
    }
}