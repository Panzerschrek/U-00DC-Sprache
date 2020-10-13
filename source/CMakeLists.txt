cmake_minimum_required(VERSION 3.10)

project(Sprache)

option( U_BUILD_COMPILER "Enable compilation of compiler" YES )
option( U_BUILD_TESTS "Enable compilation of base tests" YES )
option( U_BUILD_LINKAGE_TESTS "Enable compilation of linkage tests" YES )
option( U_BUILD_PY_TESTS "Enable compilation of py_tests" YES )
option( U_BUILD_CPP_HEADER_CONVERTER "Enable compilation of c++ header converter (clang required)" YES )
option( U_BUILD_UNICODE_FILE_NAMES_TEST "Enable compilation of unicode filenames test (such test may not work correctly is some environmens)" YES )
option( U_BUILD_DOCS "Enable Ǖ docs (Sphinx Required)" YES )
option( U_BUILD_COMPILER1 "Build compiler1 (written on Ǖ)" YES)

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

# Python

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

	list( APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -Wl,--gc-sections" )
	list( APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s -Wl,--gc-sections" )

endif()

add_subdirectory(sprache_version)

#
# Generated stuff
#

file( READ keywords_list.txt KEYWORDS_LIST_CONTENT )
set( KEYWORDS_LIST_REGEXP "[A-Za-z][A-Za-z0-9_]*" )

#
# CodeBuilderLibCommon
#

file( GLOB CODE_BUILDER_COMMON_SOURCES "code_builder_lib_common/*" )
add_library( CodeBuilderLibCommon ${CODE_BUILDER_COMMON_SOURCES} )
target_link_libraries( CodeBuilderLibCommon ${LLVM_LIBS_FOR_CODE_BUILDER_LIB} )

#
# TestsLib
#
if( U_BUILD_TESTS )

	#
	# C++ tests
	#
	file( GLOB TESTS_SOURCES "tests/cpp_tests/*" )
	add_library( TestsLib ${TESTS_SOURCES} )
	target_link_libraries( TestsLib PUBLIC ${LLVM_LIBS_FOR_TESTS} )

	# Hack! Force use static variables with initializers with tests registration
	set( TESTS_LIB_WRAPPED TestsLib )
	if( NOT MSVC )
		set( TESTS_LIB_WRAPPED -Wl,--whole-archive ${TESTS_LIB_WRAPPED} -Wl,--no-whole-archive )
	endif()

endif()

add_subdirectory( compiler0 )


if( REALLY_BUILD_LINKAGE_TESTS )

	# Build rule for "Ü" files
	# Returns result in ${out_object}
	function( CompileUSource u_source_file out_object )
		file( RELATIVE_PATH source_name_rel ${CMAKE_CURRENT_SOURCE_DIR} ${u_source_file} )
		set( object_name ${CMAKE_CURRENT_BINARY_DIR}/${source_name_rel}.o )
		add_custom_command(
			OUTPUT ${object_name}
			DEPENDS ${u_source_file} Compiler
			COMMAND Compiler ${u_source_file} -o ${object_name} ${SPRACHE_COMPILER_PIC_OPTIONS} )
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

	add_subdirectory( tests/cpp_header_converter_test )
	add_subdirectory( tests/cpp_linkage_test )
	add_subdirectory( tests/debug_info_test )
	add_subdirectory( tests/dep_file_test )
	add_subdirectory( tests/import_test )
	add_subdirectory( tests/include_dirs_test )
	add_subdirectory( tests/inline_linkage_test )
	add_subdirectory( tests/type_id_test )
	add_subdirectory( tests/unicode_file_names_test )

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


if( U_BUILD_COMPILER AND U_BUILD_COMPILER1 )
	set( CURRENT_COMPILER Compiler )
	set( CURRENT_COMPLIER_GENERATION 1 )
	add_subdirectory( compiler1 )
endif()
