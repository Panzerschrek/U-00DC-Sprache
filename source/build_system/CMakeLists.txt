message( STATUS "Build build system" )

file( GLOB BUILD_SYSTEM_HEADERS "src/*.uh" "src/json/*.uh" "imports/*.uh" )
file( GLOB BUILD_SYSTEM_SOURCES "src/*.u" "src/json/*.u" )

if( TARGET lldCommon AND "${CURRENT_COMPILER_GENERATION}" GREATER_EQUAL 1 )

	set( BUILD_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/build.u )

	set( BK_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/bk_build )

	if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
		set( BUILD_SYSTEM_BUILD_CONFIGURATION "debug" )
	else()
		set( BUILD_SYSTEM_BUILD_CONFIGURATION "release" )
	endif()

	set( BUILD_SYSTEM_OUT_FILE ${BK_BUILD_DIR}/${BUILD_SYSTEM_BUILD_CONFIGURATION}/BuildSystem )

	add_custom_command(
		OUTPUT ${BUILD_SYSTEM_OUT_FILE}
		DEPENDS ${BUILD_SYSTEM_HEADERS} ${BUILD_SYSTEM_SOURCES}
		DEPENDS ${BUILD_SCRIPT_FILE}
		DEPENDS Compiler${CURRENT_COMPILER_GENERATION}
		DEPENDS BuildSystem
		COMMAND
			BuildSystem build
			--build-configuration ${BUILD_SYSTEM_BUILD_CONFIGURATION}
			--compiler-executable $<TARGET_FILE:Compiler${CURRENT_COMPILER_GENERATION}>
			--build-system-imports-path ${CMAKE_CURRENT_SOURCE_DIR}/imports/
			--ustlib-path ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
			--project-directory ${CMAKE_CURRENT_SOURCE_DIR}
			--build-directory ${BK_BUILD_DIR}
		USES_TERMINAL
			)

	add_custom_target(
		QQBuildSystem${CURRENT_COMPILER_GENERATION} ALL
		DEPENDS ${BUILD_SYSTEM_OUT_FILE}
		SOURCES ${BUILD_SYSTEM_SOURCES} ${BUILD_SYSTEM_HEADERS} ${BUILD_SCRIPT_FILE}
		)
endif()

if( ${CMAKE_BUILD_TYPE} STREQUAL Debug )
	set( SPRACHE_COMPILER_OPT_OPTIONS -O0 -g )
else()
	set( SPRACHE_COMPILER_OPT_OPTIONS -O2 )
endif()

# Build rule for "Ü" files.
function( CompileUSource u_source_file out_object )
	file( RELATIVE_PATH current_subdir ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} )
	file( RELATIVE_PATH u_source_file_relative ${CMAKE_CURRENT_SOURCE_DIR} ${u_source_file} )

	if( ${CMAKE_GENERATOR} STREQUAL "Ninja" )
		set( DEPFILE_OPTION DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d )
	else()
		set( DEPFILE_OPTION "" )
	endif()

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
			# Provide source directories in order to mark symbols declared in headers within these directories as hidden.
			--source-dir ${CMAKE_CURRENT_SOURCE_DIR}
			--data-sections --function-sections # Put symbols into section in order to remove unused functions during linking.
		${DEPFILE_OPTION}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # Hack! cmake produces ninja build file with content like "build_system/main.u.o:", but launches custom command in current binary directory
		COMMENT "Building ${current_subdir}/${u_source_file_relative}.o"
		)
	set( ${out_object} ${u_source_file_relative}.o PARENT_SCOPE )

endfunction()

if( WIN32 )
	file( GLOB BUILD_SYSTEM_SOURCES_WINDOWS "src/windows/*.u" )
	set( BUILD_SYSTEM_SOURCES ${BUILD_SYSTEM_SOURCES};${BUILD_SYSTEM_SOURCES_WINDOWS} )

	file( GLOB BUILD_SYSTEM_HEADERS_WINDOWS "src/windows/*.uh" )
	set( BUILD_SYSTEM_HEADERS ${BUILD_SYSTEM_HEADERS};${BUILD_SYSTEM_HEADERS_WINDOWS} )
else()
	file( GLOB BUILD_SYSTEM_SOURCES_UNIX "src/unix/*.u" )
	set( BUILD_SYSTEM_SOURCES ${BUILD_SYSTEM_SOURCES};${BUILD_SYSTEM_SOURCES_UNIX} )

	file( GLOB BUILD_SYSTEM_HEADERS_UNIX "src/unix/*.uh" )
	set( BUILD_SYSTEM_HEADERS ${BUILD_SYSTEM_HEADERS};${BUILD_SYSTEM_HEADERS_UNIX} )
endif()

foreach( source ${BUILD_SYSTEM_SOURCES} )
	CompileUSource( ${source} out_object )
	list( APPEND BUILD_SYSTEM_OBJECTS ${out_object} )
endforeach()

add_executable( BuildSystem${CURRENT_COMPILER_GENERATION} ${BUILD_SYSTEM_SOURCES} ${BUILD_SYSTEM_HEADERS} ${BUILD_SYSTEM_OBJECTS} )
# Since we have no C or C++ sources, set linker language to C, in order to execute a C linker for linking the build system executable.
# C++ is unnecessary here - we don't want to link against C++ standard libraries.
set_target_properties( BuildSystem${CURRENT_COMPILER_GENERATION} PROPERTIES LINKER_LANGUAGE C )

if( MINGW )
	target_link_options( BuildSystem${CURRENT_COMPILER_GENERATION} PRIVATE "-municode" ) # Allow "wmain" as entry point.
endif()

if( NOT WIN32 )
	target_link_libraries( BuildSystem${CURRENT_COMPILER_GENERATION} PRIVATE dl )
