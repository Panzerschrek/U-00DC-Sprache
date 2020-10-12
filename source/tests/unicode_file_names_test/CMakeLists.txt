if( U_BUILD_UNICODE_FILE_NAMES_TEST )
	set( UNICODE_FILE_NAMES_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Entry ФшЯё_66.ü" )
	CompileUSources( "${UNICODE_FILE_NAMES_SOURCES}" UNICODE_FILE_NAMES_OBJECTS )

	add_executable( UnicodeFileNamesTest ../dummy.cpp ${UNICODE_FILE_NAMES_SOURCES} ${UNICODE_FILE_NAMES_OBJECTS} )
	add_dependencies( UnicodeFileNamesTest Compiler )
endif()
