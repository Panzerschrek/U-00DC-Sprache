# Produce an executable file directly with the Ü compiler.
# This requires LLD libraries to be linked.

if( NOT TARGET lldCommon )
	return()
endif()

if( MINGW )
	# We need to provide path to MinGW libraries for linking.
	# Detect MinGW installation using g++ path.
	get_filename_component( MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY )
	set( MINGW_INSTALLATION_PATH ${MINGW_BIN_DIR}/../ )
	set( SYSROOT_OPTION --sysroot \"${MINGW_INSTALLATION_PATH}\" )
elseif( NOT ${CMAKE_OSX_SYSROOT} STREQUAL "" )
	# Building for OS X may also require sysroot specifying.
	set( SYSROOT_OPTION --sysroot \"${CMAKE_OSX_SYSROOT}\" )
else()
	set( SYSROOT_OPTION "" )
endif()

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	set( SPRACHE_COMPILER_OPT_OPTIONS -O0 -g )
else()
	set( SPRACHE_COMPILER_OPT_OPTIONS -O2 )
endif()

set(
	SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/main.u
	${CMAKE_CURRENT_SOURCE_DIR}/a.u
	${CMAKE_CURRENT_SOURCE_DIR}/b.u
	)

set( IMPORTS ${CMAKE_CURRENT_SOURCE_DIR}/inc.iu )

set( OUT_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/unin_tests_table_test${CURRENT_COMPILER_GENERATION}.exe )

add_custom_command(
	OUTPUT ${OUT_EXECUTABLE}
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION} ${SOURCES} ${IMPORTS}
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${SOURCES}
		-o ${OUT_EXECUTABLE}
		-filetype=exe
		${SPRACHE_COMPILER_PIC_OPTIONS}
		${SPRACHE_COMPILER_OPT_OPTIONS}
		--verify-module
		--internalize
		--build-tests-table=my_pretty_tests_table
		${SYSROOT_OPTION}
	)

add_custom_target(
	UnitTestsTableTest${CURRENT_COMPILER_GENERATION} ALL
	DEPENDS ${OUT_EXECUTABLE}
	SOURCES ${SOURCES} ${IMPORTS}
	)

# Run the test
add_custom_command( TARGET UnitTestsTableTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND ${OUT_EXECUTABLE} )
