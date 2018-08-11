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
	set( BOOST_ROOT ../../boost_1_60_0 )
else()
	message( STATUS "you can define path to boost library, using variable BOOST_ROOT" )
endif()
set( Boost_USE_STATIC_LIBS yes )
find_package( Boost COMPONENTS system filesystem REQUIRED )
include_directories( ${Boost_INCLUDE_DIRS} )

# LLVM
if( NOT DEFINED LLVM_INSTALL_DIR )
	set( LLVM_INSTALL_DIR ../../llvm-3.7.1.src/build/ )
else()
	message( STATUS "you can define path to llvm library, using variable LLVM_INSTALL_DIR" )
endif()
set( LLVM_DIR ${LLVM_INSTALL_DIR}/share/llvm/cmake/ )

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER_LIB Core )
llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER BitWriter AsmParser Linker native ) # "native" is alias for native code generator.
llvm_map_components_to_libnames( LLVM_LIBS_FOR_TESTS ExecutionEngine Interpreter )

get_filename_component( LLVM_UTILS_DIR ${LLVM_INSTALL_DIR}/bin ABSOLUTE )

#
# CompilerLib
#

set( COMPILER_LIB_SOURCES
	src/code_builder_errors.cpp
	src/code_builder.cpp
	src/code_builder_expressions.cpp
	src/code_builder_initializers.cpp
	src/code_builder_method_gen.cpp
	src/code_builder_global_things_build.cpp
	src/code_builder_names_scope_prepare.cpp
	src/code_builder_overloading_resolution.cpp
	src/code_builder_reference_checking.cpp
	src/code_builder_resolve.cpp
	src/code_builder_templates.cpp
	src/code_builder_typeinfo.cpp
	src/code_builder_types.cpp
	src/code_builder_virtual.cpp
	src/constexpr_function_evaluator.cpp
	src/keywords.cpp
	src/lang_types.cpp
	src/lexical_analyzer.cpp
	src/mangling.cpp
	src/operators.cpp
	src/program_string.cpp
	src/source_graph_loader.cpp
	src/syntax_analyzer.cpp
	src/syntax_elements.cpp )

set( COMPILER_LIB_HEADERS
	src/assert.hpp
	src/code_builder_errors.hpp
	src/code_builder.hpp
	src/code_builder_types.hpp
	src/constexpr_function_evaluator.hpp
	src/i_code_builder.hpp
	src/keywords.hpp
	src/lexical_analyzer.hpp
	src/mangling.hpp
	src/operators.hpp
	src/pop_llvm_warnings.hpp
	src/program_string.hpp
	src/push_disable_llvm_warnings.hpp
	src/source_graph_loader.hpp
	src/syntax_elements.hpp
	src/syntax_analyzer.hpp )

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
		src/main.cpp )

	set( COMPILER_HEADERS
		src/stdlib_asm.hpp )

	add_executable( Compiler ${COMPILER_SOURCES} ${COMPILER_HEADERS} )
	target_link_libraries( Compiler CompilerLib ${LLVM_LIBS_FOR_COMPILER} )
	target_link_libraries( Compiler ${Boost_LIBRARIES} )
endif( BUILD_COMPILER )

#
# Tests
#
if( BUILD_TESTS )
	set( TESTS_SOURCES
		tests/auto_variables_test.cpp
		tests/auto_variables_errors_test.cpp
		tests/class_templates_errors_test.cpp
		tests/class_templates_test.cpp
		tests/classes_declaration_errors_test.cpp
		tests/classes_declaration_test.cpp
		tests/code_builder_errors_test.cpp
		tests/code_builder_test.cpp
		tests/constexpr_errors_test.cpp
		tests/constexpr_test.cpp
		tests/constructors_errors_test.cpp
		tests/constructors_test.cpp
		tests/destructors_errors_test.cpp
		tests/destructors_test.cpp
		tests/enums_errors_test.cpp
		tests/enums_test.cpp
		tests/global_variables_errors_test.cpp
		tests/global_variables_test.cpp
		tests/halt_test.cpp
		tests/imports_errors_test.cpp
		tests/imports_test.cpp
		tests/initializers_errors_test.cpp
		tests/initializers_test.cpp
		tests/mangling_test.cpp
		tests/methods_errors_test.cpp
		tests/methods_test.cpp
		tests/mutability_errors_test.cpp
		tests/namespaces_errors_test.cpp
		tests/namespaces_test.cpp
		tests/operators_overloading_errors_test.cpp
		tests/operators_overloading_test.cpp
		tests/operators_priority_test.cpp
		tests/operators_test.cpp
		tests/reference_check_test.cpp
		tests/reference_check_for_structs_test.cpp
		tests/references_inside_structs_errors_test.cpp
		tests/references_inside_structs_test.cpp
		tests/references_tags_test.cpp
		tests/temp_variables_moving_test.cpp
		tests/tests.cpp
		tests/tests_common.hpp
		tests/type_conversions_test.cpp
		tests/typedefs_errors_test.cpp
		tests/typedefs_test.cpp
		tests/variables_state_merge_test.cpp )

	set( TESTS_HEADERS
		tests/tests.hpp )

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
		py_tests/py_tests.cpp
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
		COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_EXECUTABLE} py_tests/py_tests.py
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

	set( CPP_LINKAGE_SOURCES cpp_linkage_test/cpp_linkage.u )
	CompileUSources( "${CPP_LINKAGE_SOURCES}" )
	set( CPP_LINKAGE_OBJECTS ${U_SOURCE_OBJECTS} )

	add_executable( CPPLinkageTest cpp_linkage_test/cpp_linkage.cpp ${CPP_LINKAGE_OBJECTS} )
	add_dependencies( CPPLinkageTest Compiler )
endif( BUILD_LINKAGE_TESTS )

#
# Inline linkage test
#
if( BUILD_LINKAGE_TESTS )
	# Compile multimodule program on "Ü-Sprache". Each module includes one common file.

	set( INLINE_LINKAGE_SOURCES
		inline_linkage_test/a.u
		inline_linkage_test/b.u
		inline_linkage_test/c.u )
	CompileUSources( "${INLINE_LINKAGE_SOURCES}" )
	set( INLINE_LINKAGE_OBJECTS ${U_SOURCE_OBJECTS} )

	add_executable( InlineLinkageTest inline_linkage_test/entry.cpp ${INLINE_LINKAGE_OBJECTS} )
	add_dependencies( InlineLinkageTest Compiler )
endif( BUILD_LINKAGE_TESTS )

#
# Imports test
#
if( BUILD_LINKAGE_TESTS )

	set( IMPORT_TEST_SOURCES import_test/root.u )
	CompileUSources( "${IMPORT_TEST_SOURCES}" )
	set( IMPORT_TEST_OBJECTS ${U_SOURCE_OBJECTS} )

	add_executable( ImportTest import_test/entry.c ${IMPORT_TEST_OBJECTS} )
	add_dependencies( ImportTest Compiler )
endif( BUILD_LINKAGE_TESTS )
