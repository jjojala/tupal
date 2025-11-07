cmake_minimum_required(VERSION 3.21)

set(VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)

project(tupal LANGUAGES CXX VERSION ${VERSION})

include(FetchContent)

# control whether this project builds its tests
option(BUILD_PROJECT_TESTS "Build tupal's tests" ON)
# control whether external/fetchcontent deps build their tests (default OFF)
option(BUILD_DEPENDENCY_TESTS "Allow dependencies to build their tests" OFF)

# Prevent dependencies from building their tests by forcing BUILD_TESTING=OFF
# before FetchContent/FetchContent_MakeAvailable runs.
if(NOT BUILD_DEPENDENCY_TESTS)
  set(BUILD_TESTING OFF CACHE BOOL "Disable building tests for dependencies" FORCE)
endif()

#find_package(ICU REQUIRED COMPONENTS uc i18n)

find_package(Boost REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(PostgreSQL)

set(CONAN OFF)
set(BEAUTY_BUILD_EXAMPLES OFF)

FetchContent_Declare(
    beauty
    GIT_REPOSITORY https://github.com/dfleury2/beauty
    GIT_TAG 1.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(beauty)

set(SOCI_SHARED ON)
set(SOCI_STATIC ON)
set(SOCI_TESTS OFF)
set(SOCI_VISIBILITY ON)
set(SOCI_LTO ON)

if (PQSQL_FOUND)
    set(SOCI_POSTGRESQL ON)
else()
    set(SOCI_POSTGRESQL OFF)
endif()

set(SOCI_SQLITE3 ON)
set(SOCI_ORACLE OFF)
set(SOCI_EMPTY OFF)
set(SOCI_DB2 OFF)
set(SOCI_FIREBIRD OFF)
set(SOCI_MYSQL OFF)
set(SOCI_ODBC OFF)

FetchContent_Declare(
    soci
    GIT_REPOSITORY https://github.com/SOCI/soci
    GIT_TAG v4.0.3
    GIT_SHALLOW TRUE
    OVERRIDE_FIND_PACKAGE
)
FetchContent_GetProperties(soci)
if (NOT soci_POPULATED)
    FetchContent_Populate(soci)
    add_subdirectory(${soci_SOURCE_DIR} ${soci_BINARY_DIR})
endif()
set(SOCI_TESTS OFF CACHE BOOL "Disable building tests for soci" FORCE)
FetchContent_MakeAvailable(soci)

add_subdirectory(src)

if(BUILD_PROJECT_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()