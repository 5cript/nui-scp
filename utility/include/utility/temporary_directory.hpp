#pragma once

#include <filesystem>

namespace Utility
{
    class TemporaryDirectory
    {
      public:
        TemporaryDirectory();
        ~TemporaryDirectory();

        TemporaryDirectory(TemporaryDirectory const&) = delete;
        TemporaryDirectory& operator=(TemporaryDirectory const&) = delete;

        TemporaryDirectory(TemporaryDirectory&&) = delete;
        TemporaryDirectory& operator=(TemporaryDirectory&&) = delete;

        std::filesystem::path const& path() const;

      private:
        std::filesystem::path m_basePath;
        std::filesystem::path m_path;
    };
}