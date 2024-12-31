message( STATUS "Build compiler ${NEXT_COMPILER_GENERATION}")

file( GLOB_RECURSE CODE_BUILDER_LIB_CPP_SOURCES "src/*.cpp" )
file( GLOB_RECURSE CODE_BUILDER_LIB_HEADERS "src/*.uh" "imports/*.uh" )
file( GLOB_RECURSE CODE_BUILDER_LIB_SOURCES "src/*.u" )

if( TARGET BuildSystem AND TARGET lldCommon AND ${NEXT_COMPILER_GENERATION} GREATER_EQUAL 2 )

	message( STATUS "Build Compiler${NEXT_COMPILER_GENERATION} using build system." )

	set( BK_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/bk_build )

	if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
		set( BUILD_SYSTEM_BUILD_CONFIGURATION "debug" )
	else()
		set( BUILD_SYSTEM_BUILD_CONFIGURATION "release" )
	endif()

	set( CODE_BUILDER_LIB_OBJECT_PATH ${BK_BUILD_DIR}/${BUILD_SYSTEM_BUILD_CONFIGURATION}/CodeBuilderLib.o )
	set( BUILD_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/build.u )

	add_custom_command(
		OUTPUT ${CODE_BUILDER_LIB_OBJECT_PATH}
		DEPENDS ${CODE_BUILDER_LIB_HEADERS} ${CODE_BUILDER_LIB_SOURCES}
		DEPENDS ${BUILD_SCRIPT_FILE}
		DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
		DEPENDS BuildSystem
		COMMAND
			BuildSystem build
			--build-configuration ${BUILD_SYSTEM_BUILD_CONFIGURATION}
			--compiler-executable $<TARGET_FILE:Compiler${CURRENT_COMPILER_GENERATION}>
			--build-system-imports-path ${CMAKE_CURRENT_SOURCE_DIR}/../build_system/imports/
			--ustlib-path ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
			--project-directory ${CMAKE_CURRENT_SOURCE_DIR}
			--build-directory ${BK_BUILD_DIR}
		USES_TERMINAL
			)

	add_library(
		CodeBuilderLib${NEXT_COMPILER_GENERATION}
			${CODE_BUILDER_LIB_OBJECT_PATH}
			${BUILD_SCRIPT_FILE}
			${CODE_BUILDER_LIB_CPP_SOURCES}
			${CODE_BUILDER_LIB_HEADERS}
			${CODE_BUILDER_LIB_SOURCES}
		)

