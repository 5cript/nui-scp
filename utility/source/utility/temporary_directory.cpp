#include <utility/temporary_directory.hpp>

#include <cstdlib>
#include <random>
#include <ctime>

using namespace std::string_literals;

namespace Utility
{
    namespace
    {
        [[maybe_unused]] std::string generateRandomString(int length)
        {
            std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
            std::string randomString;

            std::mt19937 rng(std::time(nullptr)); // Seed the random generator with time
            std::uniform_int_distribution<int> distribution(0, characters.length() - 1);

            for (int i = 0; i < length; ++i)
            {
                randomString += characters[distribution(rng)];
            }

            return randomString;
        }
    }

    TemporaryDirectory::TemporaryDirectory()
        : m_basePath{[]() {
            const auto temporaryDirectory = std::filesystem::temp_directory_path();
            return temporaryDirectory / "sscv2_client_tmpdir";
        }()}
        , m_path{}
    {
        if (!std::filesystem::exists(m_basePath))
            std::filesystem::create_directories(m_basePath);

#if __linux__
        std::string dirNameAsString{(m_basePath / "dirXXXXXX").string()};
        bool valid = mkdtemp(&dirNameAsString[0]) && std::filesystem::is_directory(dirNameAsString);
        if (valid)
            m_path = dirNameAsString;
#else
        int i = 0;
        for (; i != 1000; ++i)
        {
            const auto directoryName = "dir"s + generateRandomString(10);
            const auto path = m_basePath / directoryName;
            if (!std::filesystem::exists(path))
            {
                std::filesystem::create_directory(path);
                m_path = path;
                break;
            }
        }
        bool valid = i != 1000;
#endif
        if (!valid)
            throw std::runtime_error(std::string{"Could not setup temporary directory: "} + m_path.string());
    }
    TemporaryDirectory::~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
        std::filesystem::remove(m_basePath, error);
    }

    std::filesystem::path const& TemporaryDirectory::path() const
    {
        return m_path;
    }
}