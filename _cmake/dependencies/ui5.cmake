include(FetchContent)

FetchContent_Declare(
    ui5
    GIT_REPOSITORY https://github.com/NuiCpp/ui5.git
    GIT_TAG        main
)

FetchContent_MakeAvailable(ui5)