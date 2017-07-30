cmake_minimum_required(VERSION 2.8)

project(Sprache)

set( CMAKE_CXX_STANDARD 11 )

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	add_definitions( -DU_DEBUG )
else()
endif()

#
# CompilerLib
#

set( COMPILER_LIB_SOURCES
	src/code_builder_errors.cpp
	src/code_builder.cpp
	src/code_builder_expressions.cpp
	src/code_builder_initializers.cpp
	src/code_builder_types.cpp
	src/keywords.cpp
	src/lang_types.cpp
	src/lexical_analyzer.cpp
	src/mangling.cpp
	src/program_string.cpp
	src/syntax_analyzer.cpp
	src/syntax_elements.cpp )

set( COMPILER_LIB_HEADERS
	src/assert.hpp
	src/code_builder_errors.hpp
	src/code_builder.hpp
	src/code_builder_types.hpp
	src/keywords.hpp
	src/lexical_analyzer.hpp
	src/mangling.hpp
	src/pop_llvm_warnings.hpp
	src/program_string.hpp
	src/push_disable_llvm_warnings.hpp
	src/syntax_elements.hpp
	src/syntax_analyzer.hpp )

set( LLVM_BASE_DIR ../../llvm-3.7.1.src )
set( LLVM_BUILD_DIR ${LLVM_BASE_DIR}/build )
set( LLVM_LIBS_DIR ${LLVM_BUILD_DIR}/lib )
set( LLVM_INCLUDES_DIR ${LLVM_BASE_DIR}/include )
set( LLVM_GEN_INCLUDES_DIR ${LLVM_BUILD_DIR}/include )
set( BOOST_BASE_DIR ../../boost_1_60_0 )

include_directories( ${LLVM_INCLUDES_DIR} )
include_directories( ${LLVM_GEN_INCLUDES_DIR} )
include_directories( ${BOOST_BASE_DIR} )

# We use separate-compiled llvm libraries here, because using llvm-cmake project is too slow.

set( LLVM_LIBS
	libLLVMInterpreter.a
	libLLVMExecutionEngine.a
	libLLVMCodeGen.a
	libLLVMCore.a
	libLLVMMC.a
	libLLVMSupport.a
	libLLVMTarget.a
	libLLVMAsmParser.a
	libLLVMBitWriter.a )

add_library( CompilerLib ${COMPILER_LIB_SOURCES} ${COMPILER_LIB_HEADERS} )
target_link_libraries( CompilerLib ${LLVM_LIBS} )
link_directories( ${LLVM_LIBS_DIR} )

#
# Compiler
#

set( COMPILER_SOURCES
	src/main.cpp )

add_executable( Compiler ${COMPILER_SOURCES} )
target_link_libraries( Compiler CompilerLib )

#
# Tests
#

set( TESTS_SOURCES
	src/tests/auto_variables_test.cpp
	src/tests/auto_variables_errors_test.cpp
	src/tests/classes_declaration_errors_test.cpp
	src/tests/classes_declaration_test.cpp
	src/tests/code_builder_errors_test.cpp
	src/tests/code_builder_test.cpp
	src/tests/initializers_errors_test.cpp
	src/tests/initializers_test.cpp
	src/tests/methods_errors_test.cpp
	src/tests/methods_test.cpp
	src/tests/namespaces_errors_test.cpp
	src/tests/namespaces_test.cpp
	src/tests/operators_priority_test.cpp
	src/tests/tests_main.cpp
	src/tests/tests.cpp )

set( TESTS_HEADERS
	src/tests/tests.hpp )

add_executable( Tests ${TESTS_SOURCES} ${TESTS_HEADERS} )
target_link_libraries( Tests CompilerLib )

