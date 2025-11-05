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

#FetchContent_Declare(
#	openssl
#	GIT_REPOSITORY https://github.com/openssl/openssl
#	GIT_TAG openssl-3.4.0
#	GIT_SHALLOW TRUE
#	OVERRIDE_FIND_PACKAGE
#)
#FetchContent_MakeAvailable(openssl)

#set(Boost_ENABLE_CMAKE ON)
#set(Boost_USE_STATIC_LIBS ON)
#set(Boost_USE_MULTITHREADED ON)
#set(Boost_INCLUDE_LIBS boost asio regex algorit json thread filesystem system program_options)

#FetchContent_Declare(
#	Boost
#	GIT_REPOSITORY https://github.com/boostorg/boost
#	GIT_TAG boost-1.86.0
#	GIT_SHALLOW TRUE
#	CONFIGURE_COMMAND ""
#	OVERRIDE_FIND_PACKAGE
#)
#FetchContent_MakeAvailable(Boost boost)
find_package(Boost)

set(CONAN OFF)
set(BEAUTY_BUILD_EXAMPLES OFF)
#set(BUILD_TESTING OFF)
#include(CTest)                        # defines the BUILD_TESTING option
#option(BUILD_TESTING "Enable building tests" ON)

#if(BUILD_TESTING)
#	enable_testing()
#	add_subdirectory(test)
#endif()

# FetchContent calls for dependencies (beauty, SQLite, soci, ...)
FetchContent_Declare(
    beauty
    GIT_REPOSITORY https://github.com/dfleury2/beauty
    GIT_TAG 1.0.6
    GIT_SHALLOW TRUE
#	OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(beauty)

FetchContent_Declare(
    SQLite
    GIT_REPOSITORY https://github.com/azadkuh/sqlite-amalgamation
    GIT_TAG 3.38.2
    GIT_SHALLOW TRUE
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(SQLite)

set(SOCI_SHARED OFF)
set(SOCI_STATIC ON)
set(SOCI_TESTS OFF)
set(SOCI_VISIBILITY ON)
set(SOCI_LTO ON)

set(SOCI_POSTGRESQL OFF)
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

# Now add our project's tests if requested.
# We deliberately do this after FetchContent so dependency configuration
# saw BUILD_TESTING=OFF and didn't add their test targets.
if(BUILD_PROJECT_TESTS)
  include(CTest)       # provides CTest support
  enable_testing()
  add_subdirectory(test)
endif()

