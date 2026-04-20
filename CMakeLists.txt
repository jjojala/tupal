cmake_minimum_required(VERSION 3.21)

set(VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)

project(tupal LANGUAGES C CXX VERSION ${VERSION})

include(FetchContent)

option(BUILD_PROJECT_TESTS "Build tupal's tests" ON)
option(BUILD_DEPENDENCY_TESTS "Allow dependencies to build their tests" OFF)

if(NOT BUILD_DEPENDENCY_TESTS)
  set(BUILD_TESTING OFF CACHE BOOL "Disable building tests for dependencies" FORCE)
endif()

find_package(PostgreSQL)

#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(
  sqlite3_src
  GIT_REPOSITORY https://github.com/azadkuh/sqlite-amalgamation
  GIT_TAG 3.38.2
  GIT_SHALLOW TRUE
)

FetchContent_GetProperties(sqlite3_src)
if(NOT sqlite3_src_POPULATED)
  FetchContent_Populate(sqlite3_src)
endif()

set(SQLITE3_DIR ${sqlite3_src_SOURCE_DIR})

add_library(sqlite3 STATIC
  "${SQLITE3_DIR}/sqlite3.c"
  "${SQLITE3_DIR}/sqlite3.h"
)
set_target_properties(sqlite3 PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(sqlite3 PUBLIC "${SQLITE3_DIR}")
target_compile_definitions(sqlite3 PRIVATE SQLITE_API=)

add_library(SQLite::SQLite3 ALIAS sqlite3)

set(CONAN OFF)
set(BEAUTY_BUILD_EXAMPLES OFF)

FetchContent_Declare(
    beauty
    GIT_REPOSITORY https://github.com/dfleury2/beauty
    GIT_TAG 1.0.8
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

# Set SQLite3 include and library paths for soci-src dependency
set(SQLite3_INCLUDE_DIR "${SQLITE3_DIR}")
set(SQLite3_LIBRARY sqlite3)

FetchContent_Declare(
    soci
    GIT_REPOSITORY https://github.com/SOCI/soci
    GIT_TAG v4.1.2
    GIT_SHALLOW TRUE
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(soci)

FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG v3.0.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cxxopts)

# Fetch Boost via FetchContent
FetchContent_Declare(
  boost
  GIT_REPOSITORY https://github.com/boostorg/boost.git
  GIT_TAG boost-1.83.0
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(boost)

add_subdirectory(src)

if(BUILD_PROJECT_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()
