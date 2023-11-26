#pragma once
#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Module.h>
#include "pop_llvm_warnings.hpp"


namespace U
{

void InlineAsyncCalls( llvm::Module& module );

} // namespace U
