add_library(
    nui-file-explorer
    STATIC
        "${CMAKE_CURRENT_LIST_DIR}/file_grid.cpp"
)

target_link_libraries(
    nui-file-explorer
    PRIVATE
        nui-frontend
)

target_include_directories(
    nui-file-explorer
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../../include
)