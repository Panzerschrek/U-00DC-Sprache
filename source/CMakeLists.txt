cmake_minimum_required(VERSION 3.13.4)

project(Sprache)

option( U_EMSCRIPTEN "Build for emscripten" NO )
if( U_EMSCRIPTEN )
	set( CMAKE_CROSSCOMPILING YES )
endif()

option( U_BUILD_COMPILER "Enable compilation of compiler" YES )
option( U_BUILD_TESTS "Enable compilation of base tests" YES )
option( U_BUILD_LINKAGE_TESTS "Enable compilation of linkage tests" YES )
option( U_BUILD_PY_TESTS "Enable compilation of py_tests" YES )
option( U_BUILD_CPP_HEADER_CONVERTER "Enable compilation of c++ header converter (clang required)" YES )
option( U_BUILD_INTERPRETER "Build Interpreter" YES )
option( U_BUILD_LANGUAGE_SERVER "Build language server" YES )
option( U_BUILD_UNICODE_FILE_NAMES_TEST "Enable compilation of unicode filenames test (such test may not work correctly is some environmens)" YES )
option( U_BUILD_DOCS "Enable Ü docs (Sphinx Required)" YES )
option( U_BUILD_EXAMPLES "Build examples" YES )
option( U_BUILD_COMPILER1 "Build compiler1 (written on Ü)" YES)
option( U_BUILD_COMPILER2 "Build compiler2 (using compiler1)" NO)
option( U_BUILD_COMPILER3 "Build compiler3 (using compiler3)" NO)
option( U_COVERAGE_ENABLED "Enable gcov options for the build" NO)

# Support versions of CMake prior to 3.20 and use old-style of Ninja dep-files handling.
if( CMAKE_VERSION VERSION_GREATER_EQUAL "3.20" )
	cmake_policy( SET CMP0116 OLD )
endif()

# Setup C++ version
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED On )
set( CMAKE_CXX_EXTENSIONS Off )
if( MSVC )
	add_definitions( -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS )
endif()

#
# Global codegen options (for both Ü and dependencies).
#

if( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" )
	message( "Target architecture seems to be x86_64 (${CMAKE_SYSTEM_PROCESSOR}), turn on modern instructions support" )

	# Use a CPU from year 2015 or newer. It seems to be fine to avoid using older CPUs.
	set( SPRACHE_TARGET_CPU_NAME skylake )
	set( SPRACHE_COMPILER_ARCH_OPTIONS -mcpu=${SPRACHE_TARGET_CPU_NAME} )

	if(MSVC)
		add_compile_options( /arch:AVX2 )
	else()
		add_compile_options( -march=${SPRACHE_TARGET_CPU_NAME} )
	endif()

else()
	message( "Target architecture is not x86_64 (${CMAKE_SYSTEM_PROCESSOR})" )
endif()

#
# External dependencies
#

# LLVM

if( DEFINED LLVM_SRC_DIR )
	message( STATUS "Build LLVM from sources" )

	add_subdirectory( ${LLVM_SRC_DIR} ${CMAKE_BINARY_DIR}/llvm EXCLUDE_FROM_ALL )

	include_directories( ${LLVM_SRC_DIR}/include ${CMAKE_BINARY_DIR}/llvm/include )

	# llvm does not export definitions as target properties, so, retrieve compile definitions manually.
	get_directory_property( LLVM_COMPILE_DEFINITIONS DIRECTORY ${LLVM_SRC_DIR} COMPILE_DEFINITIONS )
	foreach( d ${LLVM_COMPILE_DEFINITIONS} )
		add_definitions( "-D${d}" )
	endforeach()

	# Hack! Can't get LLVM version from sources, so, hardcode it.
	set( CLANG_BUILTIN_HEADERS_FOR_CPP_HEADER_CONVERTER ${CMAKE_BINARY_DIR}/llvm/lib/clang/15.0.7/include/ )
	set( CLANG_BUILTIN_HEADERS_INSTALL_DESTINATION lib/clang/15.0.7/include/ )

elseif( DEFINED LLVM_LIB_DIR )
	message( STATUS "Use binary LLVM package" )

	set( CMAKE_PREFIX_PATH
		${CMAKE_PREFIX_PATH}
		${LLVM_LIB_DIR}/cmake/lld
		${LLVM_LIB_DIR}/cmake/llvm
		${LLVM_LIB_DIR}/cmake/clang )

	set( CMAKE_MODULE_PATH
		${CMAKE_MODULE_PATH}
		${LLVM_LIB_DIR}/cmake/lld
		${LLVM_LIB_DIR}/cmake/llvm
		${LLVM_LIB_DIR}/cmake/clang )

	find_package( LLVM REQUIRED )
	find_package( LLD ) # Optional
	find_package( Clang ) # Optional
	find_package( Sphinx ) # Optional

	include_directories( ${LLVM_INCLUDE_DIRS} )

	set( CLANG_BUILTIN_HEADERS_FOR_CPP_HEADER_CONVERTER ${LLVM_LIB_DIR}/clang/${LLVM_VERSION}/include/ )
	set( CLANG_BUILTIN_HEADERS_INSTALL_DESTINATION lib/clang/${LLVM_VERSION}/include/ )

else()
	message( FATAL_ERROR "llvm not found. Define LLVM_SRC_DIR for building llvm from sources, or define LLVM_LIB_DIR for installed llvm." )
endif()

llvm_map_components_to_libnames( LLVM_LIBS_FOR_CODE_BUILDER_LIB Core Target TransformUtils )
llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILERS_SUPPORT_LIB Core MC )
llvm_map_components_to_libnames( LLVM_LIBS_FOR_COMPILER
	IPO # for pass manager builder, etc.
	Linker # for linking of multiple llvm modules.
	Passes # for new PassManager.
	${LLVM_TARGETS_TO_BUILD}
	)
