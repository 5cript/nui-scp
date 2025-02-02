add_library(
    shared_data
    STATIC
        directory_entry.cpp
)

target_include_directories(shared_data PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../include")

target_link_libraries(shared_data PUBLIC utility)