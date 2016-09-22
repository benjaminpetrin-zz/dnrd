set(VERSION_FILE "${CMAKE_BINARY_DIR}/gen/generated/version.h")

add_custom_target(
    version
    COMMAND mkdir -p "${CMAKE_BINARY_DIR}/gen/generated"
    COMMAND echo "#pragma once" > "${VERSION_FILE}.new"
    COMMAND sh -c "echo \"#define PACKAGE_VERSION \\\"$(git --git-dir=./.git describe --dirty 2>/dev/null || echo ${PACKAGE_VERSION})\\\"\"" >> "${VERSION_FILE}.new"
    COMMAND cmp -s "${VERSION_FILE}" "${VERSION_FILE}.new" && rm "${VERSION_FILE}.new" || mv "${VERSION_FILE}.new" "${VERSION_FILE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
)
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "${VERSION_FILE}")

