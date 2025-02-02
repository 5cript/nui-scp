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

        FileGrid();
        ~FileGrid();
        FileGrid(const FileGrid&) = delete;
        FileGrid& operator=(const FileGrid&) = delete;
        FileGrid(FileGrid&&);
        FileGrid& operator=(FileGrid&&);

        /**
         * @brief Sets the items to be displayed in the grid.
         */
        void items(const std::vector<Item>& items, bool sorted = true);

        /**
         * @brief Determines how the grid should be displayed.
         */
        void flavor(FileGridFlavor value);
        FileGridFlavor flavor() const;

        /**
         * @brief Sets the size of the icons in the grid.
         */
        void iconSize(unsigned int value);
        unsigned int iconSize() const;

        void deselectAll(bool rerender = false);

        void selectAll(bool rerender = false);

        Nui::ElementRenderer operator()(std::vector<Nui::Attribute>&& attributes = {});

      private:
        Nui::ElementRenderer iconFlavor();
        Nui::ElementRenderer tableFlavor();
        Nui::ElementRenderer headMenu();
        Nui::ElementRenderer filter();

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}