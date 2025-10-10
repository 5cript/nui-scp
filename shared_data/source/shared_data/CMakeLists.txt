add_library(
    shared_data
    STATIC
        directory_entry.cpp
        file_operations/operation_type.cpp
        file_operations/operation_added.cpp
)

target_include_directories(shared_data PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../include")

target_link_libraries(shared_data PUBLIC utility ids)

if (EMSCRIPTEN)
    target_link_libraries(shared_data PUBLIC nui-frontend)
    target_sources(
        shared_data
        PRIVATE
            file_operations/operation_type_val.cpp
            file_operations/operation_added_val.cpp
    )
else()
    target_link_libraries(shared_data PUBLIC nui-backend)
endif()