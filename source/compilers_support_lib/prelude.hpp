#pragma once
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/DataLayout.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"


namespace U
{

std::string GenerateCompilerPreludeCode(
	const llvm::Triple& target_triple,
	const llvm::DataLayout& data_layout,
	llvm::StringRef features,
	llvm::StringRef cpu_name,
	char optimization_level,
	bool generate_debug_info,
	 uint32_t compiler_generatio );

} // namespace U
