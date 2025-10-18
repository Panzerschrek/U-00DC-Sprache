if(WIN32)
	# For now Windows isn't supported.
	return()
endif()

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	set( BUILD_SYSTEM_BUILD_CONFIGURATION "debug" )
else()
	set( BUILD_SYSTEM_BUILD_CONFIGURATION "release" )
endif()

if( MINGW )
	# We need to provide path to MinGW libraries for linking.
	# Detect MinGW installation using g++ path.
	get_filename_component( MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY )
	set( MINGW_INSTALLATION_PATH ${MINGW_BIN_DIR}/../ )
	set( SYSROOT_OPTION --sysroot \"${MINGW_INSTALLATION_PATH}\" --host-sysroot \"${MINGW_INSTALLATION_PATH}\" )
elseif( NOT ${CMAKE_OSX_SYSROOT} STREQUAL "" )
	# Building for OS X may also require sysroot specifying.
	set( SYSROOT_OPTION --sysroot \"${CMAKE_OSX_SYSROOT}\" --host-sysroot \"${CMAKE_OSX_SYSROOT}\" )
else()
	set( SYSROOT_OPTION "" )
endif()

file( GLOB_RECURSE ALL_LIB_SOURCES "*" )
file( GLOB LIB_CMAKE_FILE "CMakeLists.txt" )
list( REMOVE_ITEM ALL_LIB_SOURCES ${LIB_CMAKE_FILE} )

set( TESTS_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/${BUILD_SYSTEM_BUILD_CONFIGURATION}/tests )

add_custom_command(
	OUTPUT ${TESTS_EXECUTABLE}
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
	DEPENDS BuildSystem
	DEPENDS ${ALL_LIB_SOURCES}
	COMMAND
		BuildSystem build
		--build-configuration ${BUILD_SYSTEM_BUILD_CONFIGURATION}
		--compiler-executable $<TARGET_FILE:Compiler${CURRENT_COMPILER_GENERATION}>
		--build-system-imports-path ${CMAKE_CURRENT_SOURCE_DIR}/../../build_system/imports/
		--ustlib-path ${CMAKE_CURRENT_SOURCE_DIR}/../../ustlib/
		--project-directory ${CMAKE_CURRENT_SOURCE_DIR}
		--build-directory ${CMAKE_CURRENT_BINARY_DIR}
		${SYSROOT_OPTION}
	USES_TERMINAL
		)

if(NOT APPLE) # For now disable running tests on MacOS - they hang for unknown reason and it's hard to find why exactly.

	set( TIMESTAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/tests_timestamp )

	add_custom_command(
		OUTPUT ${TIMESTAMP_FILE}
		DEPENDS ${TESTS_EXECUTABLE}
		COMMAND ${TESTS_EXECUTABLE}
		COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE} # A hacky step to produce a marker file for this command.
		)

	# A custom target which is needed only to execute the custom command above.
	add_custom_target(
		sm_async_net_run${CURRENT_COMPILER_GENERATION} ALL
		DEPENDS ${TIMESTAMP_FILE}
		SOURCES ${ALL_LIB_SOURCES}
		)

endif()

# Install the library sources.
if( CURRENT_COMPILER_GENERATION STREQUAL "" )
	install( CODE " file( INSTALL ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION \${CMAKE_INSTALL_PREFIX}/extra_libs/ ) " )
endif()
