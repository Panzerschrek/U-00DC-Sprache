#pragma once
#include <string>
#include <vector>
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/ArrayRef.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

namespace U
{

void DeduplicateAndFilterDepsList( std::vector<std::string>& deps_list );

bool WriteDepFile(
	const std::string& out_file_path,
	llvm::ArrayRef<std::string> deps_list, // Files list should not contain duplicates.
	const std::string& dep_file_path );

} // namespace U
