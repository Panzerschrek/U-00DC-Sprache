file( GLOB_RECURSE SOURCES "*.cpp" )
file( GLOB_RECURSE HEADERS "*.hpp" )

add_executable( Interpreter ${SOURCES} ${HEADERS} )
# Link directly against CodeBuilderLib of compiler0. There is no reason to use compiler1 for interpreter.
target_link_libraries( Interpreter CodeBuilderLib CompilersSupportLib ${LLVM_LIBS_FOR_INTERPRETER} )

# Run various tests.

set( TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/simple_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/simple_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/stdout_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/stdout_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/custom_entry_point_test.u --entry custom_entry_point )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/custom_entry_point_test.u --entry custom_entry_point --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/includes_test.u --include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/ )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/includes_test.u --include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/ --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/heap_allocation_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/heap_allocation_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/prelude_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/prelude_test.u --use-jit )
