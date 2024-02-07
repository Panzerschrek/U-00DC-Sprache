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
		--allow-unused-names # Allow unused variables in order to inspect debug info for them.
		--verify-module
	)

add_executable( DebugInfoTest${CURRENT_COMPILER_GENERATION} ${DEBUG_INFO_TEST_FILE} ${DEBUG_INFO_TEST_FILE_OUT} )
target_link_libraries( DebugInfoTest${CURRENT_COMPILER_GENERATION} PRIVATE CPPDummy )
add_dependencies( DebugInfoTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )
