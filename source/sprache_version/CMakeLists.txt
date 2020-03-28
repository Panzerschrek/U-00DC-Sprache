cmake_minimum_required(VERSION 2.8)

set( SPRACHE_VERSION "0.5" )

# Custom target builds each time, but it changes file with revision only if needed.
add_custom_target(
	GenGitRevisionHeader
	COMMAND ${CMAKE_COMMAND} -DOUTPUT_HEADER_PATH=${CMAKE_CURRENT_BINARY_DIR}/git_revision.hpp -P ${CMAKE_CURRENT_SOURCE_DIR}/UpdateGitRevision.cmake
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	)


# Executable depends on generated file.
add_library( SpracheVersion sprache_version.cpp sprache_version.hpp UpdateGitRevision.cmake )
add_dependencies( SpracheVersion GenGitRevisionHeader )
target_include_directories( SpracheVersion PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )
target_compile_definitions( SpracheVersion PRIVATE "-DSPRACHE_VERSION=\"${SPRACHE_VERSION}\"" )

