#include "../../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <llvm/IR/BasicBlock.h>
#include "../../code_builder_lib/pop_llvm_warnings.hpp"

extern "C" void U1_DropAllBasicBlockUsersReferences(const LLVMBasicBlockRef basic_block)
{
	for( const auto use : llvm::unwrap(basic_block)->users() )
		use->dropAllReferences();
}
