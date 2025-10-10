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
    set_target_properties(
        fmt
        PROPERTIES
            LINK_FLAGS "-sMEMORY64=1"
            COMPILE_FLAGS "-sMEMORY64=1"
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