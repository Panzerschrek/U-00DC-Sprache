if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	set( SPRACHE_COMPILER_OPT_OPTIONS -O0 -g )
else()
	set( SPRACHE_COMPILER_OPT_OPTIONS -O2 )
endif()

# Build rule for "Ü" files.
function( CompileUSource u_source_file out_object )
	file( RELATIVE_PATH u_source_file_relative ${CMAKE_CURRENT_SOURCE_DIR} ${u_source_file} )
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.o
		DEPENDS ${u_source_file} ${CURRENT_COMPILER}
		COMMAND
			${CURRENT_COMPILER}
			${u_source_file}
			-o compiler1/${u_source_file_relative}.o
			-MF ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
			${SPRACHE_COMPILER_OPT_OPTIONS} ${SPRACHE_COMPILER_PIC_OPTIONS}
			--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
			--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/
			--include-dir ${CMAKE_CURRENT_BINARY_DIR}/
		DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # Hack! cmake produces ninja build file with content like "compiler1/lex_syn_lib/file_pos.u.o:", but launches custom command in current binary directory
		)
	set( ${out_object} ${u_source_file_relative}.o PARENT_SCOPE )
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
file( READ ../errors_list.hpp ERRORS_LIST_CONTENT )

set( ERRORS_LIST_REGEXP "PROCESS_ERROR\\( ([A-Za-z]+), (\\\"([^\\\"]|(\\\\\")|(\\\\\\n))+\\\") \\)" )

string(
	REGEX REPLACE
	${ERRORS_LIST_REGEXP}
	"	\\1,"
	CODE_BUILDER_ERRORS_LIST
	${ERRORS_LIST_CONTENT} )

string(
	REGEX REPLACE
	${ERRORS_LIST_REGEXP}
	"	if( e == CodeBuilderErrorCode::\\1 ) { return ust::array_view_imut</ char8 />(\\2); }"
	ERROR_CODE_TO_STRING_LIST
	${ERRORS_LIST_CONTENT} )

configure_file( code_builder_lib/code_builder_errors.uh.in ${CMAKE_CURRENT_BINARY_DIR}/code_builder_errors.uh )

#
# LexSyntLib
#

file( GLOB LEX_SYNT_LIB_HEADERS "lex_synt_lib/*.uh" )
file( GLOB LEX_SYNT_LIB_SOURCES "lex_synt_lib/*.u" )
CompileUSources( LEX_SYNT_LIB_SOURCES LEX_SYNT_LIB_OBJECTS )
add_library( LexSyntLib${CURRENT_COMPLIER_GENERATION} lib_stub.cpp ${LEX_SYNT_LIB_HEADERS} ${LEX_SYNT_LIB_SOURCES} ${LEX_SYNT_LIB_OBJECTS} )

#
# CodeBuilderLib
#

file( GLOB CODE_BUILDER_LIB_HEADERS "code_builder_lib/*.uh" )
file( GLOB CODE_BUILDER_LIB_SOURCES "code_builder_lib/*.u" )
set( CODE_BUILDER_LIB_HEADERS ${CODE_BUILDER_LIB_HEADERS} ${CMAKE_CURRENT_BINARY_DIR}/code_builder_errors.uh )
CompileUSources( CODE_BUILDER_LIB_SOURCES CODE_BUILDER_LIB_OBJECTS )
add_library( CodeBuilderLib${CURRENT_COMPLIER_GENERATION} lib_stub.cpp ${CODE_BUILDER_LIB_HEADERS} ${CODE_BUILDER_LIB_SOURCES} ${CODE_BUILDER_LIB_OBJECTS} )
target_link_libraries( CodeBuilderLib${CURRENT_COMPLIER_GENERATION} LexSyntLib${CURRENT_COMPLIER_GENERATION} ${LLVM_LIBS_FOR_CODE_BUILDER_LIB} )

#
# TestsCommon
#
if( U_BUILD_TESTS OR U_BUILD_PY_TESTS )
	file( GLOB TESTS_COMMON_HEADERS "tests_common/*.uh" )
	file( GLOB TESTS_COMMON_SOURCES "tests_common/*.u" )
	file( GLOB TESTS_COMMON_CPP_SOURCES "tests_common/*.cpp" )
	file( GLOB TESTS_COMMON_CPP_HEADERS "tests_common/*.hpp" )
	CompileUSources( TESTS_COMMON_SOURCES TESTS_COMMON_OBJECTS )

	add_library(
		TestsCommon${CURRENT_COMPLIER_GENERATION}
			${TESTS_COMMON_CPP_SOURCES}
			${TESTS_COMMON_CPP_HEADERS}
			${TESTS_COMMON_SOURCES}
			${TESTS_COMMON_HEADERS}
			${TESTS_COMMON_OBJECTS} )

	target_link_libraries(
		TestsCommon${CURRENT_COMPLIER_GENERATION}
		CodeBuilderLib${CURRENT_COMPLIER_GENERATION}
		)
endif()

#
# Tests
#

if( U_BUILD_TESTS )

	set( TESTS_SOURCES
		tests/cpp_tests_launcher.cpp
		../lex_synt_lib/file_pos.cpp # Hack! FilePos methods like "GetLine" and "constructor" used in tests.
		)

	add_executable(
		Tests${CURRENT_COMPLIER_GENERATION}
			${TESTS_SOURCES} )

	target_link_libraries(
		Tests${CURRENT_COMPLIER_GENERATION}
		PRIVATE
			TestsCommon${CURRENT_COMPLIER_GENERATION}
			${TESTS_LIB_WRAPPED}
		)

	if( MSVC )
		set_target_properties( Tests${CURRENT_COMPLIER_GENERATION} PROPERTIES LINK_FLAGS /WHOLEARCHIVE:TestsLib )
	endif()

	add_custom_command( TARGET Tests${CURRENT_COMPLIER_GENERATION} POST_BUILD COMMAND Tests${CURRENT_COMPLIER_GENERATION} )

endif()

if( U_BUILD_PY_TESTS )
	set( PY_TESTS_LIB_SOURCES
		py_tests_lib/py_tests_lib.cpp
		../tests/tests_common.hpp )

	add_library( PyTestsLib${CURRENT_COMPLIER_GENERATION} SHARED ${PY_TESTS_LIB_SOURCES} )
	target_link_libraries(
		PyTestsLib${CURRENT_COMPLIER_GENERATION}
			PRIVATE
				TestsCommon${CURRENT_COMPLIER_GENERATION}
				${LLVM_LIBS_FOR_TESTS}
				${PYTHON_LIBRARIES}
			)

	target_include_directories( PyTestsLib${CURRENT_COMPLIER_GENERATION} PRIVATE ${PYTHON_INCLUDE_DIRS} )

	# Set proper name for result dynamic library.
	if( WIN32 )
		set_target_properties( PyTestsLib${CURRENT_COMPLIER_GENERATION}
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.pyd PREFIX "" SUFFIX "" )
	else()
		set_target_properties( PyTestsLib${CURRENT_COMPLIER_GENERATION}
			PROPERTIES OUTPUT_NAME sprache_compiler_tests_py_lib.so PREFIX "" SUFFIX "" )
	endif()

	file( GLOB PY_TESTS_SOURCES "../tests/py_tests/*.py" ) # Add all python files to sources list of target.

	# Run result test.
	add_custom_target(
		PyTests1Run ALL
		COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR} ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tests/py_tests/py_tests.py
		DEPENDS PyTestsLib${CURRENT_COMPLIER_GENERATION}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		SOURCES ${PY_TESTS_SOURCES} )

endif()
