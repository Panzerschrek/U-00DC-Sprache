#pragma once
#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/ArrayRef.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../compiler0/lex_synt_lib/source_graph_loader.hpp"

namespace U
{

std::unique_ptr<IVfs> CreateVfsOverSystemFS( llvm::ArrayRef<std::string> include_dirs );
std::unique_ptr<IVfs> CreateVfsOverSystemFSWithPrefixedPaths( llvm::ArrayRef<std::string> include_dirs_prefixed );

} // namespace U
