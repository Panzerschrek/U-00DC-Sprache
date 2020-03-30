cmake_minimum_required(VERSION 2.8)

project(Sprache)

option( U_BUILD_COMPILER "Enable compilation of compiler" YES )
option( U_BUILD_TESTS "Enable compilation of base tests" YES )
option( U_BUILD_LINKAGE_TESTS "Enable compilation of linkage tests" YES )
option( U_BUILD_PY_TESTS "Enable compilation of py_tests" YES )
option( U_BUILD_CPP_HEADER_CONVERTER "Enable compilation of c++ header converter (clang required)" YES )
option( U_BUILD_UNICODE_FILE_NAMES_TEST "Enable compilation of unicode filenames test (such test may not work correctly is some environmens)" YES )
option( U_BUILD_DOCS "Enable Ǖ docs (Sphinx Required)" YES )

if( U_BUILD_COMPILER )
	message( STATUS "Build compiler" )
else()
	message( STATUS "Skip compiler" )
endif()

if( U_BUILD_TESTS )
	message( STATUS "Build tests" )
else()
	message( STATUS "Skip tests" )
endif()

if( U_BUILD_LINKAGE_TESTS AND U_BUILD_COMPILER )
	set( REALLY_BUILD_LINKAGE_TESTS YES )
	message( STATUS "Build linkage tests" )
else()
	set( REALLY_BUILD_LINKAGE_TESTS NO )
	message( STATUS "Skip linkage tests" )
endif()

if( U_BUILD_PY_TESTS )
	message( STATUS "Build py_tests" )
else()
	message( STATUS "Skip py_tests" )
endif()

# Setup C++ version
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED On )
set( CMAKE_CXX_EXTENSIONS Off )
if( MSVC )
	add_definitions( -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS )
endif()

#
# External dependencies
#

# LLVM

if( DEFINED LLVM_SRC_DIR )

	add_subdirectory( ${LLVM_SRC_DIR} ${CMAKE_BINARY_DIR}/llvm )

	include_directories( ${LLVM_SRC_DIR}/include ${CMAKE_BINARY_DIR}/llvm/include )

	# llvm does not export definitions as target properties, so, retrieve compile definitions manually.
	get_directory_property( LLVM_COMPILE_DEFINITIONS DIRECTORY ${LLVM_SRC_DIR} COMPILE_DEFINITIONS )
	foreach( d ${LLVM_COMPILE_DEFINITIONS} )
		add_definitions( "-D${d}" )
	endforeach()

elseif( DEFINED LLVM_LIB_DIR )

	set( CMAKE_PREFIX_PATH
		${CMAKE_PREFIX_PATH}
		${LLVM_LIB_DIR}/cmake/llvm
		${LLVM_LIB_DIR}/cmake/clang )

	set( CMAKE_MODULE_PATH
		${CMAKE_MODULE_PATH}
		${LLVM_LIB_DIR}/cmake/llvm
		${LLVM_LIB_DIR}/cmake/clang )

	find_package( LLVM REQUIRED )
	find_package( Clang ) # Optional
	find_package( Sphinx ) # Optional

	include_directories( ${LLVM_INCLUDE_DIRS} )

else()
	message( FATAL_ERROR "llvm not found. Define LLVM_SRC_DIR for building llvm from sources, or define LLVM_LIB_DIR for installed llvm." )
endif()

llvm_map_components_to_libnames( LLVM_LIBS_FOR_CODE_BUILDER_LIB Core )
llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER
	Linker # for linking of multiple llvm modules
	IPO # for pass manager builder, etc.
	${LLVM_TARGETS_TO_BUILD}
	)
llvm_map_components_to_libnames( LLVM_LIBS_FOR_TESTS Interpreter )

set( REALLY_BUILD_CPP_HEADER_CONVERTER NO )
if( U_BUILD_CPP_HEADER_CONVERTER )
	if( TARGET clangTooling )
		set( REALLY_BUILD_CPP_HEADER_CONVERTER YES )
		message( STATUS "Build c++ header converter" )
	else()
		message( STATUS "Clang not found, c++ header converter build disabled" )
	endif()
else()
	message( STATUS "Skip c++ header converter" )
endif()

#
# Compiler options for Ü-Sprache projects
#

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	add_definitions( -DU_DEBUG )
endif()

