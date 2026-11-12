set(CORE_SOURCES
    model.cc
    manager.cc)

set(CORE_HEADERS
    model.hh
    manager.hh)

add_library(tupal_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
set_target_properties(tupal_core PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    POSITION_INDEPENDENT_CODE ON)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tupal_core PRIVATE -pedantic-errors)
endif()

target_include_directories(tupal_core 
    PUBLIC
        ${soci_SOURCE_DIR}/include
        ${soci_BINARY_DIR}/include
        ${cxxopts_SOURCE_DIR}/include)

target_link_libraries(tupal_core
    PRIVATE
        soci_core
        Boost::json
        Boost::url
        Boost::date_time)

add_executable(tupald tupald.cc)
target_link_libraries(tupald PRIVATE tupal_core beauty::beauty)
set_target_properties(tupald PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(tupald PRIVATE -pedantic-errors)
endif()
add_dependencies(tupald web_frontend)

if(WIN32)
    set_target_properties(tupald
        PROPERTIES
            LINK_FLAGS "/ignore:4099"
    )
endif()
