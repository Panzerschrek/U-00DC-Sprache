set( SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.u )
CompileUSources( "${SOURCES}" OBJECTS )

add_executable( CompilerInfoTest${CURRENT_COMPILER_GENERATION} ../dummy.cpp ${SOURCES} ${OBJECTS} )
add_dependencies( CompilerInfoTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

# Run the test
add_custom_command( TARGET CompilerInfoTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND CompilerInfoTest${CURRENT_COMPILER_GENERATION} )
