add_library(
    log
    STATIC
        log.cpp
)
target_include_directories(log PUBLIC "${CMAKE_SOURCE_DIR}/log/include")

if (EMSCRIPTEN)
    target_link_libraries(
        log
        PUBLIC
            core-target
            utility
            fmt
            nui-frontend
    )
else()
    target_link_libraries(
        log
        PUBLIC
            core-target
            utility
            spdlog::spdlog
            nui-backend
    )
endif()