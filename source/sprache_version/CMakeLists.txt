set( SPRACHE_VERSION "0.8" )

option( U_EMBED_GIT_REVISION "Embed git resivion into compiler executable." YES )

add_library( SpracheVersion sprache_version.cpp sprache_version.hpp UpdateGitRevision.cmake )
target_compile_definitions( SpracheVersion PRIVATE "-DSPRACHE_VERSION=\"${SPRACHE_VERSION}\"" )
target_include_directories( SpracheVersion PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )

set( GENERATED_GIT_REVISION_FILE ${CMAKE_CURRENT_BINARY_DIR}/git_revision.hpp )

if( U_EMBED_GIT_REVISION )

	# Custom target builds each time, but it changes file with revision only if needed.
	add_custom_target(
		GenGitRevisionHeader
		COMMAND ${CMAKE_COMMAND} -DOUTPUT_HEADER_PATH=${GENERATED_GIT_REVISION_FILE} -P ${CMAKE_CURRENT_SOURCE_DIR}/UpdateGitRevision.cmake
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)

	# Library depends on generated file.
	add_dependencies( SpracheVersion GenGitRevisionHeader )

else()
	file( GENERATE OUTPUT ${GENERATED_GIT_REVISION_FILE} CONTENT "\"0000000000000000000000000000000000000000\"" )
endif()
