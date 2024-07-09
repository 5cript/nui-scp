set(THEME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin/themes")
set(THEME_NAMES "dark")

set(THEME_OUTPUTS "")
foreach(THEME_NAME ${THEME_NAMES})
    list(APPEND THEME_OUTPUTS "${THEME_OUTPUT_DIRECTORY}/${THEME_NAME}/css_variables.css")
endforeach()

set(THEME_DEPENDS "")
foreach(THEME_NAME ${THEME_NAMES})
    list(APPEND THEME_DEPENDS "${CMAKE_SOURCE_DIR}/themes/${THEME_NAME}/theme.less")
endforeach()

add_custom_command(
    OUTPUT ${THEME_OUTPUTS}
    COMMAND ${NUI_NODE} "${CMAKE_SOURCE_DIR}/themes/compile.mjs" "${THEME_OUTPUT_DIRECTORY}"
    DEPENDS ${THEME_DEPENDS}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_custom_target(
    theme_compilation ALL
    DEPENDS ${THEME_OUTPUTS}
)