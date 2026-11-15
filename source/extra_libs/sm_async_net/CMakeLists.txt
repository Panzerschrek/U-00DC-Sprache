if(WIN32)
	# For now Windows isn't supported.
	return()
endif()

if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
	# For now don't support 32-bit platforms - we require 64-bit atomic operations for unique task ID counter.
	return()
endif()

if( LLVM_DEFAULT_TARGET_TRIPLE STREQUAL "i686-unknown-linux-gnu" )
	# Fuck you Cmake!
	# Why can't you set CMAKE_SIZEOF_VOID_P to 4 if -m32 option is specified?
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

if( ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" )
	# For now disable running tests on FreeBSD - for some reason tests fail on this OS (but only in CI builds).
	set( BUILD_AND_TEST_COMMAND "build" )
else()
	set( BUILD_AND_TEST_COMMAND "test" )
endif()

add_custom_command(
	OUTPUT ${TESTS_EXECUTABLE}
	DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
	DEPENDS BuildSystem
	DEPENDS ${ALL_LIB_SOURCES}
	COMMAND
		BuildSystem ${BUILD_AND_TEST_COMMAND}
		--build-configuration ${BUILD_SYSTEM_BUILD_CONFIGURATION}
		--compiler-executable $<TARGET_FILE:Compiler${CURRENT_COMPILER_GENERATION}>
		--build-system-files-path ${CMAKE_CURRENT_SOURCE_DIR}/../../build_system/
		--ustlib-path ${CMAKE_CURRENT_SOURCE_DIR}/../../ustlib/
		--project-directory ${CMAKE_CURRENT_SOURCE_DIR}
		--build-directory ${CMAKE_CURRENT_BINARY_DIR}
		${SYSROOT_OPTION}
	USES_TERMINAL
		)

# Add custom target for tests executable.
add_custom_target(
	SmAsyncNetTests${CURRENT_COMPILER_GENERATION} ALL
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${BUILD_SYSTEM_BUILD_CONFIGURATION}/tests
	SOURCES ${ALL_LIB_SOURCES}
	)

# Install the library sources.
if( CURRENT_COMPILER_GENERATION STREQUAL "" )
	install( CODE " file( INSTALL ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION \${CMAKE_INSTALL_PREFIX}/extra_libs/ ) " )
endif()
