#pragma once
#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Function.h>
#include "pop_llvm_warnings.hpp"

namespace U
{

void TryToPerformReturnValueAllocationOptimization( llvm::Function& fucnction );

} // namespace U
