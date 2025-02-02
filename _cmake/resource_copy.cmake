# Icons
add_custom_command(
    OUTPUT
        "${CMAKE_BINARY_DIR}/bin/assets/icons/folder_main.png"
        "${CMAKE_BINARY_DIR}/bin/assets/icons/file.png"
        "${CMAKE_BINARY_DIR}/bin/assets/icons/hard_drive.png"
        "${CMAKE_BINARY_DIR}/bin/assets/icons/cpp_file.png"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/../dependencies/OS-Folder-Icons/images/os_folders/windows/11/folder_main.png" "${CMAKE_BINARY_DIR}/bin/assets/icons/folder_main.png"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/../dependencies/OS-Folder-Icons/images/masks/Others/hard-drive.png" "${CMAKE_BINARY_DIR}/bin/assets/icons/hard_drive.png"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/../static/assets/file.png" "${CMAKE_BINARY_DIR}/bin/assets/icons/file.png"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/../dependencies/OS-Folder-Icons/images/masks/Development/noun-c-4921409.png" "${CMAKE_BINARY_DIR}/bin/assets/icons/cpp_file.png"
    DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/../dependencies/OS-Folder-Icons/images/os_folders/windows/11/folder_main.png"
        "${CMAKE_CURRENT_LIST_DIR}/../static/assets/file.png"
        "${CMAKE_CURRENT_LIST_DIR}/../dependencies/OS-Folder-Icons/images/masks/Others/hard-drive.png"
)

add_custom_target(
    scp-resource-copy
    ALL
    DEPENDS
        "${CMAKE_BINARY_DIR}/bin/assets/icons/folder_main.png"
)