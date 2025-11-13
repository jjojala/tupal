
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)
FetchContent_MakeAvailable(doctest)

add_executable(tests 
    test_main.cc
    test_competition_manager.cc
    test_start_group_manager.cc
    test_competition_class_manager.cc
    test_competitor_manager.cc)

target_include_directories(tests 
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${doctest_SOURCE_DIR})

target_link_libraries(tests
    PRIVATE
        tupal_core
        boost_json
        soci_core_static)

add_test(NAME tests COMMAND $<TARGET_FILE:tests>)

set_tests_properties(
        tests
    PROPERTIES 
        LABELS "tupal" 
        TIMEOUT 10
        ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH}")

add_executable(
        rest_tests
    test_main.cc
    rest_test.cc)

target_include_directories(
        rest_tests 
    PRIVATE
        ${doctest_SOURCE_DIR})

target_link_libraries(rest_tests PRIVATE boost_filesystem beauty::beauty)

add_test(NAME rest_tests COMMAND $<TARGET_FILE:rest_tests>)

set_tests_properties(
        rest_tests
    PROPERTIES 
        LABELS "tupal,rest"
        TIMEOUT 10
        ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH};PATH=${CMAKE_BINARY_DIR}/src:$ENV{PATH}")
