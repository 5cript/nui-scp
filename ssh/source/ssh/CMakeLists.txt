add_library(
    secure-shell
    STATIC
        processing_thread.cpp
        session.cpp
        channel.cpp
)

target_include_directories(
    secure-shell
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_definitions(secure-shell PRIVATE -DSSH_NO_CPP_EXCEPTIONS)

target_link_libraries(
    secure-shell
    PUBLIC
        ssh
        ids
        persistence
        fmt
)