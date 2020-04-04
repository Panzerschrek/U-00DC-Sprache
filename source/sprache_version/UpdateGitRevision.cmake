option( OUTPUT_HEADER_PATH "Path to output header file" "" )

find_package( Git REQUIRED QUIET )

execute_process(
	COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
	OUTPUT_VARIABLE GIT_CURRENT_COMMIT_HASH
	OUTPUT_STRIP_TRAILING_WHITESPACE
	)

set(
	HEADER_CONTENT
	"\"${GIT_CURRENT_COMMIT_HASH}\"" )

# Revrite header file only if it's content different, than current content.
# This neads, because we whant to rebuild dependent files only if revision changed.
if( EXISTS ${OUTPUT_HEADER_PATH} )
	file(
		READ
		${OUTPUT_HEADER_PATH}
		FILE_CONTENT_PREV
		)
endif()

if( NOT HEADER_CONTENT STREQUAL FILE_CONTENT_PREV )
	file(
		WRITE
		${OUTPUT_HEADER_PATH}
		${HEADER_CONTENT}
		)
	message( "Update git revison header: ${GIT_CURRENT_COMMIT_HASH}" )
endif()
