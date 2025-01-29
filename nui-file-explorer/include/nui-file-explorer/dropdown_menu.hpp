#pragma once

#include <nui/frontend/element_renderer.hpp>

#include <vector>
#include <string>
#include <functional>

namespace NuiFileExplorer
{
    class DropdownMenu
    {
      public:
        DropdownMenu(
            std::vector<std::string> items,
            std::function<void(std::string const&)> callback,
            std::function<void()> onOpen,
            std::string title);
        ~DropdownMenu();
        DropdownMenu(const DropdownMenu&) = delete;
        DropdownMenu& operator=(const DropdownMenu&) = delete;
        DropdownMenu(DropdownMenu&&) noexcept;
        DropdownMenu& operator=(DropdownMenu&&) noexcept;

        void items(const std::vector<std::string>& items);

        Nui::ElementRenderer operator()(std::vector<Nui::Attribute>&& attributes = {}) const;

        void open();
        void close();
        bool isOpen() const;

      private:
        struct Implementation;
        std::unique_ptr<Implementation> impl_;
    };
}