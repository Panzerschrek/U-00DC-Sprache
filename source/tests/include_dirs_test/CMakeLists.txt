add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include_dirs_test.o
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${CMAKE_CURRENT_SOURCE_DIR}/root.u -o ${CMAKE_CURRENT_BINARY_DIR}/include_dirs_test.o
		${SPRACHE_COMPILER_PIC_OPTIONS}
		--verify-module
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/a
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/b
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/c )

add_executable( IncludeDirsTest${CURRENT_COMPILER_GENERATION} ${CMAKE_CURRENT_SOURCE_DIR}/root.u ${CMAKE_CURRENT_BINARY_DIR}/include_dirs_test.o )
target_link_libraries( IncludeDirsTest${CURRENT_COMPILER_GENERATION} PRIVATE CPPDummy )
add_dependencies( IncludeDirsTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )
