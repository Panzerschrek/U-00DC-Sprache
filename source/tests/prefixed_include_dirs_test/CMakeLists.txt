add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/prefixed_include_dirs_test.o
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${CMAKE_CURRENT_SOURCE_DIR}/main.u -o ${CMAKE_CURRENT_BINARY_DIR}/prefixed_include_dirs_test.o
		${SPRACHE_COMPILER_PIC_OPTIONS}
		--verify-module
		--include-dir-prefixed ${CMAKE_CURRENT_SOURCE_DIR}/a::qwerty
		)

add_executable( PrefixedIncludeDirsTest${CURRENT_COMPILER_GENERATION} ../dummy.cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.u ${CMAKE_CURRENT_BINARY_DIR}/prefixed_include_dirs_test.o )
add_dependencies( PrefixedIncludeDirsTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )
