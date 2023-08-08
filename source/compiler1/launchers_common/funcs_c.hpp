#pragma once
#include <llvm-c/Target.h>
#include <llvm-c/Types.h>

#include "../../code_builder_lib_common/mangling.hpp"

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

using U1_UserHandle= size_t;

struct U1_ErrorsHandlingCallbacks
{
	U1_UserHandle (*error_callback)( U1_UserHandle data, uint32_t file_index, uint32_t line, uint32_t column, uint32_t error_code, const U1_StringView& error_text );
	U1_UserHandle (*template_errors_context_callback)( U1_UserHandle data, uint32_t file_index, uint32_t line, uint32_t column, const U1_StringView& context_name, const U1_StringView& args_description );
};

struct U1_IVfsInterface
{
	using FillStringCallback= void(*)( U1_UserHandle user_data, const U1_StringView& result_path_normalized );

	U1_UserHandle this_;
	void (*normalize_path_function)( U1_UserHandle this_, const U1_StringView& file_path, const U1_StringView& parent_file_path_normalized, FillStringCallback result_callback, U1_UserHandle user_data );
	bool (*load_file_content_function)( U1_UserHandle this_, const U1_StringView& full_file_path, FillStringCallback result_callback, U1_UserHandle user_data );
};

using U1_SourceFilePathCallback= void(*)( U1_UserHandle data, const U1_StringView& file_path_normalized );

using U1_LexSyntErrorCallback= void(*)( U1_UserHandle data, uint32_t file_index, uint32_t line, uint32_t column, const U1_StringView& text );

// Return pointer to llvm module. Use "delete" to delete result.
// Returns null if program build failed and prints errors to stderr.
LLVMModuleRef U1_BuildProgram(
	const U1_StringView& program_text_start,
	LLVMContextRef llvm_context,
	LLVMTargetDataRef data_layout,
	bool enable_unsed_name_errors );

bool U1_BuildProgramWithErrors(
	const U1_StringView& program_text_start,
	LLVMContextRef llvm_context,
	LLVMTargetDataRef data_layout,
	const U1_ErrorsHandlingCallbacks& errors_handling_callbacks,
	U1_UserHandle data );

void U1_BuildProgramWithSyntaxErrors(
	const U1_StringView& program_text_start,
	const U1_ErrorsHandlingCallbacks& errors_handling_callbacks,
	U1_UserHandle data );

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
	const U1_ErrorsHandlingCallbacks& errors_handling_callbacks,
	U1_UserHandle data );

LLVMModuleRef U1_BuildProgramForLifetimesTest(
	const U1_StringView& program_text_start,
	LLVMContextRef llvm_context,
	LLVMTargetDataRef data_layout );

LLVMModuleRef U1_BuildProgramForMSVCManglingTest(
	const U1_StringView& program_text_start,
	LLVMContextRef llvm_context,
	LLVMTargetDataRef data_layout );

LLVMModuleRef U1_BuildProgramUsingVFS(
	const U1_IVfsInterface& vfs_interface,
	const U1_StringView& root_file_path,
	LLVMContextRef llvm_context,
	LLVMTargetDataRef data_layout,
	const U1_StringView& target_triple_str,
	bool build_debug_info,
	bool generate_tbaa_metadata,
	bool enable_unused_names,
	U::ManglingScheme mangling_scheme,
	const U1_StringView& prelude_code,
	U1_SourceFilePathCallback result_source_file_path_callback,
	U1_UserHandle result_source_file_path_processing_data,
	U1_LexSyntErrorCallback lex_synt_error_callback,
	U1_UserHandle lex_synt_error_callback_data,
	const U1_ErrorsHandlingCallbacks& errors_handling_callbacks,
	U1_UserHandle error_processing_data );

} // extern "C"
