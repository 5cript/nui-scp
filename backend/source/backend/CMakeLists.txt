target_sources(
    ${PROJECT_NAME}
    PRIVATE
        password/password_prompter.cpp
        process/process.cpp
        process/process_store.cpp
        process/environment.cpp
        ssh/ssh_session_manager.cpp
        main.cpp
)

if (WIN32)
    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            pty/windows/conpty.cpp
            pty/windows/pipe.cpp
    )
else()
    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            pty/linux/pty.cpp
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE -DBOOST_PROCESS_V2_SEPARATE_COMPILATION)
endif()

find_package(Boost CONFIG 1.81.0 REQUIRED COMPONENTS system filesystem)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/backend/include")

target_compile_definitions(${PROJECT_NAME} PRIVATE -DBOOST_PROCESS_USE_STD_FS=1 -DSSH_NO_CPP_EXCEPTIONS)

include("${CMAKE_CURRENT_LIST_DIR}/../../../_cmake/dependencies/libssh.cmake")

# Link backend of nui outside of emscripten
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        nui-backend
        efsw-static
        Boost::filesystem
        utility
        log
        persistence
        ssh
        events
)

nui_set_target_output_directories(${PROJECT_NAME})

# Creates a target that is compiled through emscripten. This target becomes the frontend part.
nui_add_emscripten_target(
    TARGET
        ${PROJECT_NAME}
    ENABLE_TAILWIND
    PREJS
        ${PREJS_FILE}
    SOURCE_DIR
        ${CMAKE_SOURCE_DIR}
    CMAKE_OPTIONS
        # I recommend to work with a release build by default because debug builds get big fast.
        -DCMAKE_BUILD_TYPE=Release
)