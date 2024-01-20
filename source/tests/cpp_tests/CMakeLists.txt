if( U_BUILD_TESTS )
	message( STATUS "Build tests" )

	file( GLOB CPP_TESTS_SOURCES "*.cpp" "*.hpp" )
	add_library( CppTestsLib ${CPP_TESTS_SOURCES} )
	target_link_libraries( CppTestsLib PUBLIC TestsLib ${LLVM_LIBS_FOR_CODE_BUILDER_LIB} )

else()
	message( STATUS "Skip tests" )
endif()
