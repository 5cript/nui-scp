add_library(
    shared_data
    STATIC
        directory_entry.cpp
)

target_include_directories(shared_data PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../include" "${CMAKE_CURRENT_SOURCE_DIR}/../../../ssh/include")

target_link_libraries(shared_data PUBLIC utility ids fmt)

if (EMSCRIPTEN)
    target_link_libraries(shared_data PUBLIC nui-frontend)
else()
    target_link_libraries(shared_data PUBLIC nui-backend)
endif()