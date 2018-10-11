cmake_minimum_required(VERSION 2.8)

project(Sprache)

option( BUILD_COMPILER "Enable compilation of compiler" YES )
option( BUILD_TESTS "Enable compilation of base tests" YES )
option( BUILD_LINKAGE_TESTS "Enable compilation of linkage tests" YES )
option( BUILD_PY_TESTS "Enable compilation of py_tests" YES )

if( BUILD_COMPILER )
	message( STATUS "Build compiler" )
else()
	message( STATUS "Skip compiler" )
endif( BUILD_COMPILER )

if( BUILD_TESTS )
	message( STATUS "Build tests" )
else()
	message( STATUS "Skip tests" )
endif( BUILD_TESTS )

if( BUILD_LINKAGE_TESTS )
	message( STATUS "Build linkage tests" )
else()
	message( STATUS "Skip linkage tests" )
endif( BUILD_LINKAGE_TESTS )

if( BUILD_PY_TESTS )
	message( STATUS "Build py_tests" )
else()
	message( STATUS "Skip py_tests" )
endif( BUILD_PY_TESTS )


set( CMAKE_CXX_STANDARD 11 )

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	add_definitions( -DU_DEBUG )
else()
endif()

if( NOT WIN32 )
	add_compile_options( -fPIC )
endif()
add_compile_options( -Wall -Wextra -Wconversion )

#
# External dependencies
#

# Boost
if( NOT DEFINED BOOST_ROOT )
	message( FATAL_ERROR "BOOST_ROOT not defined. You can define path to boost library, using variable BOOST_ROOT" )
endif()
set( Boost_USE_STATIC_LIBS yes )
find_package( Boost COMPONENTS system filesystem program_options REQUIRED )
include_directories( ${Boost_INCLUDE_DIRS} )

# LLVM
if( NOT DEFINED LLVM_INSTALL_DIR )
	message( FATAL_ERROR "LLVM_INSTALL_DIR not defined. You can define path to llvm library, using variable LLVM_INSTALL_DIR" )
endif()
set( LLVM_DIR ${LLVM_INSTALL_DIR}/share/llvm/cmake/ )

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER_LIB Core )
llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER BitWriter BitReader Linker native ) # "native" is alias for native code generator.
llvm_map_components_to_libnames( LLVM_LIBS_FOR_TESTS ExecutionEngine Interpreter )

get_filename_component( LLVM_UTILS_DIR ${LLVM_INSTALL_DIR}/bin ABSOLUTE )

#
# CompilerLib
#

set( COMPILER_LIB_SOURCES
	code_builder_lib/code_builder_errors.cpp
	code_builder_lib/code_builder.cpp
	code_builder_lib/code_builder_expressions.cpp
	code_builder_lib/code_builder_initializers.cpp
	code_builder_lib/code_builder_method_gen.cpp
	code_builder_lib/code_builder_global_things_build.cpp
	code_builder_lib/code_builder_names_scope_prepare.cpp
	code_builder_lib/code_builder_overloading_resolution.cpp
	code_builder_lib/code_builder_reference_checking.cpp
	code_builder_lib/code_builder_templates.cpp
	code_builder_lib/code_builder_typeinfo.cpp
	code_builder_lib/code_builder_types.cpp
	code_builder_lib/code_builder_virtual.cpp
	code_builder_lib/constexpr_function_evaluator.cpp
	code_builder_lib/mangling.cpp
	lex_synt_lib/keywords.cpp
	lex_synt_lib/lang_types.cpp
	lex_synt_lib/lexical_analyzer.cpp
	lex_synt_lib/operators.cpp
	lex_synt_lib/program_string.cpp
	lex_synt_lib/source_graph_loader.cpp
	lex_synt_lib/syntax_analyzer.cpp
	lex_synt_lib/syntax_elements.cpp )

set( COMPILER_LIB_HEADERS
	code_builder_lib/code_builder_errors.hpp
	code_builder_lib/code_builder.hpp
	code_builder_lib/code_builder_types.hpp
	code_builder_lib/constexpr_function_evaluator.hpp
	code_builder_lib/i_code_builder.hpp
	code_builder_lib/mangling.hpp
	code_builder_lib/pop_llvm_warnings.hpp
	code_builder_lib/push_disable_llvm_warnings.hpp
	lex_synt_lib/assert.hpp
	lex_synt_lib/keywords.hpp
	lex_synt_lib/lexical_analyzer.hpp
	lex_synt_lib/operators.hpp
	lex_synt_lib/program_string.hpp
	lex_synt_lib/source_graph_loader.hpp
	lex_synt_lib/syntax_elements.hpp
	lex_synt_lib/syntax_analyzer.hpp )

