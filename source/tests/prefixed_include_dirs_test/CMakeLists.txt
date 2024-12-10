add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/prefixed_include_dirs_test.o
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/main.u
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${CMAKE_CURRENT_SOURCE_DIR}/main.u -o ${CMAKE_CURRENT_BINARY_DIR}/prefixed_include_dirs_test.o
		${SPRACHE_COMPILER_PIC_OPTIONS}
		--verify-module
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/a::qwerty
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/b::qwe
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/c::/ach/wie/gut/dass/niemand/weiss/wer/ich/bin/und/wie/ich/heiss
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/burn/the/bridges/to/the/hilt::d
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/e::
		)

add_executable( PrefixedIncludeDirsTest${CURRENT_COMPILER_GENERATION} ../dummy.cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.u ${CMAKE_CURRENT_BINARY_DIR}/prefixed_include_dirs_test.o )
add_dependencies( PrefixedIncludeDirsTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )
