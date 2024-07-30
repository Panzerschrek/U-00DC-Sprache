if( TARGET clangTooling )
	message( STATUS "Build c++ header converter" )
	
	llvm_map_components_to_libnames( LLVM_LIBS_FOR_CPP_HEADER_CONVERTER ${LLVM_TARGETS_TO_BUILD} )

	file( GLOB CPP_HEADER_CONVERTER_SOURCES "*.cpp" )
	add_executable( CppHeaderConverter ${CPP_HEADER_CONVERTER_SOURCES} )
	target_link_libraries( CppHeaderConverter LexSyntLib clangTooling ${LLVM_LIBS_FOR_CPP_HEADER_CONVERTER} )
	# Hack! Clang libraries do not export their include directories, set it manually.
	target_include_directories( CppHeaderConverter PRIVATE ${LLVM_EXTERNAL_CLANG_SOURCE_DIR}/include )
	target_include_directories( CppHeaderConverter PRIVATE ${CMAKE_BINARY_DIR}/llvm/tools/clang/include )

	install( TARGETS CppHeaderConverter )

else()
	message( STATUS "Clang not found, c++ header converter build disabled" )
endif()
