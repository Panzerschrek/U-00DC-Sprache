set( TYPE_ID_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/type_id_a.u ${CMAKE_CURRENT_SOURCE_DIR}/type_id_b.u )
CompileUSources( "${TYPE_ID_TEST_SOURCES}" TYPE_ID_TEST_OBJECTS )

add_executable( TypeIdTest entry.cpp ${TYPE_ID_TEST_SOURCES} ${TYPE_ID_TEST_OBJECTS} )
add_dependencies( TypeIdTest Compiler )

# Run type_id test
add_custom_command( TARGET TypeIdTest POST_BUILD COMMAND TypeIdTest )
