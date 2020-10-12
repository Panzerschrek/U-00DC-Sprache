if( ${CMAKE_GENERATOR} STREQUAL "Ninja" )
	message( "Build DepFileTest" )

	file( RELATIVE_PATH project_subpath ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} )

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/dep_main.o
		DEPENDS Compiler
		COMMAND
			Compiler
			${CMAKE_CURRENT_SOURCE_DIR}/dep_main.u -o ${project_subpath}/dep_main.o -MF ${CMAKE_CURRENT_BINARY_DIR}/dep_main.d
			${SPRACHE_COMPILER_PIC_OPTIONS}
		DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/dep_main.d # This works only for Ninja
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # for correct dep_file work
	)

	add_executable( DepFileTest ../dummy.cpp ${CMAKE_CURRENT_BINARY_DIR}/dep_main.o )
	add_dependencies( DepFileTest Compiler )

else()
	message( "DepFileTest test disabled because it is not supported by current cmake generator" )
endif()
