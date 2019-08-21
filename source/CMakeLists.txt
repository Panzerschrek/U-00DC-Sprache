cmake_minimum_required(VERSION 2.8)

project(Sprache)
set( PROJECT_VERSION 0.5 )

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

if( BUILD_LINKAGE_TESTS AND BUILD_COMPILER )
	set( REALLY_BUILD_LINKAGE_TESTS YES )
	message( STATUS "Build linkage tests" )
else()
	set( REALLY_BUILD_LINKAGE_TESTS NO )
	message( STATUS "Skip linkage tests" )
endif()

if( BUILD_PY_TESTS )
	message( STATUS "Build py_tests" )
else()
	message( STATUS "Skip py_tests" )
endif( BUILD_PY_TESTS )


set( CMAKE_CXX_STANDARD 11 )

#
# External dependencies
#

# Boost
if( NOT DEFINED BOOST_ROOT )
	message( FATAL_ERROR "BOOST_ROOT not defined. You can define path to boost library, using variable BOOST_ROOT" )
endif()
find_package( Boost )
include_directories( ${Boost_INCLUDE_DIRS} )

# LLVM
if( NOT DEFINED LLVM_SRC_DIR )
	message( FATAL_ERROR "LLVM_SRC_DIR not defined. You can define path to llvm library, using variable LLVM_SRC_DIR" )
endif()

if( REALLY_BUILD_LINKAGE_TESTS )
	set( LLVM_BUILD_TOOLS OFF CACHE BOOL "build llvm tools" )
endif()

set( LLVM_ENABLE_RTTI ON CACHE BOOL "rtti for llvm" )

add_subdirectory( ${LLVM_SRC_DIR} ${CMAKE_BINARY_DIR}/llvm )

include_directories( ${LLVM_SRC_DIR}/include ${CMAKE_BINARY_DIR}/llvm/include )
add_definitions( ${LLVM_DEFINITIONS} )

llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER_LIB Core )
llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER
	BitWriter # for writing of result bitcode
	BitReader # for reading of stdlib bitcode
	Linker # for linking of multiple llvm modules
	IPO # for pass manager builder, etc.
	${LLVM_TARGETS_TO_BUILD}
	)
llvm_map_components_to_libnames( LLVM_LIBS_FOR_TESTS ExecutionEngine Interpreter )

set( LLVM_UTILS_DIR ${CMAKE_BINARY_DIR}/llvm/bin )

#
# options for Ü-Sprache projects
#

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	add_definitions( -DU_DEBUG )
else()
endif()

if( NOT WIN32 )
	add_compile_options( -fPIC )
	set( SPRACHE_COMPILER_PIC_OPTIONS --relocation-model pic --enable-pie )
else()
	set( SPRACHE_COMPILER_PIC_OPTIONS "" )
endif()
add_compile_options( -Wall -Wextra -Wconversion )

#
# LexSyntLib
#

file( GLOB LEX_SYNT_LIB_SOURCES "lex_synt_lib/*" )
add_library( LexSyntLib ${LEX_SYNT_LIB_SOURCES} )

#
# CompilerLib
#

file( GLOB COMPILER_LIB_SOURCES "code_builder_lib/*" )

add_library( CompilerLib ${COMPILER_LIB_SOURCES} )
target_link_libraries( CompilerLib ${LLVM_LIBS_FOR_COMPILER_LIB} )
target_link_libraries( CompilerLib LexSyntLib )
if( NOT WIN32 )
	target_link_libraries( CompilerLib pthread dl )
endif()

