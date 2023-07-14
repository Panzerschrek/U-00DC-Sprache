file( GLOB_RECURSE SOURCES "*.cpp" )
file( GLOB_RECURSE HEADERS "*.hpp" )

# Hack - reuse vfs from compiler. TODO - move it into common plane.
set( EXTERNAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../compilers_common_lib/vfs.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../compilers_common_lib/errors_print.cpp" )

add_executable( Interpreter ${SOURCES} ${HEADERS} ${EXTERNAL_SOURCES} )
target_link_libraries( Interpreter CodeBuilderLib ${LLVM_LIBS_FOR_COMPILER} ${LLVM_LIBS_FOR_INTERPRETER} )
