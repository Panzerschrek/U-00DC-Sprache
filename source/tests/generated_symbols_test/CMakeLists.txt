set( GENERATED_SYMBOLS_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/generated_symbols_test.u )
CompileUSources( "${GENERATED_SYMBOLS_TEST_SOURCES}" GENERATED_SYMBOLS_TEST_OBJECTS )

add_executable( GeneratedSymbolsTest${CURRENT_COMPILER_GENERATION} generated_symbols_test.cpp ${GENERATED_SYMBOLS_TEST_SOURCES} ${GENERATED_SYMBOLS_TEST_OBJECTS} )
add_dependencies( GeneratedSymbolsTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

# Run generated symbols test
add_custom_command( TARGET GeneratedSymbolsTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND GeneratedSymbolsTest${CURRENT_COMPILER_GENERATION} )
