#pragma once
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/DataLayout.h>
#include <llvm/TargetParser/Triple.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

namespace U
{

namespace LangServer
{

// Normally DataLaout is obtained via llvm::TargetMachine.
// But doing so requires linking against at least one target linrary, that drastically increases result language server executable size.
// So, use our own target data layout construction stub.
// It is not so bad to construct somehow incorrect data layout, since this doesn't affect any real compilation.
// Compiler frontend uses only few data layout features, like pointer size, types alignment and endianness.
llvm::DataLayout CreateStubDataLayout( const llvm::Triple& target_triple );

} // namespace LangServer

} // namespace U
