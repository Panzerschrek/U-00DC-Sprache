message( STATUS "Build compiler ${NEXT_COMPILER_GENERATION}")

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
	set( SPRACHE_COMPILER_OPT_OPTIONS -O2 -polly )
endif()

# Build rule for "Ü" files.
function( CompileUSource u_source_file out_object )
	file( RELATIVE_PATH current_subdir ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} )
	file( RELATIVE_PATH u_source_file_relative ${CMAKE_CURRENT_SOURCE_DIR} ${u_source_file} )

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
				${SPRACHE_COMPILER_OPT_OPTIONS} ${SPRACHE_COMPILER_PIC_OPTIONS}
				--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
				--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/
				--include-dir ${CMAKE_CURRENT_BINARY_DIR}/
			DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
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
				${SPRACHE_COMPILER_OPT_OPTIONS} ${SPRACHE_COMPILER_PIC_OPTIONS}
				--data-sections --function-sections # Put symbols into section in order to remove unused functions during linking.
				--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
				--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/
				--include-dir ${CMAKE_CURRENT_BINARY_DIR}/
			DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
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

set( ERRORS_LIST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../errors_list.hpp )
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

configure_file( code_builder_lib/code_builder_errors.uh.in ${CMAKE_CURRENT_BINARY_DIR}/code_builder_errors.uh )

#
# CodeBuilderLib
#

# Combine together lexical/syntaxis analyzer lib and code builder to speed up build.
# Add also some files, needed by tests

string(
	REGEX REPLACE
	${KEYWORDS_LIST_REGEXP}
	"	\\0_,"
	KEYWORDS_LIST
	${KEYWORDS_LIST_CONTENT} )

configure_file( lex_synt_lib/keywords.uh.in ${CMAKE_CURRENT_BINARY_DIR}/keywords.uh )

file( GLOB LEX_SYNT_LIB_HEADERS "lex_synt_lib/*.uh" )
file( GLOB LEX_SYNT_LIB_SOURCES "lex_synt_lib/*.u" )
set( LEX_SYNT_LIB_SOURCES_GENERATED
	${CMAKE_CURRENT_BINARY_DIR}/keywords.uh
	)
CompileUSources( LEX_SYNT_LIB_SOURCES LEX_SYNT_LIB_OBJECTS )

file( GLOB CODE_BUILDER_LIB_HEADERS "code_builder_lib/*.uh" )
file( GLOB CODE_BUILDER_LIB_SOURCES "code_builder_lib/*.u" )
file( GLOB CODE_BUILDER_LIB_CPP_SOURCES "code_builder_lib/*.cpp" )
set( CODE_BUILDER_LIB_HEADERS ${CODE_BUILDER_LIB_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/code_builder_errors.uh )
CompileUSources( CODE_BUILDER_LIB_SOURCES CODE_BUILDER_LIB_OBJECTS )

file( GLOB LAUNCHERS_COMMON_HEADERS "launchers_common/*.uh" )
file( GLOB LAUNCHERS_COMMON_SOURCES "launchers_common/*.u" )
file( GLOB LAUNCHERS_COMMON_CPP_SOURCES "launchers_common/*.cpp" )
file( GLOB LAUNCHERS_COMMON_CPP_HEADERS "launchers_common/*.hpp" )
CompileUSources( LAUNCHERS_COMMON_SOURCES LAUNCHERS_COMMON_OBJECTS )

if( COMPILER1_LTO )
	set( CODE_BUILDER_LIB_BC ${CMAKE_CURRENT_BINARY_DIR}/CodeBuilderLib.bc )
	set( CODE_BUILDER_LIB_O ${CMAKE_CURRENT_BINARY_DIR}/CodeBuilderLib.bc.o )

	add_custom_command(
		OUTPUT ${CODE_BUILDER_LIB_O}
		DEPENDS ${LEX_SYNT_LIB_OBJECTS}
		DEPENDS ${CODE_BUILDER_LIB_OBJECTS}
		DEPENDS ${LAUNCHERS_COMMON_OBJECTS}
		COMMAND
			Compiler${CURRENT_COMPILER_GENERATION}
			${LEX_SYNT_LIB_OBJECTS} ${CODE_BUILDER_LIB_OBJECTS} ${LAUNCHERS_COMMON_OBJECTS}
			${SPRACHE_COMPILER_OPT_OPTIONS} ${SPRACHE_COMPILER_PIC_OPTIONS}
			-o ${CODE_BUILDER_LIB_O}
			--input-filetype=bc
			-lto-mode=link
			--internalize
			"--internalize-preserve=\
U1_BuildProgram,\
U1_BuildProgramWithErrors,\
U1_BuildProgramWithSyntaxErrors,\
U1_BuildMultisourceProgram,\
U1_BuildMultisourceProgramWithErrors,\
U1_BuildProgramForLifetimesTest,\
U1_BuildProgramForMSVCManglingTest,\
U1_BuildProgramUsingVFS"
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Building ${CODE_BUILDER_LIB_O}"
		)

	add_library(
		CodeBuilderLib${NEXT_COMPILER_GENERATION}
			${CODE_BUILDER_LIB_O}
			${LEX_SYNT_LIB_HEADERS}
			${LEX_SYNT_LIB_SOURCES}
			${LEX_SYNT_LIB_SOURCES_GENERATED}
			${LEX_SYNT_LIB_OBJECTS}
			${CODE_BUILDER_LIB_HEADERS}
			${CODE_BUILDER_LIB_SOURCES}
			${CODE_BUILDER_LIB_CPP_SOURCES}
			${LAUNCHERS_COMMON_CPP_SOURCES}
			${LAUNCHERS_COMMON_CPP_HEADERS}
			${LAUNCHERS_COMMON_SOURCES}
			${LAUNCHERS_COMMON_HEADERS}
			${LAUNCHERS_COMMON_OBJECTS}
		)
else()
	add_library(
		CodeBuilderLib${NEXT_COMPILER_GENERATION}
			${LEX_SYNT_LIB_HEADERS}
			${LEX_SYNT_LIB_SOURCES}
			${LEX_SYNT_LIB_SOURCES_GENERATED}
			${LEX_SYNT_LIB_OBJECTS}
			${CODE_BUILDER_LIB_CPP_SOURCES}
			${CODE_BUILDER_LIB_HEADERS}
			${CODE_BUILDER_LIB_SOURCES}
			${CODE_BUILDER_LIB_OBJECTS}
			${LAUNCHERS_COMMON_CPP_SOURCES}
			${LAUNCHERS_COMMON_CPP_HEADERS}
			${LAUNCHERS_COMMON_SOURCES}
			${LAUNCHERS_COMMON_HEADERS}
			${LAUNCHERS_COMMON_OBJECTS}
		)
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
