set( IMPORT_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/root.u )
CompileUSources( "${IMPORT_TEST_SOURCES}" IMPORT_TEST_OBJECTS )

add_executable( ImportTest entry.cpp ${IMPORT_TEST_SOURCES} ${IMPORT_TEST_OBJECTS} )
add_dependencies( ImportTest Compiler )
