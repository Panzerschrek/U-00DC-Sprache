#pragma once
#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Module.h>
#include "pop_llvm_warnings.hpp"


namespace U
{

/*
	Run force-inlining for async functions calls via "await".
	This function should be run before any other optimization and coroutine pases,
	since it relies on code structure produced by the compiler.
*/
void InlineAsyncCalls( llvm::Module& module );

} // namespace U