else()

	# Build Compiler1 without LTO. Build later generations with LTO, but only in release mode.
	if( ${NEXT_COMPILER_GENERATION} STREQUAL "1" OR ${CMAKE_BUILD_TYPE} STREQUAL Debug )
		set( COMPILER1_LTO NO )
		message( STATUS "build Compiler${NEXT_COMPILER_GENERATION} with no LTO" )
	else()
		set( COMPILER1_LTO YES )
		message( STATUS "build Compiler${NEXT_COMPILER_GENERATION} with LTO" )
	endif()

	if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
		set( SPRACHE_COMPILER_OPT_OPTIONS -O0 -g )
	else()
		set( SPRACHE_COMPILER_OPT_OPTIONS -O2 )
	endif()

	# Build rule for "Ü" files.
	function( CompileUSource u_source_file out_object )
		file( RELATIVE_PATH current_subdir ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} )
		file( RELATIVE_PATH u_source_file_relative ${CMAKE_CURRENT_SOURCE_DIR} ${u_source_file} )

		if( ${CMAKE_GENERATOR} STREQUAL "Ninja" )
			set( DEPFILE_OPTION DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d )
		else()
			set( DEPFILE_OPTION "" )
		endif()

		if( COMPILER1_LTO )
			add_custom_command(
				OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.bc
				DEPENDS ${u_source_file} Compiler${CURRENT_COMPILER_GENERATION}
				COMMAND
					Compiler${CURRENT_COMPILER_GENERATION}
					${u_source_file}
					-filetype=bc
					-lto-mode=prelink
					-o ${current_subdir}/${u_source_file_relative}.bc
					-MF ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
					--verify-module
					${SPRACHE_COMPILER_OPT_OPTIONS} ${SPRACHE_COMPILER_PIC_OPTIONS} ${SPRACHE_COMPILER_ARCH_OPTIONS}
					--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
					--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/src/
					--include-dir ${CMAKE_CURRENT_BINARY_DIR}/::CodeBuilderLib
					# Provide source directories in order to mark symbols declared in headers within these directories as hidden.
					# DO NOT list here the directory "imports", since it contains declarations of publically-available symbols.
					--source-dir ${CMAKE_CURRENT_SOURCE_DIR}/src
					--source-dir ${CMAKE_CURRENT_BINARY_DIR}/
				${DEPFILE_OPTION}
				WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # Hack! cmake produces ninja build file with content like "compiler1/lex_syn_lib/file_pos.u.bc:", but launches custom command in current binary directory
				COMMENT "Building ${current_subdir}/${u_source_file_relative}.bc"
				)
			set( ${out_object} ${u_source_file_relative}.bc PARENT_SCOPE )
		else()
			add_custom_command(
				OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.o
				DEPENDS ${u_source_file} Compiler${CURRENT_COMPILER_GENERATION}
				COMMAND
					Compiler${CURRENT_COMPILER_GENERATION}
					${u_source_file}
					-o ${current_subdir}/${u_source_file_relative}.o
					-MF ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
					--verify-module
					${SPRACHE_COMPILER_OPT_OPTIONS} ${SPRACHE_COMPILER_PIC_OPTIONS} ${SPRACHE_COMPILER_ARCH_OPTIONS}
					--data-sections --function-sections # Put symbols into section in order to remove unused functions during linking.
					--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
					--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/src/
					--include-dir ${CMAKE_CURRENT_BINARY_DIR}/::CodeBuilderLib
					# Provide source directories in order to mark symbols declared in headers within these directories as hidden.
					# DO NOT list here the directory "imports", since it contains declarations of publically-available symbols.
					--source-dir ${CMAKE_CURRENT_SOURCE_DIR}/src/
					--source-dir ${CMAKE_CURRENT_BINARY_DIR}/
				${DEPFILE_OPTION}
				WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # Hack! cmake produces ninja build file with content like "compiler1/lex_syn_lib/file_pos.u.o:", but launches custom command in current binary directory
				COMMENT "Building ${current_subdir}/${u_source_file_relative}.o"
				)
			set( ${out_object} ${u_source_file_relative}.o PARENT_SCOPE )
		endif()
	endfunction()

	# Returns list of objects in ${out_objects}
	function( CompileUSources u_source_files out_objects )
		foreach( source ${${u_source_files}} )
			CompileUSource( ${source} out_object )
			list( APPEND out_objects_local ${out_object} )
		endforeach()
		set( ${out_objects} ${out_objects_local} PARENT_SCOPE )
	endfunction()

	#
	# CodeBuilderErrors
	#

	set( ERRORS_LIST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../compiler1/errors_list.hpp )
	set_property( DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${ERRORS_LIST_FILE} )

	file( READ ${ERRORS_LIST_FILE} ERRORS_LIST_CONTENT )

	set( ERRORS_LIST_REGEXP "PROCESS_ERROR\\( ([A-Za-z][A-Za-z0-9]*), (\\\"([^\\\"]|(\\\\\")|(\\\\\\n))+\\\") \\)" )

	string(
		REGEX REPLACE
		${ERRORS_LIST_REGEXP}
		"	\\1,"
		CODE_BUILDER_ERRORS_LIST
		${ERRORS_LIST_CONTENT} )

	string(
		REGEX REPLACE
		${ERRORS_LIST_REGEXP}
		"	CodeBuilderErrorCode::\\1 -> { return \\2; },"
		ERROR_CODE_TO_STRING_LIST
		${ERRORS_LIST_CONTENT} )

	configure_file( src/code_builder_lib/code_builder_error_code.uh.in ${CMAKE_CURRENT_BINARY_DIR}/code_builder_error_code.uh )

	#
	# CodeBuilderLib
	#

	string(
		REGEX REPLACE
		${KEYWORDS_LIST_REGEXP}
		"	\\0_,"
		KEYWORDS_LIST
		${KEYWORDS_LIST_CONTENT} )

	configure_file( src/lex_synt_lib/keywords.uh.in ${CMAKE_CURRENT_BINARY_DIR}/keywords.uh )

	set( CODE_BUILDER_LIB_HEADERS_GENERATED
		${CMAKE_CURRENT_BINARY_DIR}/keywords.uh
		${CMAKE_CURRENT_BINARY_DIR}/code_builder_error_code.uh
		)

	CompileUSources( CODE_BUILDER_LIB_SOURCES CODE_BUILDER_LIB_OBJECTS )

	if( COMPILER1_LTO )
		set( CODE_BUILDER_LIB_BC ${CMAKE_CURRENT_BINARY_DIR}/CodeBuilderLib.bc )
		set( CODE_BUILDER_LIB_O ${CMAKE_CURRENT_BINARY_DIR}/CodeBuilderLib.bc.o )

		add_custom_command(
			OUTPUT ${CODE_BUILDER_LIB_O}
			DEPENDS ${CODE_BUILDER_LIB_OBJECTS}
			COMMAND
				Compiler${CURRENT_COMPILER_GENERATION}
				${CODE_BUILDER_LIB_OBJECTS}
				${SPRACHE_COMPILER_OPT_OPTIONS} ${SPRACHE_COMPILER_PIC_OPTIONS} ${SPRACHE_COMPILER_ARCH_OPTIONS}
				-o ${CODE_BUILDER_LIB_O}
				--input-filetype=bc
				-lto-mode=link
				--internalize-hidden # Internalize all except publically-available symbols.
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
			COMMENT "Building ${CODE_BUILDER_LIB_O}"
			)

		add_library(
			CodeBuilderLib${NEXT_COMPILER_GENERATION}
				${CODE_BUILDER_LIB_O}
				${CODE_BUILDER_LIB_CPP_SOURCES}
				${CODE_BUILDER_LIB_HEADERS_GENERATED}
				${CODE_BUILDER_LIB_HEADERS}
				${CODE_BUILDER_LIB_SOURCES}
			)
	else()
		add_library(
			CodeBuilderLib${NEXT_COMPILER_GENERATION}
				${CODE_BUILDER_LIB_OBJECTS}
				${CODE_BUILDER_LIB_CPP_SOURCES}
				${CODE_BUILDER_LIB_HEADERS_GENERATED}
				${CODE_BUILDER_LIB_HEADERS}
				${CODE_BUILDER_LIB_SOURCES}
			)
	endif()

