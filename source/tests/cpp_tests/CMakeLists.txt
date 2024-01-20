if( U_BUILD_TESTS )
	message( STATUS "Build tests" )

	file( GLOB CPP_TESTS_SOURCES "*.cpp" "*.hpp" )
	add_library( CppTestsLib ${CPP_TESTS_SOURCES} )
	target_link_libraries( CppTestsLib PUBLIC TestsLib ${LLVM_LIBS_FOR_CODE_BUILDER_LIB} )

	# Hack! Force use static variables with initializers with tests registration
	set( CPP_TESTS_LIB_WRAPPED CppTestsLib )
	if( NOT MSVC )
		set( CPP_TESTS_LIB_WRAPPED -Wl,--whole-archive ${CPP_TESTS_LIB_WRAPPED} -Wl,--no-whole-archive )
	endif()

else()
	message( STATUS "Skip tests" )
endif()
