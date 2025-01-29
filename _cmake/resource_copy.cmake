# Icons
add_custom_command(
    OUTPUT
        "${CMAKE_BINARY_DIR}/bin/assets/icons/folder_main.png"
    COMMAND
        ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/../dependencies/OS-Folder-Icons/images/os_folders/windows/11/folder_main.png" "${CMAKE_BINARY_DIR}/bin/assets/icons/folder_main.png"
    DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/../dependencies/OS-Folder-Icons/images/os_folders/windows/11/folder_main.png"
)

add_custom_target(
    scp-resource-copy
    ALL
    DEPENDS
        "${CMAKE_BINARY_DIR}/bin/assets/icons/folder_main.png"
)