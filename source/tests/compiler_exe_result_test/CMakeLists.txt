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
	set( SYSROOT_OPTION "--sysroot \"${MINGW_INSTALLATION_PATH}\"" )
else()
	set( SYSROOT_OPTION "" )
endif()

set( COMPILER_EXE_RESULT_TEST_FILE ${CMAKE_CURRENT_SOURCE_DIR}/compiler_exe_result_test.u )
set( COMPILER_EXE_RESULT_TEST_FILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/compiler_exe_result_test${CURRENT_COMPILER_GENERATION}.exe )
add_custom_command(
	OUTPUT ${COMPILER_EXE_RESULT_TEST_FILE_OUT}
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION} ${COMPILER_EXE_RESULT_TEST_FILE}
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${COMPILER_EXE_RESULT_TEST_FILE}
		-o ${COMPILER_EXE_RESULT_TEST_FILE_OUT}
		-filetype=exe
		${SPRACHE_COMPILER_PIC_OPTIONS}
		--verify-module
		${SYSROOT_OPTION}
	)

add_custom_target(
	CompilerExeResultTest${CURRENT_COMPILER_GENERATION} ALL
	DEPENDS ${COMPILER_EXE_RESULT_TEST_FILE_OUT}
	SOURCES ${COMPILER_EXE_RESULT_TEST_FILE}
	)