if( WIN32 )
	set( SPRACHE_COMPILER_PIC_OPTIONS "" )
else()
	add_compile_options( -fPIC )
	set( SPRACHE_COMPILER_PIC_OPTIONS --relocation-model pic )
endif()

if(MSVC)

	add_compile_options( /utf-8 /W3 /WX )

	if( NOT LLVM_ENABLE_RTTI )
		add_compile_options( /GR- )
	endif()

else()
	add_compile_options( -Werror -Wall -Wextra -Wconversion )
	add_compile_options( -fvisibility=hidden -fvisibility-inlines-hidden )

	if( NOT LLVM_ENABLE_RTTI )
		add_compile_options( -fno-rtti )
	endif()

	list( APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE -s )
	list( APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE -s )

endif()

add_subdirectory(sprache_version)

#
# LexSyntLib
#

file( GLOB LEX_SYNT_LIB_SOURCES "lex_synt_lib/*" )
add_library( LexSyntLib ${LEX_SYNT_LIB_SOURCES} )

#
# CodeBuilderLib
#

file( GLOB COMPILER_LIB_SOURCES "code_builder_lib/*" )
add_library( CodeBuilderLib ${COMPILER_LIB_SOURCES} )
target_link_libraries( CodeBuilderLib LexSyntLib SpracheVersion ${LLVM_LIBS_FOR_CODE_BUILDER_LIB} )

#
# Compiler
#
if( U_BUILD_COMPILER )
	file( GLOB COMPILER_SOURCES "compiler/*" )

	function( CompileLLFile ll_file )
		get_filename_component( source_name ${ll_file} NAME_WE )
		set( bc_name ${source_name}.bc )
		set( bc_header_path ${CMAKE_CURRENT_BINARY_DIR}/${source_name}.h )
		add_custom_command(
			OUTPUT ${bc_name}
			DEPENDS ${CMAKE_SOURCE_DIR}/ustlib/src/${ll_file}
			DEPENDS llvm-as
			COMMAND llvm-as ${CMAKE_SOURCE_DIR}/ustlib/src/${ll_file} -o ${bc_name} )
		add_custom_command(
			OUTPUT ${bc_header_path}
			DEPENDS ${bc_name}
			DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bin2c.cmake ${bc_name}
			COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/bin2c.cmake ${bc_name} ${bc_header_path} c_${source_name}_file_content
			)
		set( BC_FILE ${bc_header_path} PARENT_SCOPE )
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
	target_link_libraries( Compiler CodeBuilderLib ${LLVM_LIBS_FOR_COMPILER} )
	target_include_directories( Compiler PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )
endif()

#
# CppHeaderConverter
#

if( REALLY_BUILD_CPP_HEADER_CONVERTER )
	file( GLOB CPP_HEADER_CONVERTER_SOURCES "cpp_header_converter/*" )
	add_executable( CppHeaderConverter ${CPP_HEADER_CONVERTER_SOURCES} )
	target_link_libraries( CppHeaderConverter LexSyntLib clangTooling )
	# Hack! Clang libraries does not exports their include directories, set it manually.
	target_include_directories( CppHeaderConverter PRIVATE ${LLVM_EXTERNAL_CLANG_SOURCE_DIR}/include )
	target_include_directories( CppHeaderConverter PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/llvm/tools/clang/include )
endif()

#
# Tests
#
if( U_BUILD_TESTS )
	file( GLOB TESTS_SOURCES "tests/cpp_tests/*" )
	add_executable( Tests ${TESTS_SOURCES} )
	target_link_libraries( Tests CodeBuilderLib ${LLVM_LIBS_FOR_TESTS} )
	add_custom_command( TARGET Tests POST_BUILD COMMAND Tests )
endif()

