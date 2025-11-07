
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)
FetchContent_MakeAvailable(doctest)

add_executable(tests test_competition_manager.cc)

target_include_directories(
        tests 
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${doctest_SOURCE_DIR})

target_link_libraries(
        tests
    PRIVATE
        tupal_core
        boost_json
        soci_core_static)

add_test(NAME tupal_manager_test COMMAND $<TARGET_FILE:tests>)

set_tests_properties(
        tupal_manager_test
    PROPERTIES 
        LABELS "tupal" 
        TIMEOUT 10
        ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH}")
