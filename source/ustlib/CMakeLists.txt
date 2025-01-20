#
# This file contains code for building "ustlib" into a cmake library target, which is used in the build system and compiler0.
#

file( GLOB_RECURSE USTLIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.u" )

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
	DEPENDS ${USTLIB_SOURCES}
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${USTLIB_SOURCES}
		-filetype=obj
		-o ${current_subdir}/ustlib.o
		-MF ${CMAKE_CURRENT_BINARY_DIR}/ustlib.d
		--verify-module
		${SPRACHE_COMPILER_OPT_OPTIONS} ${SPRACHE_COMPILER_PIC_OPTIONS} ${SPRACHE_COMPILER_ARCH_OPTIONS}
	${DEPFILE_OPTION}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # Hack! cmake produces ninja build file with content like "compiler1/lex_syn_lib/file_pos.u.o:", but launches custom command in current binary directory
	COMMENT "Building ${current_subdir}/ustlib.o"
	)

add_library( ustlib${CURRENT_COMPILER_GENERATION} STATIC ${USTLIB_OBJECT} ${USTLIB_SOURCES} )
set_target_properties( ustlib${CURRENT_COMPILER_GENERATION} PROPERTIES LINKER_LANGUAGE C )