add_library( CompilerLib ${COMPILER_LIB_SOURCES} ${COMPILER_LIB_HEADERS} )
target_link_libraries( CompilerLib ${LLVM_LIBS_FOR_COMPILER_LIB} )
if( NOT WIN32 )
	target_link_libraries( CompilerLib pthread dl )
endif()

#
# Compiler
#

if( BUILD_COMPILER )
	set( COMPILER_SOURCES
		compiler/main.cpp )

	set( COMPILER_HEADERS
		compiler/push_disable_boost_warnings.hpp
		compiler/pop_boost_warnings.hpp )

	function( CompileLLFile ll_file )
		get_filename_component( source_name ${ll_file} NAME_WE )
		set( bc_name ${source_name}.bc )
		add_custom_command(
			OUTPUT ${bc_name}
			DEPENDS ${CMAKE_SOURCE_DIR}/ustlib/src/${ll_file}
			COMMAND ${LLVM_UTILS_DIR}/llvm-as ${CMAKE_SOURCE_DIR}/ustlib/src/${ll_file} -o ${bc_name} )
		set( BC_FILE ${bc_name} PARENT_SCOPE )
	endfunction()

	CompileLLFile( asm_funcs.ll )
	list( APPEND bc_files ${BC_FILE} )
	CompileLLFile( asm_funcs_32.ll )
	list( APPEND bc_files ${BC_FILE} )
	CompileLLFile( asm_funcs_64.ll )
	list( APPEND bc_files ${BC_FILE} )

	add_executable( Compiler ${COMPILER_SOURCES} ${COMPILER_HEADERS} ${bc_files} )
	target_link_libraries( Compiler CompilerLib ${LLVM_LIBS_FOR_COMPILER} )
	target_link_libraries( Compiler ${Boost_LIBRARIES} )
endif( BUILD_COMPILER )

#
# Tests
#
if( BUILD_TESTS )
	set( TESTS_SOURCES
		tests/cpp_tests/auto_variables_test.cpp
		tests/cpp_tests/auto_variables_errors_test.cpp
		tests/cpp_tests/class_templates_errors_test.cpp
		tests/cpp_tests/class_templates_test.cpp
		tests/cpp_tests/classes_declaration_errors_test.cpp
		tests/cpp_tests/classes_declaration_test.cpp
		tests/cpp_tests/code_builder_errors_test.cpp
		tests/cpp_tests/code_builder_test.cpp
		tests/cpp_tests/constexpr_errors_test.cpp
		tests/cpp_tests/constexpr_test.cpp
		tests/cpp_tests/constructors_errors_test.cpp
		tests/cpp_tests/constructors_test.cpp
		tests/cpp_tests/destructors_errors_test.cpp
		tests/cpp_tests/destructors_test.cpp
		tests/cpp_tests/enums_errors_test.cpp
		tests/cpp_tests/enums_test.cpp
		tests/cpp_tests/global_variables_errors_test.cpp
		tests/cpp_tests/global_variables_test.cpp
		tests/cpp_tests/halt_test.cpp
		tests/cpp_tests/imports_errors_test.cpp
		tests/cpp_tests/imports_test.cpp
		tests/cpp_tests/initializers_errors_test.cpp
		tests/cpp_tests/initializers_test.cpp
		tests/cpp_tests/mangling_test.cpp
		tests/cpp_tests/methods_errors_test.cpp
		tests/cpp_tests/methods_test.cpp
		tests/cpp_tests/mutability_errors_test.cpp
		tests/cpp_tests/namespaces_errors_test.cpp
		tests/cpp_tests/namespaces_test.cpp
		tests/cpp_tests/operators_overloading_errors_test.cpp
		tests/cpp_tests/operators_overloading_test.cpp
		tests/cpp_tests/operators_priority_test.cpp
		tests/cpp_tests/operators_test.cpp
		tests/cpp_tests/reference_check_test.cpp
		tests/cpp_tests/reference_check_for_structs_test.cpp
		tests/cpp_tests/references_inside_structs_errors_test.cpp
		tests/cpp_tests/references_inside_structs_test.cpp
		tests/cpp_tests/references_tags_test.cpp
		tests/cpp_tests/temp_variables_moving_test.cpp
		tests/cpp_tests/tests.cpp
		tests/cpp_tests/type_conversions_test.cpp
		tests/cpp_tests/typedefs_errors_test.cpp
		tests/cpp_tests/typedefs_test.cpp
		tests/cpp_tests/variables_state_merge_test.cpp
		tests/tests_common.hpp )

	set( TESTS_HEADERS
		tests/cpp_tests/tests.hpp )

	add_executable( Tests ${TESTS_SOURCES} ${TESTS_HEADERS} )
	target_link_libraries( Tests CompilerLib ${LLVM_LIBS_FOR_TESTS} )
