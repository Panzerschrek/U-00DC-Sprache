set( SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/generated_symbols_test.u )
set( OBJECT_FILE ${CMAKE_CURRENT_BINARY_DIR}/generated_symbols_test.o )
add_custom_command(
	OUTPUT ${OBJECT_FILE}
	DEPENDS ${SOURCE_FILE} Compiler${CURRENT_COMPILER_GENERATION}
	COMMAND Compiler${CURRENT_COMPILER_GENERATION} ${SOURCE_FILE} -o ${OBJECT_FILE} ${SPRACHE_COMPILER_PIC_OPTIONS} --verify-module --halt-mode=configurable_handler )

add_executable( GeneratedSymbolsTest${CURRENT_COMPILER_GENERATION} generated_symbols_test.cpp ${SOURCE_FILE} ${OBJECT_FILE} )
add_dependencies( GeneratedSymbolsTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

# Run generated symbols test
add_custom_command( TARGET GeneratedSymbolsTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND GeneratedSymbolsTest${CURRENT_COMPILER_GENERATION} )
