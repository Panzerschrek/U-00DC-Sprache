# Build rule for "Ü" files.
function( CompileUSource u_source_file out_object )
	file( RELATIVE_PATH u_source_file_relative ${CMAKE_CURRENT_SOURCE_DIR} ${u_source_file} )
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.o
		DEPENDS ${u_source_file} ${CURRENT_COMPILER}
		COMMAND
			${CURRENT_COMPILER}
			${CMAKE_CURRENT_SOURCE_DIR}/${u_source_file_relative}
			-o ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.o
			-MF ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
			${SPRACHE_COMPILER_PIC_OPTIONS}
			--include-dir ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib/
		DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.d
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
	set( ${out_object} ${CMAKE_CURRENT_BINARY_DIR}/${u_source_file_relative}.o PARENT_SCOPE )
endfunction()

# Returns list of objects in ${out_objects}
function( CompileUSources u_source_files out_objects )
	foreach( source ${${u_source_files}} )
		CompileUSource( ${source} out_object )
		list( APPEND out_objects_local ${out_object} )
	endforeach()
	set( ${out_objects} ${out_objects_local} PARENT_SCOPE )
endfunction()

file( GLOB LEX_SYNT_LIB_SOURCES "lex_synt_lib/*.u" )
CompileUSources( LEX_SYNT_LIB_SOURCES LEX_SYNT_LIB_OBJECTS )
add_library( LexSyntLib${CURRENT_COMPLIER_GENERATION} lib_stub.cpp ${LEX_SYNT_LIB_SOURCES} ${LEX_SYNT_LIB_OBJECTS} )

file( GLOB TESTS_SOURCES "tests/*.u" )
CompileUSources( TESTS_SOURCES TESTS_OBJECTS )
add_executable( Tests${CURRENT_COMPLIER_GENERATION} lib_stub.cpp ${TESTS_SOURCES} ${TESTS_OBJECTS} )
target_link_libraries( Tests${CURRENT_COMPLIER_GENERATION} PRIVATE LexSyntLib${CURRENT_COMPLIER_GENERATION} )
