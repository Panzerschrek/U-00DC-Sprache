#pragma once
#include <llvm-c/Types.h>

// If contents of this file changed, funcs_u.uh must be changed too!

extern "C"
{

// Return pointer to llvm module. Use "delete" to delete result.
// Returns null if program build failed and prints errors to stderr.
LLVMModuleRef U1_BuildProgram( const char* program_text_null_terminated, LLVMContextRef llvm_context );

using ErrorHanglerFunc= void(*)( void* data, uint32_t line, uint32_t column, uint32_t error_code, const char* error_text, size_t error_text_length );

bool U1_BuildProgramWithErrors( const char* program_text_null_terminated, LLVMContextRef llvm_context, ErrorHanglerFunc error_handler_func, void* data );

} // extern "C"