#
# PyTests
#
if( U_BUILD_PY_TESTS )
	# Fix python search on systems with both python2/python3 installed.
	# LLVM CMake file calls "FindPythonInterp" without version specified, so, version 2 may be found.
	# If python2 found, search for python3 failed. To fix that, reset related cache variables.
	unset( PYTHON_EXECUTABLE CACHE )
	unset( PYTHONINTERP_FOUND CACHE )
	unset( PYTHON_VERSION_STRING CACHE )
	unset( PYTHON_VERSION_MAJOR CACHE )
	unset( PYTHON_VERSION_MINOR CACHE )
	unset( PYTHON_VERSION_PATCH CACHE )

	find_package( PythonLibs 3 REQUIRED )
	find_package( PythonInterp 3 REQUIRED )

	set( PY_TESTS_LIB_SOURCES
		tests/py_tests_lib/py_tests_lib.cpp
		tests/tests_common.hpp )

	add_library( PyTestsLib SHARED ${PY_TESTS_LIB_SOURCES} )
	target_link_libraries( PyTestsLib CodeBuilderLib ${LLVM_LIBS_FOR_TESTS} ${PYTHON_LIBRARIES} )
	target_include_directories( PyTestsLib PRIVATE ${PYTHON_INCLUDE_DIRS} )

	# Set proper name for result dynamic library.
	if( WIN32 )
		set_target_properties( PyTestsLib
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.pyd PREFIX "" SUFFIX "" )
	else()
		set_target_properties( PyTestsLib
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.so PREFIX "" SUFFIX "" )
	endif()

	file( GLOB PY_TESTS_SOURCES "tests/py_tests/*.py" ) # Add all python files to sources list of target.

	# Run result test.
	add_custom_target(
		PyTestsRun ALL
		COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_EXECUTABLE} tests/py_tests/py_tests.py
		DEPENDS PyTestsLib
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		SOURCES ${PY_TESTS_SOURCES} )

endif()

