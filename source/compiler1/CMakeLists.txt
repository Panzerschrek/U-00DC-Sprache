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
# Tests
#

if( U_BUILD_TESTS )

	file( GLOB TESTS_HEADERS "tests/*.uh" )
	file( GLOB TESTS_SOURCES "tests/*.u" )
	file( GLOB TESTS_CPP_SOURCES "tests/*.cpp" )
	CompileUSources( TESTS_SOURCES TESTS_OBJECTS )
	add_executable( Tests${CURRENT_COMPLIER_GENERATION} ${TESTS_CPP_SOURCES} ${TESTS_HEADERS} ${TESTS_SOURCES} ${TESTS_OBJECTS} )
	target_link_libraries(
		Tests${CURRENT_COMPLIER_GENERATION}
		PRIVATE
			CodeBuilderLib${CURRENT_COMPLIER_GENERATION}
			${TESTS_LIB_WRAPPED}
			LexSyntLib # Hack! Use C++ lib for "FilePos" class.
		)

endif()
