if( U_BUILD_COMPILER )
	message( STATUS "Build compiler" )

	file( GLOB COMPILERS_COMMON_LIB_SOURCES "*.cpp" "*.hpp" )
	if( TARGET lldCommon )
		file( GLOB COMPILERS_COMMON_LIB_EXCLUDE_SOURCES "linker_stub.cpp" )
	else()
		file( GLOB COMPILERS_COMMON_LIB_EXCLUDE_SOURCES "linker.cpp" "linker_elf.cpp" "linker_coff.cpp" )
	endif()
	list( REMOVE_ITEM COMPILERS_COMMON_LIB_SOURCES ${COMPILERS_COMMON_LIB_EXCLUDE_SOURCES} )

	add_library( CompilersCommonLib ${COMPILERS_COMMON_LIB_SOURCES} )
	target_link_libraries( CompilersCommonLib CompilersSupportLib ${LLVM_LIBS_FOR_COMPILER} )

	if( TARGET lldCommon )
		# Hack! LLD libraries do not export their include directories, set it manually.
		target_include_directories( CompilersCommonLib PRIVATE ${LLVM_EXTERNAL_LLD_SOURCE_DIR}/include )
		target_include_directories( CompilersCommonLib PRIVATE ${CMAKE_BINARY_DIR}/llvm/tools/lld/include )

		# TODO - use other linkers.
		target_link_libraries( CompilersCommonLib lldCOFF lldELF lldMinGW )
	endif()

else()
	message( STATUS "Skip compiler" )
endif()
