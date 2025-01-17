# Compile simple program on "Ü-Sprache", link it together with C++ program.
set( EXTERNAL_SYMBOLS_ACCESS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/external_symbols.u )
CompileUSources( "${EXTERNAL_SYMBOLS_ACCESS_SOURCES}" EXTERNAL_SYMBOLS_ACCESS_OBJECTS )

add_executable( ExternalSymbolsAccessTest${CURRENT_COMPILER_GENERATION} external_symbols.cpp ${EXTERNAL_SYMBOLS_ACCESS_SOURCES} ${EXTERNAL_SYMBOLS_ACCESS_OBJECTS} )
add_dependencies( ExternalSymbolsAccessTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

# Run the test.
add_custom_command( TARGET ExternalSymbolsAccessTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND ExternalSymbolsAccessTest${CURRENT_COMPILER_GENERATION} )
