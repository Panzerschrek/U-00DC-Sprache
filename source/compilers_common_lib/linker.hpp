#pragma once

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Triple.h>
#include <llvm/ADT/ArrayRef.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"


namespace U
{

// Returns true if all ok.
bool RunLinker(
	const char* argv0,
	llvm::ArrayRef<std::string> additional_args,
	const llvm::Triple& triple,
	const std::string& input_temp_file_path,
	const std::string& output_file_path,
	bool produce_shared_library,
	bool remove_unreferenced_symbols );

bool RunLinkerCOFF(
	const char* argv0,
	llvm::ArrayRef<std::string> additional_args,
	const llvm::Triple& triple,
	const std::string& input_temp_file_path,
	const std::string& output_file_path,
	bool produce_shared_library,
	bool remove_unreferenced_symbols );

bool RunLinkerELF(
	const char* argv0,
	llvm::ArrayRef<std::string> additional_args,
	const llvm::Triple& triple,
	const std::string& input_temp_file_path,
	const std::string& output_file_path,
	bool produce_shared_library,
	bool remove_unreferenced_symbols );

} // namespace U
