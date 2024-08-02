set(NUISCP_NPM_INSTALL_ARGS "" CACHE STRING "Arguments to pass to npm install for nui-scp")

target_sources(
    ${PROJECT_NAME}
    PRIVATE
        main.cpp
        main_page.cpp
        sidebar.cpp
        toolbar.cpp
        session_area.cpp
        session.cpp
        password_prompter.cpp
        terminal/terminal.cpp
        terminal/executing_engine.cpp
        terminal/ssh_engine.cpp
        terminal/user_control_engine.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/frontend/include)

# Link frontend of nui inside of emscripten
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        nui-frontend
        ui5
        roar-include-only
        utility
        log
        persistence
        nlohmann_json::nlohmann_json
        events
)

nui_prepare_emscripten_target(
    TARGET
        ${PROJECT_NAME}
    PARCEL_ARGS
        --no-optimize
    PREJS
        ${PREJS_FILE}
    LEAN_INDEX_HTML
    NPM_INSTALL_ARGS
        ${NUISCP_NPM_INSTALL_ARGS}
    EMSCRIPTEN_LINK_OPTIONS
        -sEXPORTED_FUNCTIONS=_frontendMain
        -sALLOW_MEMORY_GROWTH=1
        -fexceptions
        # -O2
        -g
        # or other options, see emscripten docs.
    EMSCRIPTEN_COMPILE_OPTIONS
        # -O3
        -g
)

add_dependencies(theme_compilation ${PROJECT_NAME}-npm-install)
add_dependencies(${PROJECT_NAME}-parcel theme_compilation)