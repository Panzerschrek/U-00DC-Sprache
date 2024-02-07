file( GLOB UNREACHABLE_HALT_SOURCES "*.u" )

# Compile always with -O2 to optimize-out halt checks.
foreach( SOURCE ${UNREACHABLE_HALT_SOURCES} )
	file( RELATIVE_PATH source_name_rel ${CMAKE_CURRENT_SOURCE_DIR} ${SOURCE} )
	set( OBJECT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${source_name_rel}.o )
	add_custom_command(
		OUTPUT ${OBJECT_FILE}
		DEPENDS ${SOURCE} Compiler${CURRENT_COMPILER_GENERATION}
		COMMAND
			Compiler${CURRENT_COMPILER_GENERATION}  ${SPRACHE_COMPILER_PIC_OPTIONS} -O2 --halt-mode=unreachable
			${SOURCE} -o ${OBJECT_FILE}
		)
	list( APPEND UNREACHABLE_HALT_OBJECTS ${OBJECT_FILE} )
endforeach()

add_executable( UnreachableHaltTest${CURRENT_COMPILER_GENERATION} ${UNREACHABLE_HALT_SOURCES} ${UNREACHABLE_HALT_OBJECTS} )
target_link_libraries( UnreachableHaltTest${CURRENT_COMPILER_GENERATION} PRIVATE CPPDummy )
add_dependencies( UnreachableHaltTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

# Run unreachable halt test. Should run fine.
add_custom_command( TARGET UnreachableHaltTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND UnreachableHaltTest${CURRENT_COMPILER_GENERATION} )
