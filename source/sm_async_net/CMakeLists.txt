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

add_custom_target(
	sm_async_net ALL
	DEPENDS Compiler
	DEPENDS BuildSystem
	COMMAND
		BuildSystem build
		--build-configuration ${BUILD_SYSTEM_BUILD_CONFIGURATION}
		--compiler-executable $<TARGET_FILE:Compiler>
		--build-system-imports-path ${CMAKE_CURRENT_SOURCE_DIR}/../build_system/imports/
		--ustlib-path ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
		--project-directory ${CMAKE_CURRENT_SOURCE_DIR}
		--build-directory ${CMAKE_CURRENT_BINARY_DIR}
		${SYSROOT_OPTION}
	USES_TERMINAL
		)
