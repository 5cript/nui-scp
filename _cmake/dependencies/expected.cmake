include(FetchContent)
FetchContent_Declare(
    expected
    GIT_REPOSITORY https://github.com/TartanLlama/expected.git
    GIT_TAG        3f0ca7b19253129700a073abfa6d8638d9f7c80c
)

FetchContent_MakeAvailable(expected)