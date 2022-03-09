set( SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.u )
CompileUSources( "${SOURCES}" OBJECTS )

add_executable( WindowsSystemCallTest${CURRENT_COMPILER_GENERATION} ../dummy.cpp ${SOURCES} ${OBJECTS} )
add_dependencies( WindowsSystemCallTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

# Run inline linkage test
add_custom_command( TARGET WindowsSystemCallTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND WindowsSystemCallTest${CURRENT_COMPILER_GENERATION} )
