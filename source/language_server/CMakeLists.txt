file( GLOB_RECURSE SOURCES "*.cpp" )
file( GLOB_RECURSE HEADERS "*.hpp" )

add_executable( LanguageServer ${SOURCES} ${HEADERS} )
# Link directly against CodeBuilderLib of compiler0. There is no reason to use compiler1 for the LanguageServer.
target_link_libraries( LanguageServer CodeBuilderLib CompilersSupportLib )
install( TARGETS LanguageServer )
