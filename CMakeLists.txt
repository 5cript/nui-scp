cmake_minimum_required(VERSION 3.21)

project(nui-scp VERSION 0.1.0)

set(NUI_FETCH_ROAR OFF CACHE BOOL "Dont fetch roar, use local copy" FORCE)

include (${CMAKE_CURRENT_LIST_DIR}/_cmake/common_options.cmake)

# Nui Dependency
add_subdirectory("${CMAKE_SOURCE_DIR}/dependencies/nui")

# Add executable (sources are added later, depending on frontend/backend)
add_executable(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PUBLIC core-target)

# Subdirectories & Dependencies
add_library(roar-include-only INTERFACE)
target_include_directories(roar-include-only INTERFACE "${CMAKE_BINARY_DIR}/../_deps/roar-src/include")
include("${CMAKE_CURRENT_LIST_DIR}/dependencies/Nui/cmake/dependencies/nlohmann_json.cmake")
add_subdirectory("${CMAKE_SOURCE_DIR}/constants/source/constants")
add_subdirectory("${CMAKE_SOURCE_DIR}/utility/source/utility")
add_subdirectory("${CMAKE_SOURCE_DIR}/log/source/log")
add_subdirectory("${CMAKE_SOURCE_DIR}/persistence/source/persistence")
include("${CMAKE_CURRENT_LIST_DIR}/_cmake/dependencies.cmake")

set(PREJS_FILE "${CMAKE_SOURCE_DIR}/frontend/source/frontend/js/module.js")

if (EMSCRIPTEN)
    add_subdirectory("${CMAKE_SOURCE_DIR}/frontend/source/frontend")
else()
	add_subdirectory("${CMAKE_SOURCE_DIR}/backend/source/backend")
    add_subdirectory("${CMAKE_SOURCE_DIR}/dependencies/roar")

    # If msys2, copy dynamic libraries to executable directory, visual studio does this automatically.
    # And there is no need on linux.
    if (DEFINED ENV{MSYSTEM})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND bash -c "ldd $<TARGET_FILE:${PROJECT_NAME}>" | "grep" "clang" | awk "NF == 4 { system(\"${CMAKE_COMMAND} -E copy \" \$3 \" $<TARGET_FILE_DIR:${PROJECT_NAME}>\") }"
            VERBATIM
        )
    endif()
endif()