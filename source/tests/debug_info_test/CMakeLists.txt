set( DEBUG_INFO_TEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/debug_info_test.u )
set( DEBUG_INFO_TEST_FILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/debug_info_test.o )
add_custom_command(
	OUTPUT ${DEBUG_INFO_TEST_FILE_OUT}
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION} ${DEBUG_INFO_TEST_FILE}
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${DEBUG_INFO_TEST_FILE} -o ${DEBUG_INFO_TEST_FILE_OUT}
		${SPRACHE_COMPILER_PIC_OPTIONS}
		-O0 # Disable optimization
		-g # Generate debug info
		--enable-unused-names # Enable unused variables in order to inspect debug info for them.
		--verify-module
	)

add_executable( DebugInfoTest${CURRENT_COMPILER_GENERATION} ../dummy.cpp ${DEBUG_INFO_TEST_FILE} ${DEBUG_INFO_TEST_FILE_OUT} )
add_dependencies( DebugInfoTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )
