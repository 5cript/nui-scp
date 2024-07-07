target_sources(
    ${PROJECT_NAME}
    PRIVATE
        main.cpp
        process/process.cpp
        process/process_store.cpp
        process/environment.cpp
)

if (WIN32)
    target_sources(
        ${PROJECT_NAME}
        PRIVATE
            pty/windows/conpty.cpp
            pty/windows/pipe.cpp
    )
else()
endif()

find_package(Boost CONFIG 1.81.0 REQUIRED COMPONENTS system filesystem)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/backend/include")

target_compile_definitions(${PROJECT_NAME} PRIVATE -DBOOST_PROCESS_V2_SEPARATE_COMPILATION)

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