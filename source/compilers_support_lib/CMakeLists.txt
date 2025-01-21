
# External AS needed for cross-builds.
if( DEFINED U_EXTERNAL_LLVM_AS )
	set( U_LLVM_AS ${U_EXTERNAL_LLVM_AS} )
else()
	set( U_LLVM_AS llvm-as )
endif()

file( MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bc_files )

function( CompileLLFile ll_file )
	get_filename_component( source_name ${ll_file} NAME_WE )
	set( bc_name ${CMAKE_CURRENT_BINARY_DIR}/bc_files/${source_name}.bc )
	set( bc_header_path ${CMAKE_CURRENT_BINARY_DIR}/bc_files_headers/${source_name}.h )
	add_custom_command(
		OUTPUT ${bc_name}
		DEPENDS ${ll_file}
		DEPENDS ${U_LLVM_AS}
		COMMAND ${U_LLVM_AS} ${ll_file} -o ${bc_name}
		)
	add_custom_command(
		OUTPUT ${bc_header_path}
		DEPENDS ${bc_name}
		DEPENDS ${CMAKE_SOURCE_DIR}/bin2c.cmake ${bc_name}
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/bin2c.cmake ${bc_name} ${bc_header_path} c_${source_name}_file_content
		)
	set( BC_FILE ${bc_header_path} PARENT_SCOPE )
endfunction()

set( USTLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ustlib )

file( GLOB_RECURSE USTLIB_LL_FILES "${USTLIB_DIR}/compiler_builtins/*" )

foreach( LL_FILE ${USTLIB_LL_FILES} )
	CompileLLFile( ${LL_FILE} )
	list( APPEND bc_files ${BC_FILE} )
endforeach()

file( GLOB COMPILERS_SUPPORT_LIB_SOURCES "*.cpp" "*.hpp" )
add_library( CompilersSupportLib ${COMPILERS_SUPPORT_LIB_SOURCES} ${bc_files} ${USTLIB_LL_FILES} )
target_link_libraries( CompilersSupportLib ${LLVM_LIBS_FOR_COMPILERS_SUPPORT_LIB} )
target_include_directories( CompilersSupportLib PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )
