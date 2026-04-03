set(CORE_SOURCES
    model.cc
    manager.cc)

set(CORE_HEADERS
    model.hh
    manager.hh)

add_library(tupal_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
set_target_properties(tupal_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(tupal_core 
    PUBLIC
        ${soci_SOURCE_DIR}/include
        ${soci_BINARY_DIR}/include
        ${cxxopts_SOURCE_DIR}/include)

target_link_libraries(tupal_core
    PRIVATE
        soci_core
        boost_url)

add_executable(tupald tupald.cc)
target_link_libraries(tupald PRIVATE tupal_core beauty::beauty)

if(WIN32)
    set_target_properties(tupald
        PROPERTIES
            LINK_FLAGS "/ignore:4099"
    )
endif()