llvm_map_components_to_libnames( LLVM_LIBS_FOR_INTERPRETER Interpreter MCJIT ${LLVM_NATIVE_ARCH} Linker )

# Python

if( U_BUILD_PY_TESTS )
	message( STATUS "Build py_tests" )

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

else()
	message( STATUS "Skip py_tests" )
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

	# 32 MB stack for deep recursion. Necessary for things like VariantLinkedList.
	add_link_options("/STACK:33554432")

else()
	add_compile_options( -Werror -Wall -Wextra -Wconversion )
	add_compile_options( -fvisibility=hidden -fvisibility-inlines-hidden )
	add_compile_options( -fdata-sections -ffunction-sections ) # simplify later gc-sections.

	if( U_COVERAGE_ENABLED )
		add_compile_options( --coverage )
		add_link_options( --coverage )
	endif()

	# Disable warnings, related to "std::move" in new versions of GCC.
	# We can't just fix these warnings since such fixes may break build with old versions of GCC.
	if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
		if( CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "9" )
			message( STATUS "GCC >= 9 detected, disable std::move related warnings" )
			add_compile_options( -Wno-pessimizing-move -Wno-redundant-move )
		endif()
	endif()

	if( NOT LLVM_ENABLE_RTTI )
		add_compile_options( -fno-rtti )
	endif()

	# Avoid undefined symbols in shared libraries.
	string( APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined" )
	# Clear garbage.
	string( APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " -s -Wl,--gc-sections" )
	string( APPEND CMAKE_SHARED_LINKER_FLAGS_RELEASE " -s -Wl,--gc-sections" )

	# 32 MB stack for deep recursion. Necessary for things like VariantLinkedList.
	add_link_options( "-Wl,-z,stack-size=33554432" )

endif()

#
# Generated stuff
#

file( READ keywords_list.txt KEYWORDS_LIST_CONTENT )
set( KEYWORDS_LIST_REGEXP "[A-Za-z][A-Za-z0-9_]*" )

#
# Common libraries
#
add_subdirectory( sprache_version )
add_subdirectory( lex_synt_lib_common )
add_subdirectory( code_builder_lib_common )
add_subdirectory( compilers_support_lib )
add_subdirectory( compilers_common_lib )
add_subdirectory( tests/tests_lib )

add_subdirectory( tests/cpp_tests )
# Hack! Force use static variables with initializers with tests registration
set( CPP_TESTS_LIB_WRAPPED CppTestsLib )
if( NOT MSVC )
	set( CPP_TESTS_LIB_WRAPPED -Wl,--whole-archive ${CPP_TESTS_LIB_WRAPPED} -Wl,--no-whole-archive )
endif()

add_subdirectory( compiler0 )

#
# Compiler1 and other generations
#
if( U_BUILD_COMPILER AND U_BUILD_COMPILER1 )

	set( CURRENT_COMPILER_GENERATION "" )
	set( NEXT_COMPILER_GENERATION "1" )
	add_subdirectory( compiler1 )

	if( U_BUILD_COMPILER2 )

		set( CURRENT_COMPILER_GENERATION "1" )
		set( NEXT_COMPILER_GENERATION "2" )
		add_subdirectory( compiler1 compiler2 )

		if( U_BUILD_COMPILER3 )

			set( CURRENT_COMPILER_GENERATION "2" )
			set( NEXT_COMPILER_GENERATION "3" )
			add_subdirectory( compiler1 compiler3 )

		endif()

	endif()

endif()

#
# CppHeaderConverter
#
if( U_BUILD_CPP_HEADER_CONVERTER )
	add_subdirectory( cpp_header_converter )
else()
	message( STATUS "Skip c++ header converter" )
endif()

#
# Interpreter
#
if( U_BUILD_INTERPRETER )
	message( STATUS "Build Interpreter" )
	add_subdirectory( interpreter )
else()
	message( STATUS "Skip Interpreter" )
endif()

#
# Language Server
#
if( U_BUILD_LANGUAGE_SERVER )
	add_subdirectory( language_server )
else()
	message( STATUS "Skip language server" )
endif()

#
# Linkage tests
#
if( U_BUILD_COMPILER AND U_BUILD_LINKAGE_TESTS )
	message( STATUS "Build linkage tests" )

	set( CURRENT_COMPILER_GENERATION "" )
	add_subdirectory( tests )

	if( U_BUILD_COMPILER1 )

		set( CURRENT_COMPILER_GENERATION "1" )
		add_subdirectory( tests tests1 )

		if( U_BUILD_COMPILER2 )

			set( CURRENT_COMPILER_GENERATION "2" )
			add_subdirectory( tests tests2 )

			if( U_BUILD_COMPILER3 )
				set( CURRENT_COMPILER_GENERATION "3" )
				add_subdirectory( tests tests3 )
			endif()

		endif()

	endif()

else()
	message( STATUS "Skip linkage tests" )
endif()

#
# Examples
#
if( U_BUILD_COMPILER AND U_BUILD_EXAMPLES )
	message( STATUS "Build exampes" )

	set( CURRENT_COMPILER_GENERATION "" )
	add_subdirectory( examples )

	if( U_BUILD_COMPILER1 )

		set( CURRENT_COMPILER_GENERATION "1" )
		add_subdirectory( examples examples1 )

		if( U_BUILD_COMPILER2 )

			set( CURRENT_COMPILER_GENERATION "2" )
			add_subdirectory( examples examples2 )

			if( U_BUILD_COMPILER3 )

				set( CURRENT_COMPILER_GENERATION "3" )
				add_subdirectory( examples examples3 )

			endif()

		endif()

	endif()

else()
	message( STATUS "Skip exampes" )
endif()

#
# Docs
#
if( U_BUILD_DOCS )
	if( LLVM_ENABLE_SPHINX OR Sphinx_FOUND )
		message( STATUS "Include Ü docs" )
		add_subdirectory( docs )
	else()
		message( STATUS "Sphinx not found, skip Ü docs" )
	endif()
endif()
