if( U_BUILD_COMPILER )
	message( STATUS "Build compiler" )

	file( GLOB COMPILERS_COMMON_LIB_SOURCES "*.cpp" "*.hpp" )
	if( TARGET lldCommon )
		file( GLOB COMPILERS_COMMON_LIB_EXCLUDE_SOURCES "linker_stub.cpp" )
	else()
		file( GLOB COMPILERS_COMMON_LIB_EXCLUDE_SOURCES "linker.cpp" "linker_coff.cpp" "linker_elf_freebsd.cpp" "linker_elf_linux.cpp" "linker_macho.cpp" "linker_mingw.cpp" "linker_wasm.cpp" )
	endif()
	list( REMOVE_ITEM COMPILERS_COMMON_LIB_SOURCES ${COMPILERS_COMMON_LIB_EXCLUDE_SOURCES} )

	add_library( CompilersCommonLib ${COMPILERS_COMMON_LIB_SOURCES} )
	target_link_libraries( CompilersCommonLib CompilersSupportLib ${LLVM_LIBS_FOR_COMPILER} )

	if( TARGET lldCommon )
		# Hack! LLD libraries do not export their include directories, set it manually.
		target_include_directories( CompilersCommonLib PRIVATE ${LLVM_EXTERNAL_LLD_SOURCE_DIR}/include )
		target_include_directories( CompilersCommonLib PRIVATE ${CMAKE_BINARY_DIR}/llvm/tools/lld/include )

		target_link_libraries( CompilersCommonLib lldCOFF lldELF lldMachO lldMinGW lldWasm )
	endif()

else()
	message( STATUS "Skip compiler" )
endif()
