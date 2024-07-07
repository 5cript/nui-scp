add_library(
    utility
    STATIC
        temporary_directory.cpp
)

target_include_directories(utility PUBLIC ${CMAKE_SOURCE_DIR}/utility/include)

include("${CMAKE_CURRENT_LIST_DIR}/../../../dependencies/nui/cmake/dependencies/nlohmann_json.cmake")

if(EMSCRIPTEN)
    # This allows for better exception handling in frontend parts.
    target_link_libraries(utility PUBLIC nui-frontend)
else()
    target_link_libraries(utility PUBLIC nui-backend)
endif()

target_link_libraries(utility PUBLIC core-target nlohmann_json::nlohmann_json)

target_compile_definitions(utility PUBLIC "JSON_DIAGNOSTICS=1")