if( U_BUILD_TESTS )
	file( GLOB TESTS_SOURCES "*.cpp" "*.hpp" )
	add_library( TestsLib ${TESTS_SOURCES} )
endif()
