if(WIN32)
	return()
endif()

# Compile simple program on "Ü-Sprache", link it together with C++ program.
set( CPP_LINKAGE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/cpp_linkage.u )
CompileUSources( "${CPP_LINKAGE_SOURCES}" CPP_LINKAGE_OBJECTS )

add_executable( CPPLinkageTest${CURRENT_COMPILER_GENERATION} cpp_linkage.cpp ${CPP_LINKAGE_SOURCES} ${CPP_LINKAGE_OBJECTS} )
add_dependencies( CPPLinkageTest${CURRENT_COMPILER_GENERATION} Compiler${CURRENT_COMPILER_GENERATION} )

# Run the test
add_custom_command( TARGET CPPLinkageTest${CURRENT_COMPILER_GENERATION} POST_BUILD COMMAND CPPLinkageTest${CURRENT_COMPILER_GENERATION} )
