add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include_dirs_test.o
	DEPENDS Compiler
	COMMAND
		Compiler
		${CMAKE_CURRENT_SOURCE_DIR}/root.u -o ${CMAKE_CURRENT_BINARY_DIR}/include_dirs_test.o
		${SPRACHE_COMPILER_PIC_OPTIONS}
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/a
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/b
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/c )

add_executable( IncludeDirsTest ../dummy.cpp ${CMAKE_CURRENT_SOURCE_DIR}/root.u ${CMAKE_CURRENT_BINARY_DIR}/include_dirs_test.o )
add_dependencies( IncludeDirsTest Compiler )
