set( SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/x.u
	${CMAKE_CURRENT_SOURCE_DIR}/y.u
	${CMAKE_CURRENT_SOURCE_DIR}/main.u )
CompileUSources( "${SOURCES}" OBJECTS )

add_executable( MacroUniqueIdentifiersTest${CURRENT_COMPILER_GENERATION} ${SOURCES} ${OBJECTS} )
target_link_libraries( MacroUniqueIdentifiersTest${CURRENT_COMPILER_GENERATION} PRIVATE CPPDummy )
add_dependencies( MacroUniqueIdentifiersTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

# Run the test
add_custom_command( TARGET MacroUniqueIdentifiersTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND MacroUniqueIdentifiersTest${CURRENT_COMPILER_GENERATION} )
