add_library(events STATIC app_event_context.cpp)

target_include_directories(events 
    PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../../include")

target_link_libraries(events PUBLIC nui-events PRIVATE core-target)