endif()

target_link_libraries( CodeBuilderLib${NEXT_COMPILER_GENERATION} LexSyntLibCommon CodeBuilderLibCommon )

#
# Compiler
#
if( U_BUILD_COMPILER )
	file( GLOB COMPILER_SOURCES "compiler/*" )

	add_executable( Compiler${NEXT_COMPILER_GENERATION} ${COMPILER_SOURCES} )
	target_link_libraries( Compiler${NEXT_COMPILER_GENERATION} CompilersCommonLib SpracheVersion CodeBuilderLib${NEXT_COMPILER_GENERATION} )

	target_compile_definitions( Compiler${NEXT_COMPILER_GENERATION} PRIVATE "-DU_COMPILER_GENERATION=${NEXT_COMPILER_GENERATION}" )

	# Install Compiler1, but not other generations (2, 3, etc).
	if( ${NEXT_COMPILER_GENERATION} STREQUAL "1" )
		install( TARGETS Compiler${NEXT_COMPILER_GENERATION} )
	endif()

endif()

#
# Tests
#
if( U_BUILD_TESTS )

	add_executable( Tests${NEXT_COMPILER_GENERATION} tests/cpp_tests_launcher.cpp )

	target_link_libraries(
		Tests${NEXT_COMPILER_GENERATION}
		PRIVATE
			CodeBuilderLib${NEXT_COMPILER_GENERATION}
			${CPP_TESTS_LIB_WRAPPED}
		)

	if( MSVC )
		set_target_properties( Tests${NEXT_COMPILER_GENERATION} PROPERTIES LINK_FLAGS /WHOLEARCHIVE:CppTestsLib )
	endif()

	add_custom_command( TARGET Tests${NEXT_COMPILER_GENERATION} POST_BUILD COMMAND Tests${NEXT_COMPILER_GENERATION} )

endif()

#
# PyTests
#
if( U_BUILD_PY_TESTS )
	add_library( PyTestsLib${NEXT_COMPILER_GENERATION} SHARED py_tests_lib/py_tests_lib.cpp )
	target_link_libraries(
		PyTestsLib${NEXT_COMPILER_GENERATION}
			PRIVATE
				CodeBuilderLib${NEXT_COMPILER_GENERATION}
				CodeBuilderLibCommon
				${PYTHON_LIBRARIES}
			)

	target_include_directories( PyTestsLib${NEXT_COMPILER_GENERATION} PRIVATE ${PYTHON_INCLUDE_DIRS} )

	# Set proper name for result dynamic library.
	if( WIN32 )
		set_target_properties( PyTestsLib${NEXT_COMPILER_GENERATION}
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.pyd PREFIX "" SUFFIX "" )
	else()
		set_target_properties( PyTestsLib${NEXT_COMPILER_GENERATION}
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.so PREFIX "" SUFFIX "" )
	endif()

	file( GLOB PY_TESTS_SOURCES "../tests/py_tests/*.py" ) # Add all python files to sources list of target.

	# Run result test.
	add_custom_target(
		PyTestsRun${NEXT_COMPILER_GENERATION} ALL
		COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tests/py_tests/py_tests.py
		DEPENDS PyTestsLib${NEXT_COMPILER_GENERATION}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		SOURCES ${PY_TESTS_SOURCES} )

endif()
