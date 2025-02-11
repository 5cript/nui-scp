add_library(
    secure-shell
    STATIC
        processing_thread.cpp
)

target_include_directories(
    secure-shell
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(
    secure-shell
    PUBLIC
        ssh
)