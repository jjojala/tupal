cmake_minimum_required(VERSION 3.14)
project(tests)

include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)
FetchContent_MakeAvailable(doctest)

enable_testing()

add_executable(tests test_competition_manager.cc)

# Prefer doctest targets if the FetchContent-created package provides them.
if (TARGET doctest::doctest_main)
    target_link_libraries(tests PRIVATE doctest::doctest_main)
elseif (TARGET doctest::doctest)
    target_link_libraries(tests PRIVATE doctest::doctest)
else()
    # Fallback: add the single_include directory from the fetched doctest sources.
    # doctest_SOURCE_DIR is provided by FetchContent when the repo is populated.
    target_include_directories(tests PRIVATE ${doctest_SOURCE_DIR}/single_include)
endif()

add_test(NAME tupal_manager_test COMMAND $<TARGET_FILE:tests>)
set_tests_properties(tupal_manager_test PROPERTIES LABELS "tupal" TIMEOUT 10)