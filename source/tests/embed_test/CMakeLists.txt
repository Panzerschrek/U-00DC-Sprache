if( NOT ${CMAKE_GENERATOR} STREQUAL "Ninja" )
	return()
endif()

file( RELATIVE_PATH project_subpath ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/main.o
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/main.u
	COMMAND
		Compiler${CURRENT_COMPILER_GENERATION}
		${CMAKE_CURRENT_SOURCE_DIR}/main.u
		-o ${project_subpath}/main.o
		-MF ${CMAKE_CURRENT_BINARY_DIR}/main.d
		${SPRACHE_COMPILER_PIC_OPTIONS}
		--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/inc_absolute
		--verify-module
	DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/main.d # This works only for Ninja
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # for correct dep_file work
	)

add_executable( EmbedTest${CURRENT_COMPILER_GENERATION} ../dummy.cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.u ${CMAKE_CURRENT_BINARY_DIR}/main.o )
add_dependencies( EmbedTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )
