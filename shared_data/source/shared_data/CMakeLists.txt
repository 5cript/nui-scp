add_library(
    shared_data
    STATIC
        directory_entry.cpp
        file_operations/operation_type.cpp
        file_operations/operation_added.cpp
        file_operations/download_progress.cpp
        file_operations/operation_completed.cpp
        file_operations/operation_error.cpp
        file_operations/operation_state.cpp
)

target_include_directories(shared_data PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../include" "${CMAKE_CURRENT_SOURCE_DIR}/../../../ssh/include")

target_link_libraries(shared_data PUBLIC utility ids fmt)

if (EMSCRIPTEN)
    target_link_libraries(shared_data PUBLIC nui-frontend)
else()
    target_link_libraries(shared_data PUBLIC nui-backend)
endif()