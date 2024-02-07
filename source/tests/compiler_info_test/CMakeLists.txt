set( SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.u )
CompileUSources( "${SOURCES}" OBJECTS )

add_executable( CompilerInfoTest${CURRENT_COMPILER_GENERATION}  ${SOURCES} ${OBJECTS} )
target_link_libraries( CompilerInfoTest${CURRENT_COMPILER_GENERATION} PRIVATE CPPDummy )
add_dependencies( CompilerInfoTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )
