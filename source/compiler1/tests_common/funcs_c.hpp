#pragma once
#include <llvm-c/Target.h>
#include <llvm-c/Types.h>

// If contents of this file changed, funcs_u.uh must be changed too!

extern "C"
{

struct U1_StringView
{
	const char* data;
	size_t size;
};

struct U1_SourceFile
{
	U1_StringView file_path;
	U1_StringView file_content;
};

using UserHandle= size_t;

struct ErrorsHandlingCallbacks
{
	UserHandle (*error_callback)( UserHandle data, uint32_t line, uint32_t column, uint32_t error_code, const U1_StringView& error_text );
	UserHandle (*template_errors_context_callback)( UserHandle data, uint32_t line, uint32_t column, const U1_StringView& context_name, const U1_StringView& args_description );
};

// Return pointer to llvm module. Use "delete" to delete result.
// Returns null if program build failed and prints errors to stderr.
LLVMModuleRef U1_BuildProgram(
	const U1_StringView& program_text_start,
	LLVMContextRef llvm_context,
	LLVMTargetDataRef data_layout );

bool U1_BuildProgramWithErrors(
	const U1_StringView& program_text_start,
	LLVMContextRef llvm_context,
	LLVMTargetDataRef data_layout,
	const ErrorsHandlingCallbacks& errors_handling_callbacks,
	UserHandle data );

void U1_BuildProgramWithSyntaxErrors(
	const U1_StringView& program_text_start,
	const ErrorsHandlingCallbacks& errors_handling_callbacks,
	UserHandle data );

LLVMModuleRef U1_BuildMultisourceProgram(
	const U1_SourceFile* source_files,
	size_t source_file_count,
	const U1_StringView& root_file_path,
	LLVMContextRef llvm_context,
	LLVMTargetDataRef data_layout );

bool U1_BuildMultisourceProgramWithErrors(
	const U1_SourceFile* source_files,
	size_t source_file_count,
	const U1_StringView& root_file_path,
	LLVMContextRef llvm_context,
	LLVMTargetDataRef data_layout,
	const ErrorsHandlingCallbacks& errors_handling_callbacks,
	UserHandle data );

struct IVfsInterface
{
	using FillStringCallback= void(*)( UserHandle user_data, const U1_StringView& result_path_normalized );

	UserHandle this_;
	void (*normalize_path_function)( UserHandle this_, const U1_StringView& file_path, const U1_StringView& parent_file_path_normalized, FillStringCallback result_callback, UserHandle user_data );
	bool (*load_file_content_function)( UserHandle this_, const U1_StringView& file_path, const U1_StringView& parent_file_path_normalized, FillStringCallback result_callback, UserHandle user_data );
};

extern "C" LLVMModuleRef U1_BuildProgrammUsingVFS(
	const IVfsInterface& vfs_interface,
	const U1_StringView& root_file_path,
	LLVMContextRef llvm_context,
	LLVMTargetDataRef data_layout );

// Returns static string for error code.
void U1_CodeBuilderCodeToString(
	uint32_t error_code,
	const char*& out_text,
	size_t& out_text_length );

} // extern "C"
