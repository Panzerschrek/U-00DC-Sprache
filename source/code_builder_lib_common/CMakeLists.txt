file( GLOB CODE_BUILDER_COMMON_SOURCES "*cpp" "*.hpp" )
add_library( CodeBuilderLibCommon ${CODE_BUILDER_COMMON_SOURCES} )
target_link_libraries( CodeBuilderLibCommon ${LLVM_LIBS_FOR_CODE_BUILDER_LIB} LexSyntLibCommon )