endif()

install( TARGETS BuildSystem${CURRENT_COMPILER_GENERATION} )

file( GLOB BUILD_SYSTEM_IMPORTS "imports/*" )
install( FILES ${BUILD_SYSTEM_IMPORTS} DESTINATION "build_system/imports" )

# The build system uses the compiler executable itself to produce shared libraries and executables.
# This requires internal LLD.
# So, if no LLD is linked, disable build system tests.
if( U_BUILD_BUILD_SYSTEM_TESTS AND TARGET lldCommon )

	# Create a couple of test libraries - to test linking against them
	add_library( BuildSystemTestsStaticLibrary${CURRENT_COMPILER_GENERATION} test_static_lib/test_static_lib.cpp )
	add_library( BuildSystemTestsSharedLibrary${CURRENT_COMPILER_GENERATION} SHARED test_shared_lib/test_shared_lib.cpp )
	# Create also a test executable for using in tests as command for custom build steps
	add_executable( BuildSystemTestFileGenerationTool${CURRENT_COMPILER_GENERATION} test_file_generation_tool/main.cpp )

	# Generate a configuration options JSON file for tests.
	set( CONFIGURATION_OPTIONS_FILE "${CMAKE_CURRENT_BINARY_DIR}/tests/tests_configuration_options.json" )

	set( CONFIGURATION_OPTIONS_FILE_CONTENTS
		"{\n\t\"tests_libs_directory\" : \"${CMAKE_CURRENT_BINARY_DIR}\",\n\
	\"tests_libs_postfix\" : \"${CURRENT_COMPILER_GENERATION}\",\n\
	\"test_file_generation_tool\" : \"${CMAKE_CURRENT_BINARY_DIR}/BuildSystemTestFileGenerationTool${CURRENT_COMPILER_GENERATION}\"\n}\n" )

	file( GENERATE OUTPUT ${CONFIGURATION_OPTIONS_FILE} CONTENT ${CONFIGURATION_OPTIONS_FILE_CONTENTS} )

	# Run tests for build system.
	# Do this if the build system executable or some of tests were changed.
	# Use combination of a custom command and custom target in order to achieve this.

	make_directory( ${CMAKE_CURRENT_BINARY_DIR}/tests )

	file( GLOB_RECURSE TESTS_FILES "tests/*" ) # All files of tests, including the test script file, build scripts, sources, etc.

	if( WIN32 )
		# HACK! There are problems with Unicode under windows (shitty ninja uses ASCII), so, ignore for now unicode test.
		file( GLOB_RECURSE UNICODE_TESTS_FILES "tests/Тест Юникода/*" )
		list( REMOVE_ITEM TESTS_FILES ${UNICODE_TESTS_FILES} )
	endif()

	file( GLOB_RECURSE USTLIB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/*.u" )

	set( TIMESTAMP_FILE ${CMAKE_CURRENT_BINARY_DIR}/tests_timestamp )

	if( WIN32 )
		if( MINGW )
			set( MANGLING_SCHEME "itaniumabi" )
		else()
			set( MANGLING_SCHEME "msvc" )
		endif()
	else()
		set( MANGLING_SCHEME "itaniumabi" )
	endif()

	if( MINGW )
		# We need to provide path to MinGW libraries for linking.
		# Detect MinGW installation using g++ path.
		get_filename_component( MINGW_BIN_DIR ${CMAKE_CXX_COMPILER} DIRECTORY )
		set( MINGW_INSTALLATION_PATH ${MINGW_BIN_DIR}/../ )
		set( SYSROOT_OPTION "--sysroot \"${MINGW_INSTALLATION_PATH}\"" )
	else()
		set( SYSROOT_OPTION "" )
	endif()

	# Actual command which runs tests.
	add_custom_command(
		OUTPUT ${TIMESTAMP_FILE}
		COMMAND
			COMMAND ${CMAKE_COMMAND} -E env PYTHONIOENCODING=utf-8 ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py
			--tests-path ${CMAKE_CURRENT_SOURCE_DIR}/tests/
			--tests-build-root-path ${CMAKE_CURRENT_BINARY_DIR}/tests
			--build-system-executable ${CMAKE_CURRENT_BINARY_DIR}/BuildSystem${CURRENT_COMPILER_GENERATION}
			--compiler-executable $<TARGET_FILE:Compiler${CURRENT_COMPILER_GENERATION}>
			--build-system-imports-path ${CMAKE_CURRENT_SOURCE_DIR}/imports
			--ustlib-path ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib
			--configuration-options ${CONFIGURATION_OPTIONS_FILE}
			--mangling-scheme ${MANGLING_SCHEME}
			${SYSROOT_OPTION}
		COMMAND ${CMAKE_COMMAND} -E touch ${TIMESTAMP_FILE} # A hacky step to produce a marker file for this command.
		DEPENDS BuildSystem${CURRENT_COMPILER_GENERATION}
		DEPENDS ${TESTS_FILES}
		DEPENDS ${USTLIB_FILES} # Build scripts and sources may import "ustlib". So, make sure we re-run tests if something in ustlib changes.
		DEPENDS BuildSystemTestsStaticLibrary${CURRENT_COMPILER_GENERATION}
		DEPENDS BuildSystemTestsSharedLibrary${CURRENT_COMPILER_GENERATION}
		DEPENDS BuildSystemTestFileGenerationTool${CURRENT_COMPILER_GENERATION}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		)

	# A custom target which is needed only to execute the custom command above.
	add_custom_target(
		BuildSystemTestsRun${CURRENT_COMPILER_GENERATION} ALL
		DEPENDS ${TIMESTAMP_FILE} # Depend on the previous custom commadntimestamp file.
		SOURCES ${TESTS_FILES}
		)

endif()
