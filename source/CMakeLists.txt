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
	set( SPRACHE_COMPILER_PIC_OPTIONS --relocation-model pic --enable-pie )
else()
	set( SPRACHE_COMPILER_PIC_OPTIONS "" )
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
llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER
	BitWriter # for writing of result bitcode
	BitReader # for reading of stdlib bitcode
	Linker # for linking of multiple llvm modules
	IPO # for pass manager builder, etc.
	native # "native" is alias for native code generator.
	)
llvm_map_components_to_libnames( LLVM_LIBS_FOR_TESTS ExecutionEngine Interpreter )

get_filename_component( LLVM_UTILS_DIR ${LLVM_INSTALL_DIR}/bin ABSOLUTE )

#
# CompilerLib
#

file( GLOB COMPILER_LIB_SOURCES "code_builder_lib/*" "lex_synt_lib/*" )

add_library( CompilerLib ${COMPILER_LIB_SOURCES} )
target_link_libraries( CompilerLib ${LLVM_LIBS_FOR_COMPILER_LIB} )
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

	add_executable( Compiler ${COMPILER_SOURCES} ${bc_files} )
	target_link_libraries( Compiler CompilerLib ${LLVM_LIBS_FOR_COMPILER} )
	target_link_libraries( Compiler ${Boost_LIBRARIES} )
endif( BUILD_COMPILER )

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
		COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_EXECUTABLE} tests/py_tests/py_tests.py
		DEPENDS PyTestsLib
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		SOURCES ${PY_TESTS_PY_SOURCES} )

endif( BUILD_PY_TESTS )

# Build rule for "Ü" files
# Returns result in U_SOURCE_OBJECT
function( CompileUSource u_source_file )
	get_filename_component( source_name ${u_source_file} NAME_WE )
	set( object_name ${source_name}.o )
	add_custom_command(
		OUTPUT ${object_name}
		DEPENDS ${u_source_file} Compiler
		COMMAND Compiler --produce-object-file ${CMAKE_SOURCE_DIR}/${u_source_file} -o ${object_name} ${SPRACHE_COMPILER_PIC_OPTIONS} )
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
		COMMAND
			Compiler --produce-object-file
			${CMAKE_SOURCE_DIR}/tests/include_dirs_test/root.u -o include_dirs_test.o
			${SPRACHE_COMPILER_PIC_OPTIONS}
			--include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/a
			--include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/b
			--include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/c )

	add_executable( IncludeDirsTest tests/include_dirs_test/entry.c include_dirs_test.o )
	add_dependencies( IncludeDirsTest Compiler )
endif( BUILD_LINKAGE_TESTS )
