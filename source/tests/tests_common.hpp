#pragma once
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

namespace U
{

inline std::string GetTestsDataLayout()
{
	std::string result;

	result+= llvm::sys::IsBigEndianHost ? "E" : "e";
	const bool is_32_bit= sizeof(void*) <= 4u;
	result+= is_32_bit ? "-p:32:32" : "-p:64:64";
	result+= is_32_bit ? "-n8:16:32" : "-n8:16:32:64";
	result+= "-i8:8-i16:16-i32:32-i64:64";
	result+= "-f32:32-f64:64";
	result+= "-S128";

	return result;
}

inline llvm::Triple GetTestsTargetTriple()
{
	return llvm::Triple();
}

} // namespace U
