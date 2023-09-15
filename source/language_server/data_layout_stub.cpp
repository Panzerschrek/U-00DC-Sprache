#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Triple.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "data_layout_stub.hpp"

namespace U
{

namespace LangServer
{

llvm::DataLayout CreateStubDataLayout( const std::string& target_triple_str )
{
	const llvm::Triple target_triple( target_triple_str );
	(void)target_triple;

	return llvm::DataLayout( "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128" );
}

} // namespace LangServer

} // namespace U
