#pragma once
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Module.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

namespace U
{

enum class HaltMode{ Trap, Abort, ConfigurableHandler, Unreachable, };

bool LinkCompilerBuiltinModules( llvm::Module& result_module, HaltMode halt_mode, bool no_system_alloc );

} // namespace U
