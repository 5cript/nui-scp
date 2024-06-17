target_sources(
    ${PROJECT_NAME}
    PRIVATE
        main.cpp
        main_page.cpp
        sidebar.cpp
        toolbar.cpp
        session_area.cpp
        terminal/terminal.cpp
        terminal/msys2.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/frontend/include)

add_library(roar-include-only INTERFACE)
target_include_directories(roar-include-only INTERFACE "${CMAKE_BINARY_DIR}/../_deps/roar-src/include")

# Link frontend of nui inside of emscripten
target_link_libraries(${PROJECT_NAME} PRIVATE nui-frontend roar-include-only)

nui_prepare_emscripten_target(
    TARGET
        ${PROJECT_NAME}
    PARCEL_ARGS
        # --no-optimize
    PREJS
        ${PREJS_FILE}
    EMSCRIPTEN_LINK_OPTIONS
        -sEXPORTED_FUNCTIONS=_frontendMain
        -sALLOW_MEMORY_GROWTH=1
        # -O2
        -g
        # or other options, see emscripten docs.
    EMSCRIPTEN_COMPILE_OPTIONS
        # -O3
        -g
)