set( SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/nonnull_instructions_metadata_test.u )
CompileUSources( "${SOURCES}" OBJECTS )

set( TEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/nonnull_instructions_metadata_test.u )
set( TEST_FILE_LL ${CMAKE_CURRENT_BINARY_DIR}/nonnull_instructions_metadata_test.ll )
set( TEST_FILE_OBJ ${CMAKE_CURRENT_BINARY_DIR}/nonnull_instructions_metadata_test.o )

# Compile into LL first in order to inspect this file.
add_custom_command(
	OUTPUT ${TEST_FILE_LL}
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION} ${TEST_FILE}
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${TEST_FILE} -o ${TEST_FILE_LL}
		${SPRACHE_COMPILER_PIC_OPTIONS}
		-O2
		--verify-module
		--filetype=ll
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../../ustlib/imports
	)

# Compile LL into object file.
add_custom_command(
	OUTPUT ${TEST_FILE_OBJ}
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION} ${TEST_FILE_LL}
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${TEST_FILE_LL} -o ${TEST_FILE_OBJ}
		${SPRACHE_COMPILER_PIC_OPTIONS}
		-O2
		--verify-module
		--filetype=obj --input-filetype=ll
	)

add_library( NonnullInstructionsMetadataTest${CURRENT_COMPILER_GENERATION} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/stubs.cpp ${TEST_FILE} ${TEST_FILE_OBJ} )
add_dependencies( NonnullInstructionsMetadataTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )
