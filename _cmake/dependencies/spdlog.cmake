set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use an external fmt library (provide it manually)" FORCE)

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        c3aed4b68373955e1cc94307683d44dca1515d2b
)

FetchContent_MakeAvailable(spdlog)
