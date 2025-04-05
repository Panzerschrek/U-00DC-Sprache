file( GLOB LIB_SOURCES "*.cpp" )
file( GLOB LIB_HEADERS "*.hpp" )

add_library( LanguageServerLib ${LIB_SOURCES} ${LIB_HEADERS} )
# Link directly against CodeBuilderLib of compiler0. There is no reason to use compiler1 for the LanguageServer.
target_link_libraries( LanguageServerLib PUBLIC CodeBuilderLib CompilersSupportLib )

add_executable( LanguageServer "tool/main.cpp" )
target_link_libraries( LanguageServer PRIVATE LanguageServerLib )
install( PROGRAMS $<TARGET_FILE:LanguageServer> TYPE BIN RENAME u.._language_server${CMAKE_EXECUTABLE_SUFFIX} )

if( U_BUILD_TESTS )
	file( GLOB TEST_SOURCES "tests/*.cpp" )
	file( GLOB TEST_HEADERS "tests/*.hpp" )
	add_executable( LanguageServerTests ${TEST_SOURCES} ${TEST_HEADERS} "tests/main.cpp" )
	target_link_libraries( LanguageServerTests PRIVATE TestsLib LanguageServerLib )
	add_custom_command( TARGET LanguageServerTests POST_BUILD COMMAND LanguageServerTests )
endif()