#
# Compiler
#
if( BUILD_COMPILER )
	file( GLOB COMPILER_SOURCES "compiler/*" )

	function( CompileLLFile ll_file )
		get_filename_component( source_name ${ll_file} NAME_WE )
		set( bc_name ${source_name}.bc )
		set( bc_object_name ${source_name}.o )
		add_custom_command(
			OUTPUT ${bc_name}
			DEPENDS ${CMAKE_SOURCE_DIR}/ustlib/src/${ll_file}
			DEPENDS ${LLVM_UTILS_DIR}/llvm-as
			COMMAND ${LLVM_UTILS_DIR}/llvm-as ${CMAKE_SOURCE_DIR}/ustlib/src/${ll_file} -o ${bc_name} )
		add_custom_command(
			OUTPUT ${bc_object_name}
			DEPENDS ${bc_name}
			COMMAND ld -r -b binary -o ${bc_object_name} ${bc_name} ) # Convert binary file to object file, using "ld"
		set( BC_FILE ${bc_object_name} PARENT_SCOPE )
	endfunction()

	CompileLLFile( asm_funcs.ll )
	list( APPEND bc_files ${BC_FILE} )
	CompileLLFile( asm_funcs_32.ll )
	list( APPEND bc_files ${BC_FILE} )
	CompileLLFile( asm_funcs_64.ll )
	list( APPEND bc_files ${BC_FILE} )

	# Show in editor also ustlib and ustlib tests files
	file( GLOB_RECURSE USTLIB_SOURCES "ustlib/*.u" "ustlib/src/*.ll" )
	file( GLOB_RECURSE USTLIB_TEST_SOURCES "ustlib_test/*.u" )

	add_executable( Compiler ${COMPILER_SOURCES} ${bc_files} ${USTLIB_SOURCES} ${USTLIB_TEST_SOURCES} )
	target_link_libraries( Compiler CompilerLib ${LLVM_LIBS_FOR_COMPILER} )
	target_compile_definitions( Compiler PRIVATE "SPRACHE_VERSION=${PROJECT_VERSION}" )
endif( BUILD_COMPILER )

#
# CppHeaderConverter
#

if( DEFINED LLVM_EXTERNAL_CLANG_SOURCE_DIR )
	file( GLOB CPP_HEADER_CONVERTER_SOURCES "cpp_header_converter/*" )
	add_executable( CppHeaderConverter ${CPP_HEADER_CONVERTER_SOURCES} )
	target_link_libraries( CppHeaderConverter LexSyntLib )
	target_link_libraries( CppHeaderConverter clangTooling )
	# Hack! Clang libraries does not exports their include directories, set it manually.
	target_include_directories( CppHeaderConverter PRIVATE ${LLVM_EXTERNAL_CLANG_SOURCE_DIR}/include )
	target_include_directories( CppHeaderConverter PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/llvm/tools/clang/include )
endif()

#
# Tests
#
if( BUILD_TESTS )
	file( GLOB TESTS_SOURCES "tests/cpp_tests/*" )

	add_executable( Tests ${TESTS_SOURCES}  )
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

	file( GLOB PY_TESTS_PY_SOURCES "tests/py_tests/*.py" ) # Add all python files to sources list of target.

	# Run result test.
	add_custom_target(
		PyTestsRun ALL
		COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} python3 tests/py_tests/py_tests.py
		DEPENDS PyTestsLib
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		SOURCES ${PY_TESTS_PY_SOURCES} )

endif( BUILD_PY_TESTS )

