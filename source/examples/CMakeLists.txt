if( TARGET BuildSystem AND TARGET lldCommon )

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

	file( GLOB EXAMPLES_SOURCES "*.u" )

	foreach( EXAMPLE_SOURCE ${EXAMPLES_SOURCES} )

		file( RELATIVE_PATH EXAMPLE_SOURCE_RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${EXAMPLE_SOURCE} )
		get_filename_component( EXAMPLE_SOURCE_BASE_NAME ${EXAMPLE_SOURCE_RELATIVE} NAME_WE )

		set( EXAMPLE_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE_SOURCE_BASE_NAME} )
		if( WIN32 )
			set( EXAMPLE_EXECUTABLE "${EXAMPLE_EXECUTABLE}.exe" )
		endif()

		# Use build system to build examples.
		# For now don't support DEPFILE - the build system doesn't generate it for single-file builds.

		add_custom_command(
			OUTPUT ${EXAMPLE_EXECUTABLE}
			DEPENDS ${EXAMPLE_SOURCE}
			DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
			DEPENDS BuildSystem
			COMMAND
				BuildSystem build_single ${EXAMPLE_SOURCE}
				-q
				--build-configuration ${BUILD_SYSTEM_BUILD_CONFIGURATION}
				--compiler-executable $<TARGET_FILE:Compiler${CURRENT_COMPILER_GENERATION}>
				--ustlib-path ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
				--build-directory ${CMAKE_CURRENT_BINARY_DIR}
				${SYSROOT_OPTION}
				)

		add_custom_target(
			Example_${EXAMPLE_SOURCE_BASE_NAME}${CURRENT_COMPILER_GENERATION} ALL
			DEPENDS ${EXAMPLE_EXECUTABLE}
			SOURCES ${EXAMPLE_SOURCE}
			)

	endforeach()

endif()
