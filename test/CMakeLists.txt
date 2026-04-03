
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)
FetchContent_MakeAvailable(doctest)

add_executable(manager_tests test_main.cc test_competition_manager.cc test_start_group_manager.cc
    test_competition_class_manager.cc test_competitor_manager.cc)
target_include_directories(manager_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${doctest_SOURCE_DIR})
target_link_libraries(manager_tests PRIVATE tupal_core boost_json soci_core)
add_test(NAME manager_tests COMMAND $<TARGET_FILE:manager_tests>)
set_tests_properties(manager_tests PROPERTIES LABELS "tupal,manager" TIMEOUT 10
        ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH}")

add_executable(rest_tests test_main.cc rest_test.cc)
target_include_directories(rest_tests PRIVATE ${doctest_SOURCE_DIR})
target_link_libraries(rest_tests PRIVATE boost_filesystem beauty::beauty)
add_test(NAME rest_tests COMMAND $<TARGET_FILE:rest_tests>)
set_tests_properties(rest_tests PROPERTIES LABELS "tupal,rest" TIMEOUT 10
        ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/lib:$ENV{LD_LIBRARY_PATH};PATH=${CMAKE_BINARY_DIR}/src:$ENV{PATH}")

add_executable(model_tests test_main.cc model_test.cc)
target_include_directories(model_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${doctest_SOURCE_DIR})
target_link_libraries(model_tests PRIVATE tupal_core boost_json)
add_test(NAME model_tests COMMAND $<TARGET_FILE:model_tests>)
set_tests_properties(model_tests PROPERTIES LABELS "tupal,model" TIMEOUT 10)
