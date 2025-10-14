add_library(events STATIC app_event_context.cpp)

target_include_directories(events
    PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../../include")

target_link_libraries(events PUBLIC nui-events PRIVATE core-target)

if (EMSCRIPTEN)
    set_target_properties(
        nui-events
        PROPERTIES
            LINK_FLAGS "-sMEMORY64=1"
            COMPILE_FLAGS "-sMEMORY64=1"
    )
endif()