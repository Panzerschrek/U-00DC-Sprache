set( SHARED_LIBRARY_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shared_library.u )
CompileUSources( "${SHARED_LIBRARY_SOURCES}" SHARED_LIBRARY_OBJECTS )

if(WIN32)
	if( MINGW )
		set( SHARED_LIBRARY_SOURCES ${SHARED_LIBRARY_SOURCES} shared_library_mingw.def )
	else()
		set( SHARED_LIBRARY_SOURCES ${SHARED_LIBRARY_SOURCES} shared_library_msvc.def )
	endif()
endif()

add_library( SharedLibraryTest_Library${CURRENT_COMPILER_GENERATION} SHARED ../dummy.cpp ${SHARED_LIBRARY_SOURCES} ${SHARED_LIBRARY_OBJECTS} )

set( EXECUTABLE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/executable.u )
CompileUSources( "${EXECUTABLE_SOURCES}" EXECUTABLE_OBJECTS )
add_executable( SharedLibraryTest_Executable${CURRENT_COMPILER_GENERATION} ../dummy.cpp ${EXECUTABLE_SOURCES} ${EXECUTABLE_OBJECTS} )
target_link_libraries( SharedLibraryTest_Executable${CURRENT_COMPILER_GENERATION} PRIVATE SharedLibraryTest_Library${CURRENT_COMPILER_GENERATION} )
add_custom_command( TARGET SharedLibraryTest_Executable${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND SharedLibraryTest_Executable${CURRENT_COMPILER_GENERATION} )
