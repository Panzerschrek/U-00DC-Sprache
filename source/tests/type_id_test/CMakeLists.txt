set( TYPE_ID_TEST_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/type_id_a.u
	${CMAKE_CURRENT_SOURCE_DIR}/type_id_b.u
	${CMAKE_CURRENT_SOURCE_DIR}/type_id_typeinfo.u
	)
CompileUSources( "${TYPE_ID_TEST_SOURCES}" TYPE_ID_TEST_OBJECTS )

add_executable( TypeIdTest${CURRENT_COMPILER_GENERATION} entry.cpp ${TYPE_ID_TEST_SOURCES} ${TYPE_ID_TEST_OBJECTS} )
add_dependencies( TypeIdTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

# Run type_id test
add_custom_command( TARGET TypeIdTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND TypeIdTest${CURRENT_COMPILER_GENERATION} )
