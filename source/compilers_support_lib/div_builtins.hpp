#pragma once
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Module.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

namespace U
{

void GenerateDivBuiltIns( llvm::Module& module );

bool IsDivBuiltInLikeFunctionName( llvm::StringRef name );

} // namespace U
