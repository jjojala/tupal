set(SOURCES
	tupald.cc
	manager.cc)
set(HEADERS
	manager.hh)
	
add_executable(tupald ${SOURCES} ${HEADERS})
target_include_directories(tupald
	PRIVATE
		${beauty_SOURCE_DIR}/include
		${soci_SOURCE_DIR}/include
		${soci_BINARY_DIR}/include
		${SQLite_SOURCE_DIR}/include)

target_link_libraries(tupald
	PRIVATE
		soci_sqlite3_static soci_core_static boost_url
		beauty::beauty)

if(WIN32)
	set_target_properties(tupald
		PROPERTIES
			LINK_FLAGS "/ignore:4099"
		)
endif()
