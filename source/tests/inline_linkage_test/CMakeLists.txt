# Compile multimodule program on "Ü-Sprache". Each module includes one common file.
set( INLINE_LINKAGE_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/a.u
	${CMAKE_CURRENT_SOURCE_DIR}/b.u
	${CMAKE_CURRENT_SOURCE_DIR}/c.u
	${CMAKE_CURRENT_SOURCE_DIR}/entry.u )
CompileUSources( "${INLINE_LINKAGE_SOURCES}" INLINE_LINKAGE_OBJECTS )

add_executable( InlineLinkageTest ../dummy.cpp ${INLINE_LINKAGE_SOURCES} ${INLINE_LINKAGE_OBJECTS} )
add_dependencies( InlineLinkageTest Compiler )
