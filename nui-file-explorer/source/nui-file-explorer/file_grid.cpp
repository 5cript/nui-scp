#include "nui/event_system/event_context.hpp"
#include <nui-file-explorer/file_grid.hpp>
#include <nui-file-explorer/dropdown_menu.hpp>

#include <nui/event_system/observed_value.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/frontend/api/console.hpp>

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
        DropdownMenu newItemMenu{
            {
                "File",
                "Folder",
                // Soft Link ?
                // Hard Link ?
            },
            [](std::string const& item) {
                Nui::Console::log("New clicked: ", item);
                // TODO:
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
                    auto& items = this->items.value();
                    std::sort(items.begin(), items.end(), [](auto const& lhs, auto const& rhs) {
                        if (lhs.item.isDirectory != rhs.item.isDirectory)
                            return lhs.item.isDirectory > rhs.item.isDirectory;
                        return lhs.item.path.filename().string() < rhs.item.path.filename().string();
                    });
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
    };

    FileGrid::FileGrid()
        : impl_(std::make_unique<Implementation>())
    {}
    FileGrid::~FileGrid() = default;
    FileGrid::FileGrid(FileGrid&&) = default;
    FileGrid& FileGrid::operator=(FileGrid&&) = default;

    void FileGrid::items(const std::vector<FileGrid::Item>& items)
    {
        std::transform(items.begin(), items.end(), std::back_inserter(impl_->items.value()), [](auto const& item) {
            return ItemWithInternals{item};
        });

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

    void FileGrid::iconSize(unsigned int value)
    {
        impl_->iconSize = value;
        Nui::globalEventContext.executeActiveEventsImmediately();
    }
    unsigned int FileGrid::iconSize() const
    {
        return impl_->iconSize.value();
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
                "grid-template-columns"_style = observe(impl_->iconSize).generate([this]() {
                    return "repeat(auto-fill, minmax(" + std::to_string(impl_->iconSize.value()) +
                        "px, 1fr))";
                })
            },
            onClick = [this](Nui::val) {
                deselectAll();
            }
        }(
            impl_->items.map([this](auto, auto const& item){
                return div{
                    class_ = observe(item.selected).generate([&item](){
                        if (item.selected->value())
                            return "nui-file-grid-item-icons selected";
                        return "nui-file-grid-item-icons";
                    }),
                    onClick = [this, &item](Nui::val event){
                        event.call<void>("stopPropagation");

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
                                *item.selected = true;
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
                        })
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
        using namespace Nui;
        using namespace Nui::Elements;
        using namespace Nui::Attributes;
        using Nui::Elements::div;

        // clang-format off
        return div {
            std::move(attributes)
        }(
            headMenu(),
            div{
                style = "width: 100%; height: 100%; flex-grow: 1"
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
        );
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