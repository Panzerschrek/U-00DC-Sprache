file( GLOB_RECURSE SOURCES "*.cpp" )
file( GLOB_RECURSE HEADERS "*.hpp" )

add_executable( Interpreter ${SOURCES} ${HEADERS} )
target_link_libraries( Interpreter CodeBuilderLib ${LLVM_LIBS_FOR_COMPILER} )
