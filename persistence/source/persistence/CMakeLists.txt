add_library(
    persistence
    STATIC
        state/terminal_engine.cpp
        state/common_terminal_options.cpp
        state_holder.cpp
        state.cpp
)

if (EMSCRIPTEN)
    target_sources(
        persistence
        PRIVATE
            state_holder_frontend.cpp
    )
else()
    target_sources(
        persistence
        PRIVATE
            state_holder_backend.cpp
    )
endif()

target_include_directories(
    persistence
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(
    persistence
    PUBLIC
        constants
        utility
        log
        roar-include-only
        core-target
)