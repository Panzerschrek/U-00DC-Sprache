#
# This file contains code for building "ustlib" into a cmake library target, which is used in the build system and compiler0.
#

file( GLOB_RECURSE USTLIB_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/imports/*.u" )
file( GLOB_RECURSE USTLIB_TESTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.u" )
file( GLOB USTLIB_SOURCES_COMMON "${CMAKE_CURRENT_SOURCE_DIR}/src/*.u" )
file( GLOB USTLIB_INTERNAL_HEADERS_COMMON "${CMAKE_CURRENT_SOURCE_DIR}/src/*.uh" )
file( GLOB USTLIB_SOURCES_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/*.u" )
file( GLOB USTLIB_INTERNAL_HEADERS_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/src/windows/*.uh" )
file( GLOB USTLIB_SOURCES_UNIX "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/*.u" )
file( GLOB USTLIB_INTERNAL_HEADERS_UNIX "${CMAKE_CURRENT_SOURCE_DIR}/src/unix/*.uh" )

if( WIN32 )
	set( USTLIB_SOURCES ${USTLIB_SOURCES_COMMON} ${USTLIB_SOURCES_WINDOWS} )
else()
	set( USTLIB_SOURCES ${USTLIB_SOURCES_COMMON} ${USTLIB_SOURCES_UNIX} )
endif()

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	set( SPRACHE_COMPILER_OPT_OPTIONS -O0 -g )
else()
	set( SPRACHE_COMPILER_OPT_OPTIONS -O2 )
endif()

file( RELATIVE_PATH current_subdir ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

set( USTLIB_OBJECT ${CMAKE_CURRENT_BINARY_DIR}/ustlib.o )

if( ${CMAKE_GENERATOR} STREQUAL "Ninja" )
	set( DEPFILE_OPTION DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/ustlib.d )
else()
	set( DEPFILE_OPTION "" )
endif()

add_custom_command(
	OUTPUT ${USTLIB_OBJECT}
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
	DEPENDS ${USTLIB_SOURCES} ${USTLIB_HEADERS}
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${USTLIB_SOURCES}
		-filetype=obj
		-o ${current_subdir}/ustlib.o
		-MF ${CMAKE_CURRENT_BINARY_DIR}/ustlib.d
		--data-sections --function-sections # Put symbols into sections in order to remove unused functions during linking.
		${SPRACHE_COMPILER_OPT_OPTIONS} ${SPRACHE_COMPILER_PIC_OPTIONS} ${SPRACHE_COMPILER_ARCH_OPTIONS}
		--verify-module
	${DEPFILE_OPTION}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # Hack! cmake produces ninja build file with content like "compiler1/lex_syn_lib/file_pos.u.o:", but launches custom command in current binary directory
	COMMENT "Building ${current_subdir}/ustlib.o"
	)

add_library(
	ustlib${CURRENT_COMPILER_GENERATION} STATIC
	${USTLIB_OBJECT}
	# List almost all files - sources (for all platforms), headers, tests.
	${USTLIB_SOURCES_COMMON}
	${USTLIB_INTERNAL_HEADERS_COMMON}
	${USTLIB_SOURCES_WINDOWS}
	${USTLIB_INTERNAL_HEADERS_WINDOWS}
	${USTLIB_SOURCES_UNIX}
	${USTLIB_INTERNAL_HEADERS_UNIX}
	${USTLIB_HEADERS}
	${USTLIB_TESTS}
	)

set_target_properties( ustlib${CURRENT_COMPILER_GENERATION} PROPERTIES LINKER_LANGUAGE C )

if( NOT WIN32 )
	target_link_libraries( ustlib${CURRENT_COMPILER_GENERATION} PRIVATE pthread )
endif()

if( CURRENT_COMPILER_GENERATION STREQUAL "" )
	install( FILES ${USTLIB_HEADERS} DESTINATION "ustlib/imports" )
	install( FILES ${USTLIB_SOURCES_COMMON} DESTINATION "ustlib/src" )
	install( FILES ${USTLIB_INTERNAL_HEADERS_COMMON} DESTINATION "ustlib/src" )
	install( FILES ${USTLIB_SOURCES_WINDOWS} DESTINATION "ustlib/src/windows" )
	install( FILES ${USTLIB_INTERNAL_HEADERS_WINDOWS} DESTINATION "ustlib/src/windows" )
	install( FILES ${USTLIB_SOURCES_UNIX} DESTINATION "ustlib/src/unix" )
	install( FILES ${USTLIB_INTERNAL_HEADERS_UNIX} DESTINATION "ustlib/src/unix" )
endif()
