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
			--symbols-visibility=hidden # Make functions and variables hidden to avoid exporting them in ELF shared libraries (*.so)
		DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR} # Hack! cmake produces ninja build file with content like "compiler1/lex_syn_lib/file_pos.u.o:", but launches custom command in current binary directory
		COMMENT "Building ${current_subdir}/${u_source_file_relative}.o"
		)
	set( ${out_object} ${u_source_file_relative}.o PARENT_SCOPE )

endfunction()

file( GLOB BUILD_SYSTEM_HEADERS "*.uh" )
file( GLOB BUILD_SYSTEM_SOURCES "*.u" )

if( WIN32 )
	file( GLOB BUILD_SYSTEM_SOURCES_WINDOWS "windows/*.u" )
	set( BUILD_SYSTEM_SOURCES ${BUILD_SYSTEM_SOURCES};${BUILD_SYSTEM_SOURCES_WINDOWS} )

	file( GLOB BUILD_SYSTEM_SOURCES_CPP "windows/*.cpp" )
else()
	file( GLOB BUILD_SYSTEM_SOURCES_UNIX "unix/*.u" )
	set( BUILD_SYSTEM_SOURCES ${BUILD_SYSTEM_SOURCES};${BUILD_SYSTEM_SOURCES_UNIX} )

	file( GLOB BUILD_SYSTEM_SOURCES_CPP "unix/*.cpp" )
endif()

foreach( source ${BUILD_SYSTEM_SOURCES} )
	CompileUSource( ${source} out_object )
	list( APPEND BUILD_SYSTEM_OBJECTS ${out_object} )
endforeach()

add_executable( BuildSystem ${BUILD_SYSTEM_SOURCES} ${BUILD_SYSTEM_HEADERS} ${BUILD_SYSTEM_OBJECTS} ${BUILD_SYSTEM_SOURCES_CPP} )

if( WIN32 )
else()
	target_link_libraries( BuildSystem PRIVATE dl )
endif()

install( TARGETS BuildSystem )


if( U_BUILD_PY_TESTS )

	make_directory( ${CMAKE_CURRENT_BINARY_DIR}/tests )

	# TODO - run tests each time one of the sources is changed.
	add_custom_target(
		BuildSystemTestsRun # Do not include into "ALL", since it's relatively heavyweight.
		COMMAND
			${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py
			--tests-path ${CMAKE_CURRENT_SOURCE_DIR}/tests/
			--tests-build-root-path ${CMAKE_CURRENT_BINARY_DIR}/tests
			--build-system-executable ${CMAKE_CURRENT_BINARY_DIR}/BuildSystem
			--compiler-executable ${CMAKE_CURRENT_BINARY_DIR}/../compiler0/Compiler # For now hardcode here path to the compiler executable build
			--build-system-imports-path ${CMAKE_CURRENT_SOURCE_DIR}/imports
			--ustlib-path ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib
		DEPENDS BuildSystem
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py
		)

endif()
