#include <nui-file-explorer/file_grid.hpp>

#include <nui/event_system/observed_value.hpp>

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

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
        }(
            impl_->items.map([this](auto, auto const& item){
                return div{
                    class_ = observe(item.selected).generate([&item](){
                        if (item.selected->value())
                            return "nui-file-grid-item-icons selected";
                        return "nui-file-grid-item-icons";
                    }),
                    onClick = [this, &item](){

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

        attributes.push_back(style = "display: flex");

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
            div{
                class_ = "nui-file-grid-head-dropdown"
            }(
                div{
                    class_ = "nui-file-grid-head-dropdown-text"
                }(
                    "New"
                ),
                div{
                    class_ = "nui-file-grid-head-dropdown-button"
                }(
                    div{
                        class_ = "nui-file-grid-head-dropdown-button-arrow"
                    }()
                )
            ),
            div{
                class_ = "nui-file-grid-head-dropdown"
            }(
                "Sort"
            ),
            div{
                class_ = "nui-file-grid-head-dropdown"
            }(
                "View"
            ),
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
                src = "data:image/svg+xml;base64,data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiA/PjwhRE9DVFlQRSBzdmcgIFBVQkxJQyAnLS8vVzNDLy9EVEQgU1ZHIDEuMS8vRU4nICAnaHR0cDovL3d3dy53My5vcmcvR3JhcGhpY3MvU1ZHLzEuMS9EVEQvc3ZnMTEuZHRkJz48c3ZnIGhlaWdodD0iNTEycHgiIGlkPSJMYXllcl8xIiBzdHlsZT0iZW5hYmxlLWJhY2tncm91bmQ6bmV3IDAgMCA1MTIgNTEyOyIgdmVyc2lvbj0iMS4xIiB2aWV3Qm94PSIwIDAgNTEyIDUxMiIgd2lkdGg9IjUxMnB4IiB4bWw6c3BhY2U9InByZXNlcnZlIiB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHhtbG5zOnhsaW5rPSJodHRwOi8vd3d3LnczLm9yZy8xOTk5L3hsaW5rIj48cGF0aCBkPSJNMzQ0LjUsMjk4YzE1LTIzLjYsMjMuOC01MS42LDIzLjgtODEuN2MwLTg0LjEtNjguMS0xNTIuMy0xNTIuMS0xNTIuM0MxMzIuMSw2NCw2NCwxMzIuMiw2NCwyMTYuMyAgYzAsODQuMSw2OC4xLDE1Mi4zLDE1Mi4xLDE1Mi4zYzMwLjUsMCw1OC45LTksODIuNy0yNC40bDYuOS00LjhMNDE0LjMsNDQ4bDMzLjctMzQuM0wzMzkuNSwzMDUuMUwzNDQuNSwyOTh6IE0zMDEuNCwxMzEuMiAgYzIyLjcsMjIuNywzNS4yLDUyLjksMzUuMiw4NWMwLDMyLjEtMTIuNSw2Mi4zLTM1LjIsODVjLTIyLjcsMjIuNy01Mi45LDM1LjItODUsMzUuMmMtMzIuMSwwLTYyLjMtMTIuNS04NS0zNS4yICBjLTIyLjctMjIuNy0zNS4yLTUyLjktMzUuMi04NWMwLTMyLjEsMTIuNS02Mi4zLDM1LjItODVjMjIuNy0yMi43LDUyLjktMzUuMiw4NS0zNS4yQzI0OC41LDk2LDI3OC43LDEwOC41LDMwMS40LDEzMS4yeiIvPjwvc3ZnPg==",
                alt = "Filter"
            }(),
            input{

                type = "text",
                placeHolder = "Filter"
            }()
        );
        // clang-format on
    }
}