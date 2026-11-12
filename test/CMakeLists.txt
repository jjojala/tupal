
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.12
)
FetchContent_MakeAvailable(doctest)

add_executable(manager_tests test_main.cc test_competition_manager.cc test_start_group_manager.cc
    test_competition_class_manager.cc test_competitor_manager.cc
    test_finish_time_entry_manager.cc test_finish_bib_entry_manager.cc)
target_include_directories(manager_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${doctest_SOURCE_DIR})
target_link_libraries(manager_tests PRIVATE tupal_core Boost::json Boost::url Boost::date_time soci_core)
add_test(NAME manager_tests COMMAND $<TARGET_FILE:manager_tests>)
set_tests_properties(manager_tests PROPERTIES LABELS "tupal,manager" TIMEOUT 10)

add_executable(rest_tests test_main.cc rest_test.cc)
target_include_directories(rest_tests PRIVATE ${doctest_SOURCE_DIR})
target_link_libraries(rest_tests PRIVATE Boost::filesystem beauty::beauty)
if(NOT WIN32)
    add_test(NAME rest_tests COMMAND $<TARGET_FILE:rest_tests>)
    set_tests_properties(rest_tests PROPERTIES LABELS "tupal,rest" TIMEOUT 10
        ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/src:$ENV{PATH}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/src")
endif()

add_executable(model_tests test_main.cc model_test.cc)
target_include_directories(model_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${doctest_SOURCE_DIR})
target_link_libraries(model_tests PRIVATE tupal_core Boost::json Boost::url Boost::date_time)
add_test(NAME model_tests COMMAND $<TARGET_FILE:model_tests>)
set_tests_properties(model_tests PROPERTIES LABELS "tupal,model" TIMEOUT 10)

if(NOT WIN32)
    find_program(NPM_EXECUTABLE NAMES npm REQUIRED)
    add_test(NAME web_system_smoke
        COMMAND "${NPM_EXECUTABLE}" run test:system)
    set_tests_properties(web_system_smoke PROPERTIES
        LABELS "tupal,web,system"
        TIMEOUT 120
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/web"
        ENVIRONMENT
            "TUPAL_SERVER_EXECUTABLE=$<TARGET_FILE:tupald>;TUPAL_WEB_ROOT=${TUPAL_WEB_OUTPUT_DIR}")
endif()
