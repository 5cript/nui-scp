// #include "test_processing_thread.hpp"
#include "test_ssh_session.hpp"

#include "utility/node.hpp"

#include <gtest/gtest.h>

#include <filesystem>

std::filesystem::path programDirectory;

int main(int argc, char** argv)
{
    programDirectory = std::filesystem::path{argv[0]}.parent_path();

    // if (std::filesystem::exists(programDirectory / "modules"))
    //     std::filesystem::remove_all(programDirectory / "modules");
    // if (std::filesystem::exists(programDirectory / "temp"))
    //     std::filesystem::remove_all(programDirectory / "temp");

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}