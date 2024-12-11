message( STATUS "Build build system" )

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	set( SPRACHE_COMPILER_OPT_OPTIONS -O0 -g )
else()
	set( SPRACHE_COMPILER_OPT_OPTIONS -O2 )
endif()

# Build rule for "Ü" files.
function( CompileUSource u_source_file out_object )
	file( RELATIVE_PATH current_subdir ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} )
	file( RELATIVE_PATH u_source_file_relative ${CMAKE_CURRENT_SOURCE_DIR} ${u_source_file} )

	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.o
		DEPENDS ${u_source_file} Compiler${CURRENT_COMPILER_GENERATION}
		COMMAND
			Compiler${CURRENT_COMPILER_GENERATION}
			${u_source_file}
			-o ${current_subdir}/${u_source_file_relative}.o
			-MF ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
			--verify-module
			${SPRACHE_COMPILER_OPT_OPTIONS} ${SPRACHE_COMPILER_PIC_OPTIONS} ${SPRACHE_COMPILER_ARCH_OPTIONS}
			--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
			--data-sections --function-sections # Put symbols into section in order to remove unused functions during linking.
		DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # Hack! cmake produces ninja build file with content like "build_system/main.u.o:", but launches custom command in current binary directory
		COMMENT "Building ${current_subdir}/${u_source_file_relative}.o"
		)
	set( ${out_object} ${u_source_file_relative}.o PARENT_SCOPE )

endfunction()

file( GLOB BUILD_SYSTEM_HEADERS "*.uh" "json/*.uh" "imports/*.uh" )
file( GLOB BUILD_SYSTEM_SOURCES "*.u" "json/*.u" )

if( WIN32 )
	file( GLOB BUILD_SYSTEM_SOURCES_WINDOWS "windows/*.u" )
	set( BUILD_SYSTEM_SOURCES ${BUILD_SYSTEM_SOURCES};${BUILD_SYSTEM_SOURCES_WINDOWS} )

	file( GLOB BUILD_SYSTEM_HEADERS_WINDOWS "windows/*.uh" )
	set( BUILD_SYSTEM_HEADERS ${BUILD_SYSTEM_HEADERS};${BUILD_SYSTEM_HEADERS_WINDOWS} )

	file( GLOB BUILD_SYSTEM_SOURCES_CPP "windows/*.cpp" )
else()
	file( GLOB BUILD_SYSTEM_SOURCES_UNIX "unix/*.u" )
	set( BUILD_SYSTEM_SOURCES ${BUILD_SYSTEM_SOURCES};${BUILD_SYSTEM_SOURCES_UNIX} )

	file( GLOB BUILD_SYSTEM_HEADERS_UNIX "unix/*.uh" )
	set( BUILD_SYSTEM_HEADERS ${BUILD_SYSTEM_HEADERS};${BUILD_SYSTEM_HEADERS_UNIX} )

	file( GLOB BUILD_SYSTEM_SOURCES_CPP "unix/*.cpp" )
endif()

foreach( source ${BUILD_SYSTEM_SOURCES} )
	CompileUSource( ${source} out_object )
	list( APPEND BUILD_SYSTEM_OBJECTS ${out_object} )
endforeach()

add_executable( BuildSystem ${BUILD_SYSTEM_SOURCES} ${BUILD_SYSTEM_HEADERS} ${BUILD_SYSTEM_OBJECTS} ${BUILD_SYSTEM_SOURCES_CPP} )

if( NOT WIN32 )
	target_link_libraries( BuildSystem PRIVATE dl )
endif()

install( TARGETS BuildSystem )

file( GLOB BUILD_SYSTEM_IMPORTS "imports/*" )
install( FILES ${BUILD_SYSTEM_IMPORTS} DESTINATION "build_system/imports" )

# The build system uses the compiler executable itself to produce shared libraries and executables.
# This requires internal LLD.
# So, if no LLD is linked, disable build system tests.
if( U_BUILD_PY_TESTS AND TARGET lldCommon )

	# Run tests for build system.
	# Do this if the build system executable or some of tests were changed.
	# Use combination of a custom command and custom target in order to achieve this.

	make_directory( ${CMAKE_CURRENT_BINARY_DIR}/tests )

	file( GLOB_RECURSE TESTS_FILES "tests/*" ) # All files of tests, including the test script file, build scripts, sources, etc.

	file( GLOB_RECURSE USTLIB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/*.u" )

	set( TIMESTAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/tests_timestamp )

	# Actual command which runs tests.
	add_custom_command(
		OUTPUT ${TIMESTAMP_FILE}
		COMMAND
			COMMAND ${CMAKE_COMMAND} -E env PYTHONIOENCODING=utf-8 ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py
			--tests-path ${CMAKE_CURRENT_SOURCE_DIR}/tests/
			--tests-build-root-path ${CMAKE_CURRENT_BINARY_DIR}/tests
			--build-system-executable ${CMAKE_CURRENT_BINARY_DIR}/BuildSystem
			--compiler-executable ${CMAKE_CURRENT_BINARY_DIR}/../compiler0/Compiler # For now hardcode here path to the compiler executable build
			--build-system-imports-path ${CMAKE_CURRENT_SOURCE_DIR}/imports
			--ustlib-path ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib
		COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE} # A hacky step to produce a marker file for this command.
		DEPENDS BuildSystem
		DEPENDS ${TESTS_FILES}
		DEPENDS ${USTLIB_FILES} # Build scripts and sources may import "ustlib". So, make sure we re-run tests if something in ustlib changes.
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)

	# A custom target which is needed only to execute the custom command above.
	add_custom_target(
		BuildSystemTestsRun ALL
		DEPENDS ${TIMESTAMP_FILE} # Depend on the previous custom commadntimestamp file.
		SOURCES ${TESTS_FILES}
		)

endif()
