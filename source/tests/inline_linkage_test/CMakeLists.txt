# Compile multimodule program on "Ü-Sprache". Each module includes one common file.
set( INLINE_LINKAGE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/a.u
	${CMAKE_CURRENT_SOURCE_DIR}/b.u
	${CMAKE_CURRENT_SOURCE_DIR}/c.u
	${CMAKE_CURRENT_SOURCE_DIR}/entry.u )
CompileUSources( "${INLINE_LINKAGE_SOURCES}" INLINE_LINKAGE_OBJECTS )

add_executable( InlineLinkageTest${CURRENT_COMPILER_GENERATION} ${INLINE_LINKAGE_SOURCES} ${INLINE_LINKAGE_OBJECTS} )
target_link_libraries( InlineLinkageTest${CURRENT_COMPILER_GENERATION} PRIVATE CPPDummy )
add_dependencies( InlineLinkageTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

# Run inline linkage test
add_custom_command( TARGET InlineLinkageTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND InlineLinkageTest${CURRENT_COMPILER_GENERATION} )