if( REALLY_BUILD_LINKAGE_TESTS )

	# Build rule for "Ü" files
	# Returns result in ${out_object}
	function( CompileUSource u_source_file out_object )
		get_filename_component( source_name ${u_source_file} NAME_WE )
		set( object_name ${source_name}.o )
		add_custom_command(
			OUTPUT ${object_name}
			DEPENDS ${u_source_file} Compiler
			COMMAND Compiler ${CMAKE_SOURCE_DIR}/${u_source_file} -o ${object_name} ${SPRACHE_COMPILER_PIC_OPTIONS} )
		set( ${out_object} ${object_name} PARENT_SCOPE )
	endfunction()

	# Returns list of objects in ${out_objects}
	function( CompileUSources u_source_files out_objects )
		foreach( source ${u_source_files} )
			CompileUSource( ${source} out_object )
			list( APPEND out_objects_local ${out_object} )
		endforeach()
		set( ${out_objects} ${out_objects_local} PARENT_SCOPE )
	endfunction()

	#
	# CPP-Linkage test
	#
	# Compile simple program on "Ü-Sprache", link it together with C++ program.
	set( CPP_LINKAGE_SOURCES tests/cpp_linkage_test/cpp_linkage.u )
	CompileUSources( "${CPP_LINKAGE_SOURCES}" CPP_LINKAGE_OBJECTS )

	add_executable( CPPLinkageTest tests/cpp_linkage_test/cpp_linkage.cpp ${CPP_LINKAGE_SOURCES} ${CPP_LINKAGE_OBJECTS} )
	add_dependencies( CPPLinkageTest Compiler )

	#
	# Inline linkage test
	#
	# Compile multimodule program on "Ü-Sprache". Each module includes one common file.
	set( INLINE_LINKAGE_SOURCES
		tests/inline_linkage_test/a.u
		tests/inline_linkage_test/b.u
		tests/inline_linkage_test/c.u )
	CompileUSources( "${INLINE_LINKAGE_SOURCES}" INLINE_LINKAGE_OBJECTS )

	add_executable( InlineLinkageTest tests/inline_linkage_test/entry.cpp ${INLINE_LINKAGE_SOURCES} ${INLINE_LINKAGE_OBJECTS} )
	add_dependencies( InlineLinkageTest Compiler )

	#
	# Imports test
	#
	set( IMPORT_TEST_SOURCES tests/import_test/root.u )
	CompileUSources( "${IMPORT_TEST_SOURCES}" IMPORT_TEST_OBJECTS )

	add_executable( ImportTest tests/import_test/entry.cpp ${IMPORT_TEST_SOURCES} ${IMPORT_TEST_OBJECTS} )
	add_dependencies( ImportTest Compiler )

	#
	# Unciode file names test
	#
	if( U_BUILD_UNICODE_FILE_NAMES_TEST )
		set( UNICODE_FILE_NAMES_SOURCES "tests/unicode_file_names_test/Entry ФшЯё_66.ü" )
		CompileUSources( "${UNICODE_FILE_NAMES_SOURCES}" UNICODE_FILE_NAMES_OBJECTS )

		add_executable( UnicodeFileNamesTest tests/unicode_file_names_test/entry.cpp ${UNICODE_FILE_NAMES_SOURCES} ${UNICODE_FILE_NAMES_OBJECTS} )
		add_dependencies( UnicodeFileNamesTest Compiler )
	endif()

	#
	# Include dirs test
	#
	add_custom_command(
		OUTPUT include_dirs_test.o
		DEPENDS Compiler
		COMMAND
			Compiler
			${CMAKE_SOURCE_DIR}/tests/include_dirs_test/root.u -o include_dirs_test.o
			${SPRACHE_COMPILER_PIC_OPTIONS}
			--include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/a
			--include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/b
			--include-dir ${CMAKE_SOURCE_DIR}/tests/include_dirs_test/c )

	add_executable( IncludeDirsTest tests/include_dirs_test/entry.cpp tests/include_dirs_test/root.u include_dirs_test.o )
	add_dependencies( IncludeDirsTest Compiler )

	#
	# type_id test
	#
	set( TYPE_ID_TEST_SOURCES tests/type_id_test/type_id_a.u tests/type_id_test/type_id_b.u )
	CompileUSources( "${TYPE_ID_TEST_SOURCES}" TYPE_ID_TEST_OBJECTS )

	add_executable( TypeIdTest tests/type_id_test/entry.cpp ${TYPE_ID_TEST_SOURCES} ${TYPE_ID_TEST_OBJECTS} )
	add_dependencies( TypeIdTest Compiler )

	# Run type_id test
	add_custom_command( TARGET TypeIdTest POST_BUILD COMMAND TypeIdTest )

	#
	# Debug info test
	#
	set( DEBUG_INFO_TEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tests/debug_info_test/debug_info_test.u )
	set( DEBUG_INFO_TEST_FILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/debug_info_test.o )
	add_custom_command(
		OUTPUT ${DEBUG_INFO_TEST_FILE_OUT}
		DEPENDS Compiler ${DEBUG_INFO_TEST_FILE}
		COMMAND
			Compiler
			${DEBUG_INFO_TEST_FILE} -o ${DEBUG_INFO_TEST_FILE_OUT}
			${SPRACHE_COMPILER_PIC_OPTIONS}
			-O0 # Disable optimization
			-g # Generate debug info
		)

	add_executable( DebugInfoTest tests/debug_info_test/entry.cpp ${DEBUG_INFO_TEST_FILE} ${DEBUG_INFO_TEST_FILE_OUT} )
	add_dependencies( DebugInfoTest Compiler )

	#
	# CppHeaderConverter test
	#
	if( REALLY_BUILD_CPP_HEADER_CONVERTER )
		add_custom_command(
			OUTPUT c_converted.u
			DEPENDS tests/cpp_header_converter_test/c_test.h CppHeaderConverter
			COMMAND
				CppHeaderConverter -o c_converted.u
				${CMAKE_SOURCE_DIR}/tests/cpp_header_converter_test/c_test.h -- -std=c11 )

		add_custom_command(
			OUTPUT cpp_header_converter_test.o
			DEPENDS tests/cpp_header_converter_test/cpp_header_converter_test.u  c_converted.u Compiler
			COMMAND
				Compiler
				${CMAKE_SOURCE_DIR}/tests/cpp_header_converter_test/cpp_header_converter_test.u -o cpp_header_converter_test.o
				${SPRACHE_COMPILER_PIC_OPTIONS}
				--include-dir ${CMAKE_BINARY_DIR} )

		add_executable( CppHeaderConverterTest cpp_header_converter_test.o data/entry.cpp tests/cpp_header_converter_test/c_test.cpp )
		add_dependencies( CppHeaderConverterTest Compiler )
	endif()

endif( REALLY_BUILD_LINKAGE_TESTS )

#
# Docs
#

if( U_BUILD_DOCS )
	if( LLVM_ENABLE_SPHINX OR Sphinx_FOUND )
		message( "Include Ü docs" )
		add_subdirectory( docs )
	else()
		message( "Sphinx not found, skip Ü docs" )
	endif()
endif()
