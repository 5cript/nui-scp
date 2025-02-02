set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use an external fmt library (provide it manually)" FORCE)

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        ae1de0dc8cf480f54eaa425c4a9dc4fef29b28ba
)

FetchContent_MakeAvailable(spdlog)
