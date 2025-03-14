file( GLOB_RECURSE SOURCES "*.cpp" )
file( GLOB_RECURSE HEADERS "*.hpp" )

if( CMAKE_SYSTEM_NAME STREQUAL "Emscripten" )

	set( CMAKE_EXECUTABLE_SUFFIX ".js" )

	set(
		CMAKE_EXE_LINKER_FLAGS
		"${CMAKE_EXE_LINKER_FLAGS}--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/emscripten/shell.js --embed-file ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/imports/@ustlib/imports --embed-file ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/src/@ustlib/src" )

	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/emscripten/Interpreter_launcher.js DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

	# Test shell is used for debugging.
	# Launch a local http server in the build directory via command "python3 -m http.server" and open http://127.0.0.1:8000/Interpreter_test_shell.html in a browser.
	file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/emscripten/Interpreter_test_shell.html DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
endif()

add_executable( Interpreter ${SOURCES} ${HEADERS} )
# Link directly against CodeBuilderLib of compiler0. There is no reason to use compiler1 for interpreter.
target_link_libraries( Interpreter CodeBuilderLib CompilersSupportLib ${LLVM_LIBS_FOR_INTERPRETER} )

if( NOT CMAKE_CROSSCOMPILING )
	install( TARGETS Interpreter )
endif()

if( NOT CMAKE_CROSSCOMPILING )

# Run various tests.

set( TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/simple_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/simple_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/stdout_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/stdout_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/custom_entry_point_test.u --entry custom_entry_point )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/custom_entry_point_test.u --entry custom_entry_point --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/includes_test.u --include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/imports )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/includes_test.u --include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/imports --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/heap_allocation_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/heap_allocation_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/prelude_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/prelude_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/intrinsics_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/intrinsics_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${TESTS_DIR}/prefixed_imports.u --include-dir ${TESTS_DIR}/imports_dir::fancy/path )

# Run some ustlib tests. It is important, since there tests contain some intrinsics, that need to be tested.
set( USTLIB_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/tests )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${USTLIB_TESTS_DIR}/allocations_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${USTLIB_TESTS_DIR}/allocations_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${USTLIB_TESTS_DIR}/atomic_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${USTLIB_TESTS_DIR}/atomic_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${USTLIB_TESTS_DIR}/coro_test.u ) # Works only without JIT.
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${USTLIB_TESTS_DIR}/checked_math_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${USTLIB_TESTS_DIR}/checked_math_test.u --use-jit )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${USTLIB_TESTS_DIR}/volatile_test.u )
add_custom_command( TARGET Interpreter POST_BUILD COMMAND Interpreter ${USTLIB_TESTS_DIR}/volatile_test.u --use-jit )

endif() # NOT CMAKE CROSSCOMPILING
