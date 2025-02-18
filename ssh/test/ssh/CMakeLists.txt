include(GoogleTest)

add_executable(
    nui-scp-ssh-test
    main.cpp
    utility/node.cpp
)

find_package(Boost CONFIG REQUIRED COMPONENTS filesystem asio system process)

target_link_libraries(
    nui-scp-ssh-test
    PUBLIC
        secure-shell
        GTest::GTest
        GTest::Main
        Boost::filesystem
        Boost::asio
        Boost::system
        Boost::process
)

if (WIN32)
    target_link_libraries(nui-scp-ssh-test PUBLIC ws2_32)
    find_program(CMD cmd)
    target_compile_definitions(nui-scp-ssh-test PRIVATE -DCMD_EXECUTABLE="${CMD}" -DMSYS2_BASH="${MSYS2_BASH}")
endif()

set_target_properties(nui-scp-ssh-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")

find_program(NODE node)
find_program(NPM npm)

target_compile_definitions(nui-scp-ssh-test PRIVATE -DNODE_EXECUTABLE="${NODE}" -DNPM_EXECUTABLE="${NPM}")

add_custom_command(
    OUTPUT
        "${CMAKE_CURRENT_LIST_DIR}/test_ssh2_servers/dist/shell.mjs"
        "${CMAKE_CURRENT_LIST_DIR}/test_ssh2_servers/dist/sftp.mjs"
    COMMAND ${NPM} run build
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/test_ssh2_servers"
    DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/test_ssh2_servers/package.json"
        "${CMAKE_CURRENT_LIST_DIR}/test_ssh2_servers/package-lock.json"
)
add_custom_target(
    build_test_ssh2_servers
    DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/test_ssh2_servers/dist/shell.mjs"
        "${CMAKE_CURRENT_LIST_DIR}/test_ssh2_servers/dist/sftp.mjs"
)
add_dependencies(nui-scp-ssh-test build_test_ssh2_servers)

gtest_discover_tests(nui-scp-ssh-test)

# If msys2, copy dynamic libraries to executable directory, visual studio does this automatically.
# And there is no need on linux.
if (DEFINED ENV{MSYSTEM})
    add_custom_command(TARGET nui-scp-ssh-test POST_BUILD
        COMMAND bash -c "ldd $<TARGET_FILE:nui-scp-ssh-test>" | "grep" "clang" | awk "NF == 4 { system(\"${CMAKE_COMMAND} -E copy \" \$3 \" $<TARGET_FILE_DIR:nui-scp-ssh-test>\") }"
        VERBATIM
    )
endif()