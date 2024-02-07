set( TEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/test.u )
set( TEST_FILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/test.o )
add_custom_command(
	OUTPUT ${TEST_FILE_OUT}
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION} ${TEST_FILE}
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${TEST_FILE} -o ${TEST_FILE_OUT}
		${SPRACHE_COMPILER_PIC_OPTIONS}
		-O2 # Always use optimization to enable inlining
		--verify-module
	)


add_executable( AsyncCallsInliningTest${CURRENT_COMPILER_GENERATION} ${TEST_FILE} ${TEST_FILE_OUT} )
target_link_libraries( AsyncCallsInliningTest${CURRENT_COMPILER_GENERATION} PRIVATE CPPDummy )
add_dependencies( AsyncCallsInliningTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )
# Run the test
add_custom_command( TARGET AsyncCallsInliningTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND AsyncCallsInliningTest${CURRENT_COMPILER_GENERATION} )
