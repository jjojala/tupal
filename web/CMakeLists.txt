find_program(NODE_EXECUTABLE NAMES node REQUIRED)
find_program(NPM_EXECUTABLE NAMES npm REQUIRED)

get_filename_component(NODE_BIN_DIR "${NODE_EXECUTABLE}" DIRECTORY)
if(WIN32)
    set(PATH_SEPARATOR "\\;")
else()
    set(PATH_SEPARATOR ":")
endif()
set(FRONTEND_DEPENDENCIES_STAMP "${CMAKE_BINARY_DIR}/frontend-dependencies.stamp")
set(FRONTEND_OUTPUT_DIR "${TUPAL_WEB_OUTPUT_DIR}")

file(GLOB_RECURSE FRONTEND_SOURCE_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*")
file(GLOB_RECURSE WEB_DOCUMENTATION_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ts")

set(FRONTEND_PACKAGE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/package.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/package-lock.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/.nvmrc"
    "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
    "${CMAKE_CURRENT_SOURCE_DIR}/svelte.config.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/tsconfig.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/vite.config.ts")

add_custom_command(
    OUTPUT "${FRONTEND_DEPENDENCIES_STAMP}"
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=${NODE_BIN_DIR}${PATH_SEPARATOR}$ENV{PATH}"
        "${NPM_EXECUTABLE}" ci --no-audit --no-fund
    COMMAND "${CMAKE_COMMAND}" -E touch "${FRONTEND_DEPENDENCIES_STAMP}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/package.json"
            "${CMAKE_CURRENT_SOURCE_DIR}/package-lock.json"
    COMMENT "Installing locked frontend dependencies"
    VERBATIM)

add_custom_command(
    OUTPUT "${FRONTEND_OUTPUT_DIR}/index.html"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${FRONTEND_OUTPUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=${NODE_BIN_DIR}${PATH_SEPARATOR}$ENV{PATH}"
        "TUPAL_WEB_OUTPUT_DIR=${FRONTEND_OUTPUT_DIR}"
        "${NPM_EXECUTABLE}" run build
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/openapi.yaml"
        "${FRONTEND_OUTPUT_DIR}/openapi.yaml"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/swagger.html"
        "${FRONTEND_OUTPUT_DIR}/swagger.html"
    DEPENDS "${FRONTEND_DEPENDENCIES_STAMP}"
            ${FRONTEND_PACKAGE_FILES}
            ${FRONTEND_SOURCE_FILES}
            "${CMAKE_CURRENT_SOURCE_DIR}/openapi.yaml"
            "${CMAKE_CURRENT_SOURCE_DIR}/swagger.html"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Svelte frontend"
    VERBATIM)

add_custom_target(web_frontend ALL
    DEPENDS "${FRONTEND_OUTPUT_DIR}/index.html")

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    add_custom_target(web_docs
        COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile"
                ${WEB_DOCUMENTATION_FILES}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating web API documentation with Doxygen"
        VERBATIM)
endif()
