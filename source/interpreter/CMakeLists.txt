file( GLOB_RECURSE SOURCES "*.cpp" )
file( GLOB_RECURSE HEADERS "*.hpp" )

add_executable( Interpreter ${SOURCES} ${HEADERS} )
# Link directly against CodeBuilderLib of compiler0. There is no reason to use compiler1 for interpreter.
target_link_libraries( Interpreter CodeBuilderLib CompilersSupportLib ${LLVM_LIBS_FOR_COMPILER} ${LLVM_LIBS_FOR_INTERPRETER} )
