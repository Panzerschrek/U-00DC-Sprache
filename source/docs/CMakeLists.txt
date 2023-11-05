file( GLOB LANG_DIRS "*" )
foreach( LANG_DIR ${LANG_DIRS} )
	if( IS_DIRECTORY ${LANG_DIR} )
		get_filename_component( LANG_NAME ${LANG_DIR} NAME )
		set( DOC_TARGET_NAME SpracheDocs_${LANG_NAME} )
		file( GLOB_RECURSE DOC_FILES "${LANG_DIR}/*" )
		add_custom_target(
			${DOC_TARGET_NAME}
			ALL
			DEPENDS ${DOC_FILES}
			SOURCES ${DOC_FILES}
			COMMENT "Generating ${DOC_TARGET_NAME}"
			COMMAND ${SPHINX_EXECUTABLE}
			-b html
			-q # Be quiet
			${LANG_DIR} #src
			${CMAKE_BINARY_DIR}/docs/${LANG_NAME} #dst
			)

		# Install all generated files from target docs directory (html files, styles, etc.).
		install( CODE " file( INSTALL ${CMAKE_BINARY_DIR}/docs/${LANG_NAME} DESTINATION \${CMAKE_INSTALL_PREFIX}/docs ) " )

	endif()
endforeach()