endif( BUILD_TESTS )

#
# PyTests
#
if( BUILD_PY_TESTS )
	find_package( PythonLibs REQUIRED )
	find_package( PythonInterp REQUIRED )

	set( PY_TESTS_SOURCES
		tests/py_tests/py_tests.cpp
		tests/tests_common.hpp )

	add_library( PyTestsLib SHARED ${PY_TESTS_SOURCES} )
	target_link_libraries( PyTestsLib CompilerLib )
	target_link_libraries( PyTestsLib ${LLVM_LIBS_FOR_TESTS} )
	target_link_libraries( PyTestsLib ${PYTHON_LIBRARIES} )
	target_include_directories( PyTestsLib PRIVATE ${PYTHON_INCLUDE_DIRS} )

	# Set proper name for result dynamic library.
	if( WIN32 )
		set_target_properties( PyTestsLib
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.pyd PREFIX "" SUFFIX "" )
	else()
		set_target_properties( PyTestsLib
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.so PREFIX "" SUFFIX "" )
	endif()

	# Run result test.
	add_custom_target(
		PyTestsRun ALL
		COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_EXECUTABLE} tests/py_tests/py_tests.py
		DEPENDS PyTestsLib
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} )

endif( BUILD_PY_TESTS )

# Build rule for "Ü" files
# Returns result in U_SOURCE_OBJECT
function( CompileUSource u_source_file )
	get_filename_component( source_name ${u_source_file} NAME_WE )
	set( object_name ${source_name}.o )
	add_custom_command(
		OUTPUT ${object_name}
		DEPENDS ${u_source_file} Compiler
		COMMAND Compiler --produce-object-file ${CMAKE_SOURCE_DIR}/${u_source_file} -o ${object_name} )
	set( U_SOURCE_OBJECT ${object_name} PARENT_SCOPE )
endfunction()

# Returns list of objects in U_SOURCE_OBJECTS
function( CompileUSources u_source_files )
	foreach( source ${u_source_files} )
		CompileUSource( ${source} )
		list( APPEND out_objects ${U_SOURCE_OBJECT} )
	endforeach()
	set( U_SOURCE_OBJECTS ${out_objects} PARENT_SCOPE )
endfunction()

#
# CPP-Linkage test
#
if( BUILD_LINKAGE_TESTS )
	# Compile simple program on "Ü-Sprache", link it together with C++ program.

	set( CPP_LINKAGE_SOURCES tests/cpp_linkage_test/cpp_linkage.u )
	CompileUSources( "${CPP_LINKAGE_SOURCES}" )
	set( CPP_LINKAGE_OBJECTS ${U_SOURCE_OBJECTS} )

	add_executable( CPPLinkageTest tests/cpp_linkage_test/cpp_linkage.cpp ${CPP_LINKAGE_OBJECTS} )
	add_dependencies( CPPLinkageTest Compiler )
endif( BUILD_LINKAGE_TESTS )

#
# Inline linkage test
#
if( BUILD_LINKAGE_TESTS )
	# Compile multimodule program on "Ü-Sprache". Each module includes one common file.

	set( INLINE_LINKAGE_SOURCES
		tests/inline_linkage_test/a.u
		tests/inline_linkage_test/b.u
		tests/inline_linkage_test/c.u )
	CompileUSources( "${INLINE_LINKAGE_SOURCES}" )
	set( INLINE_LINKAGE_OBJECTS ${U_SOURCE_OBJECTS} )

	add_executable( InlineLinkageTest tests/inline_linkage_test/entry.cpp ${INLINE_LINKAGE_OBJECTS} )
	add_dependencies( InlineLinkageTest Compiler )
endif( BUILD_LINKAGE_TESTS )

#
# Imports test
#
if( BUILD_LINKAGE_TESTS )

	set( IMPORT_TEST_SOURCES tests/import_test/root.u )
	CompileUSources( "${IMPORT_TEST_SOURCES}" )
	set( IMPORT_TEST_OBJECTS ${U_SOURCE_OBJECTS} )

	add_executable( ImportTest tests/import_test/entry.c ${IMPORT_TEST_OBJECTS} )
	add_dependencies( ImportTest Compiler )
endif( BUILD_LINKAGE_TESTS )

#
# Include dirs test
#
if( BUILD_LINKAGE_TESTS )

	add_custom_command(
		OUTPUT include_dirs_test.o
		DEPENDS ${u_source_file} Compiler
		COMMAND Compiler --produce-object-file ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/root.u -o include_dirs_test.o --include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/a --include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/b --include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/c )

	add_executable( IncludeDirsTest tests/include_dirs_test/entry.c include_dirs_test.o )
	add_dependencies( IncludeDirsTest Compiler )
endif( BUILD_LINKAGE_TESTS )
