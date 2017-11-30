cmake_minimum_required(VERSION 2.8)

project(Sprache)

set( CMAKE_CXX_STANDARD 11 )

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	add_definitions( -DU_DEBUG )
else()
endif()

#
# External dependencies
#

# Boost
set( Boost_USE_STATIC_LIBS yes )
set( BOOST_ROOT ../../boost_1_60_0 ) # Set path to your boost here.
find_package( Boost REQUIRED )
find_package( Boost COMPONENTS system filesystem REQUIRED )
include_directories( ${Boost_INCLUDE_DIRS} )

# LLVM
set( LLVM_INSTALL_DIR ../../llvm-install/ ) # Set path to your LLVM here.
set( LLVM_DIR ${LLVM_INSTALL_DIR}/share/llvm/cmake/ )
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER_LIB Core )
llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER BitWriter AsmParser Linker )
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
	src/code_builder_resolve.cpp
	src/code_builder_templates.cpp
	src/code_builder_types.cpp
	src/keywords.cpp
	src/lang_types.cpp
	src/lexical_analyzer.cpp
	src/mangling.cpp
	src/program_string.cpp
	src/source_graph_loader.cpp
	src/syntax_analyzer.cpp
	src/syntax_elements.cpp )

set( COMPILER_LIB_HEADERS
	src/assert.hpp
	src/code_builder_errors.hpp
	src/code_builder.hpp
	src/code_builder_types.hpp
	src/i_code_builder.hpp
	src/keywords.hpp
	src/lexical_analyzer.hpp
	src/mangling.hpp
	src/pop_llvm_warnings.hpp
	src/program_string.hpp
	src/push_disable_llvm_warnings.hpp
	src/source_graph_loader.hpp
	src/syntax_elements.hpp
	src/syntax_analyzer.hpp )

add_library( CompilerLib ${COMPILER_LIB_SOURCES} ${COMPILER_LIB_HEADERS} )
target_link_libraries( CompilerLib ${LLVM_LIBS_FOR_COMPILER_LIB} )

#
# Compiler
#

set( COMPILER_SOURCES
	src/main.cpp )

set( COMPILER_HEADERS
	src/stdlib_asm.hpp )

add_executable( Compiler ${COMPILER_SOURCES} ${COMPILER_HEADERS} )
target_link_libraries( Compiler CompilerLib ${LLVM_LIBS_FOR_COMPILER} )
target_link_libraries( Compiler ${Boost_LIBRARIES} )

#
# Tests
#

set( TESTS_SOURCES
	src/tests/auto_variables_test.cpp
	src/tests/auto_variables_errors_test.cpp
	src/tests/class_templates_errors_test.cpp
	src/tests/class_templates_test.cpp
	src/tests/classes_declaration_errors_test.cpp
	src/tests/classes_declaration_test.cpp
	src/tests/code_builder_errors_test.cpp
	src/tests/code_builder_test.cpp
	src/tests/constexpr_errors_test.cpp
	src/tests/constexpr_test.cpp
	src/tests/constructors_errors_test.cpp
	src/tests/constructors_test.cpp
	src/tests/destructors_errors_test.cpp
	src/tests/destructors_test.cpp
	src/tests/global_variables_errors_test.cpp
	src/tests/global_variables_test.cpp
	src/tests/halt_test.cpp
	src/tests/imports_errors_test.cpp
	src/tests/imports_test.cpp
	src/tests/initializers_errors_test.cpp
	src/tests/initializers_test.cpp
	src/tests/mangling_test.cpp
	src/tests/methods_errors_test.cpp
	src/tests/methods_test.cpp
	src/tests/mutability_errors_test.cpp
	src/tests/namespaces_errors_test.cpp
	src/tests/namespaces_test.cpp
	src/tests/operators_overloading_errors_test.cpp
	src/tests/operators_overloading_test.cpp
	src/tests/operators_priority_test.cpp
	src/tests/operators_test.cpp
	src/tests/reference_check_test.cpp
	src/tests/tests_main.cpp
	src/tests/tests.cpp
	src/tests/type_conversions_test.cpp
	src/tests/typedefs_errors_test.cpp
	src/tests/typedefs_test.cpp )

set( TESTS_HEADERS
	src/tests/tests.hpp )

add_executable( Tests ${TESTS_SOURCES} ${TESTS_HEADERS} )
target_link_libraries( Tests CompilerLib ${LLVM_LIBS_FOR_TESTS} )

#
# CPP-Linkage test
#

# Compile simple program on "Ü-Sprache", link it together with C++ program.

set( CPP_LINKAGE_U_SOURCE test_programs/cpp_linkage.u )
set( CPP_LINKAGE_U_MODULE cpp_linkage_u.obj )
set( CPP_LINKAGE_CPP_SOURCE test_programs/cpp_linkage.cpp )

add_custom_command(
	OUTPUT ${CPP_LINKAGE_U_MODULE}
	DEPENDS ${CPP_LINKAGE_U_SOURCE} Compiler
	COMMAND Compiler -i ${CMAKE_SOURCE_DIR}/test_programs/cpp_linkage.u -o cpp_linkage.ir &&
	${LLVM_UTILS_DIR}/llc cpp_linkage.ir -filetype=obj -o=${CPP_LINKAGE_U_MODULE} )

add_executable( CPPLinkageTest ${CPP_LINKAGE_U_MODULE} ${CPP_LINKAGE_CPP_SOURCE} )
add_dependencies( CPPLinkageTest Compiler )
