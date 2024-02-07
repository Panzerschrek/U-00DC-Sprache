if( ${CMAKE_GENERATOR} STREQUAL "Ninja" )
	message( STATUS "Build DepFileTest" )

	file( RELATIVE_PATH project_subpath ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dep_main.o
		DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
		COMMAND
			Compiler${CURRENT_COMPILER_GENERATION}
			${CMAKE_CURRENT_SOURCE_DIR}/dep_main.u -o ${project_subpath}/dep_main.o -MF ${CMAKE_CURRENT_BINARY_DIR}/dep_main.d
			${SPRACHE_COMPILER_PIC_OPTIONS}
			--verify-module
		DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/dep_main.d # This works only for Ninja
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # for correct dep_file work
	)

	add_executable(
		DepFileTest${CURRENT_COMPILER_GENERATION}
			${CMAKE_CURRENT_BINARY_DIR}/dep_main.o
			${CMAKE_CURRENT_SOURCE_DIR}/dep_main.u
		)
	target_link_libraries( DepFileTest${CURRENT_COMPILER_GENERATION} PRIVATE CPPDummy )
	add_dependencies( DepFileTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

else()
	message( STATUS "DepFileTest test disabled because it is not supported by current cmake generator" )
endif()
