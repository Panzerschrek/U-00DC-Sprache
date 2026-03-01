if( NOT TARGET llvm-rc )
	return()
endif()

# HACK! A compiled resource file isn't really object file, but name it "obj" to make CMake happy.
# Otherwise CMake won't link it against the result executable.
set( RES_FILE_COMPILED ${CMAKE_CURRENT_BINARY_DIR}/res.rc.obj )

add_custom_command(
	OUTPUT ${RES_FILE_COMPILED}
	COMMAND llvm-rc ${CMAKE_CURRENT_SOURCE_DIR}/res.rc /fo ${RES_FILE_COMPILED}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res.rc ${CMAKE_CURRENT_SOURCE_DIR}/icon.ico
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

add_executable( WindowsResourceCompilerTest${CURRENT_COMPILER_GENERATION} main.cpp ${RES_FILE_COMPILED} )
