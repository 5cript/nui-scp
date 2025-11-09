#pragma once

#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/attributes/impl/attribute.hpp>

#include <memory>
#include <string>
#include <filesystem>
#include <vector>

namespace NuiFileExplorer
{
    enum class FileGridFlavor
    {
        Icons, // Customizable Size, size below certain size will flex wrap differently
        Table,
        Tiles, // Icon left, then name and details on the right in rows.
    };
    std::string fileGridFlavorToString(FileGridFlavor value);
    FileGridFlavor fileGridFlavorFromString(std::string const& value);

    class FileGrid
    {
      public:
        enum class IconSize : unsigned int
        {
            Small = 16,
            Medium = 64,
            Large = 80,
            ExtraLarge = 256
        };

        struct Item
        {
            enum class Type
            {
                Regular = 1,
                Directory = 2,
                Symlink = 3,
                Special = 4,
                Unknown = 5,
                Socket = 6,
                CharDevice = 7,
                BlockDevice = 8,
                Fifo = 9
            };

            std::filesystem::path path;
            std::string icon; // url or base64 url etc.
            Type type = Type::Unknown;
            std::filesystem::perms permissions = std::filesystem::perms::none;
            unsigned int ownerId = 0;
            unsigned int groupId = 0;
            std::uint64_t atime = 0;
            std::uint64_t size = 0;
        };

        struct Settings
        {
            bool pathBarOnTop = false;
        };

        FileGrid(Settings settings);
        ~FileGrid();
        FileGrid(const FileGrid&) = delete;
        FileGrid& operator=(const FileGrid&) = delete;
        FileGrid(FileGrid&&);
        FileGrid& operator=(FileGrid&&);

        /**
         * @brief Called when an item is double clicked or ENTER is pressed while an item is selected.
         *
         * @param callback
         */
        void onActivateItem(std::function<void(Item const&)> const& callback);

        /**
         * @brief Called when a new item is requested to be created.
         *
         * @param callback
         */
        void onNewItem(std::function<void(Item::Type)> const& callback);

        /**
         * @brief Sets the items to be displayed in the grid.
         */
        void items(const std::vector<Item>& items, bool sorted = true);

        /**
         * @brief Determines how the grid should be displayed.
         */
        void flavor(FileGridFlavor value);

        /**
         * @brief Returns the current grid flavor.
         */
        FileGridFlavor flavor() const;

        /**
         * @brief Sets the size of the icons in the grid.
         */
        void iconSize(unsigned int value);

        /**
         * @brief Sets the spacing between icons in the grid.
         */
        void iconSpacing(unsigned int value);

        /**
         * @brief Returns the size of the icons in the grid.
         */
        unsigned int iconSize() const;

        /**
         * @brief Returns the spacing between icons in the grid.
         */
        unsigned int iconSpacing() const;

        /**
         * @brief Deselects all items.
         */
        void deselectAll(bool rerender = false);

        /**
         * @brief Selects all items.
         */
        void selectAll(bool rerender = false);

        /**
         * @brief Returns all selected items.
         */
        std::vector<Item> selectedItems() const;

        /**
         * @brief Returns the paths of all selected items.
         */
        std::vector<std::filesystem::path> selectedPaths() const;

        /**
         * @brief Use this to close all menus and deselect all items.
         */
        void onUneventfulClick();

        /**
         * @brief Closes all menus without deselecting items.
         */
        void closeMenus();

        Nui::ElementRenderer operator()(std::vector<Nui::Attribute>&& attributes = {});

        /**
         * @brief Set the Path in an input box.
         */
        void path(std::filesystem::path const& path);

        /**
         * @brief Set a callback for when the user enters a new path (not triggered by the setPath method).
         *
         * @param callback Called when the user enters a new path (not triggered by the setPath method).
         */
        void onPathChange(std::function<void(std::filesystem::path const&)> const& callback);

        /**
         * @brief Set a callback for when the refresh button is clicked.
         *
         * @param callback Called when the refresh button is clicked
         */
        void onRefresh(std::function<void()> const& callback);

        /**
         * @brief Triggered when items are requested to be deleted.
         *
         * @param callback
         */
        void onDelete(std::function<void(std::vector<Item> const&)> const& callback);

        /**
         * @brief Triggered when item is requested to be renamed.
         *
         * @param callback
         */
        void onRename(std::function<void(Item const&)> const& callback);

        /**
         * @brief Triggered when item is requested to be queried for attrs.
         *
         * @param callback
         */
        void onProperties(std::function<void(Item const&)> const& callback);

        /**
         * @brief Triggered when items are requested to be downloaded.
         */
        void onDownload(std::function<void(std::vector<Item> const&)> const& callback);

        /**
         * @brief Triggered when items are requested to be uploaded / dropped onto the grid.
         */
        void
        onUpload(std::function<void(std::filesystem::path desinationDir, std::vector<Item> const&)> const& callback);

        /**
         * @brief Triggered when an error occurs.
         */
        void onError(std::function<void(std::string const&)> const& callback);

      private:
        Nui::ElementRenderer iconFlavor();
        Nui::ElementRenderer tableFlavor();
        Nui::ElementRenderer headMenu();
        Nui::ElementRenderer pathBar();
        Nui::ElementRenderer filter();
        Nui::ElementRenderer contextMenu();
        void onContextMenu(std::optional<Item> const& item, Nui::val event);

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}