if( REALLY_BUILD_LINKAGE_TESTS )

	# Build rule for "Ü" files
	# Returns result in U_SOURCE_OBJECT
	function( CompileUSource u_source_file )
		get_filename_component( source_name ${u_source_file} NAME_WE )
		set( object_name ${source_name}.o )
		add_custom_command(
			OUTPUT ${object_name}
			DEPENDS ${u_source_file} Compiler
			COMMAND Compiler ${CMAKE_SOURCE_DIR}/${u_source_file} -o ${object_name} ${SPRACHE_COMPILER_PIC_OPTIONS} )
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
	# Compile simple program on "Ü-Sprache", link it together with C++ program.
	set( CPP_LINKAGE_SOURCES tests/cpp_linkage_test/cpp_linkage.u )
	CompileUSources( "${CPP_LINKAGE_SOURCES}" )
	set( CPP_LINKAGE_OBJECTS ${U_SOURCE_OBJECTS} )

	add_executable( CPPLinkageTest tests/cpp_linkage_test/cpp_linkage.cpp ${CPP_LINKAGE_OBJECTS} )
	add_dependencies( CPPLinkageTest Compiler )

	#
	# Inline linkage test
	#
	# Compile multimodule program on "Ü-Sprache". Each module includes one common file.
	set( INLINE_LINKAGE_SOURCES
		tests/inline_linkage_test/a.u
		tests/inline_linkage_test/b.u
		tests/inline_linkage_test/c.u )
	CompileUSources( "${INLINE_LINKAGE_SOURCES}" )
	set( INLINE_LINKAGE_OBJECTS ${U_SOURCE_OBJECTS} )

	add_executable( InlineLinkageTest tests/inline_linkage_test/entry.cpp ${INLINE_LINKAGE_OBJECTS} )
	add_dependencies( InlineLinkageTest Compiler )

	#
	# Imports test
	#
	set( IMPORT_TEST_SOURCES tests/import_test/root.u )
	CompileUSources( "${IMPORT_TEST_SOURCES}" )
	set( IMPORT_TEST_OBJECTS ${U_SOURCE_OBJECTS} )

	add_executable( ImportTest tests/import_test/entry.c ${IMPORT_TEST_OBJECTS} )
	add_dependencies( ImportTest Compiler )

	#
	# Include dirs test
	#
	add_custom_command(
		OUTPUT include_dirs_test.o
		DEPENDS ${u_source_file} Compiler
		COMMAND
			Compiler
			${CMAKE_SOURCE_DIR}/tests/include_dirs_test/root.u -o include_dirs_test.o
			${SPRACHE_COMPILER_PIC_OPTIONS}
			--include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/a
			--include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/b
			--include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/c )

	add_executable( IncludeDirsTest tests/include_dirs_test/entry.c include_dirs_test.o )
	add_dependencies( IncludeDirsTest Compiler )

	#
	# type_id test
	#
	set( TYPE_ID_TEST_SOURCES tests/type_id_test/type_id_a.u tests/type_id_test/type_id_b.u )
	CompileUSources( "${TYPE_ID_TEST_SOURCES}" )
	set( TYPE_ID_TEST_OBJECTS ${U_SOURCE_OBJECTS} )

	add_executable( TypeIdTest tests/type_id_test/entry.c ${TYPE_ID_TEST_OBJECTS} )
	add_dependencies( TypeIdTest Compiler )

	# Run type_id test
	add_custom_target(
		TypeIdTestRun ALL
		COMMAND TypeIdTest
		DEPENDS TypeIdTest
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR} )

	#
	# CppHeaderConverter test
	#
	if( DEFINED LLVM_EXTERNAL_CLANG_SOURCE_DIR )
		add_custom_command(
			OUTPUT c_converted.u
			DEPENDS tests/cpp_header_converter_test/c_test.h CppHeaderConverter
			COMMAND
				CppHeaderConverter -o c_converted.u
				${CMAKE_SOURCE_DIR}/tests/cpp_header_converter_test/c_test.h -- -std=c99 )

		add_custom_command(
			OUTPUT cpp_header_converter_test.o
			DEPENDS tests/cpp_header_converter_test/cpp_header_converter_test.u  c_converted.u Compiler
			COMMAND
				Compiler
				${CMAKE_SOURCE_DIR}/tests/cpp_header_converter_test/cpp_header_converter_test.u -o cpp_header_converter_test.o
				${SPRACHE_COMPILER_PIC_OPTIONS}
				--include-dir ${CMAKE_BINARY_DIR} )

		add_executable( CppHeaderConverterTest cpp_header_converter_test.o data/entry.cpp tests/cpp_header_converter_test/c_test.c )
		add_dependencies( CppHeaderConverterTest Compiler )
	endif()

endif( REALLY_BUILD_LINKAGE_TESTS )
