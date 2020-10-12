if( NOT MSVC ) # Eenable only on compiler with Itanium ABI mangling.
	# Compile simple program on "Ü-Sprache", link it together with C++ program.
	set( CPP_LINKAGE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cpp_linkage.u )
	CompileUSources( "${CPP_LINKAGE_SOURCES}" CPP_LINKAGE_OBJECTS )

	add_executable( CPPLinkageTest cpp_linkage.cpp ${CPP_LINKAGE_SOURCES} ${CPP_LINKAGE_OBJECTS} )
	add_dependencies( CPPLinkageTest Compiler )
endif()
