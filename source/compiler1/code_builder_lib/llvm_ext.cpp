#include "../../code_builder_lib/push_disable_llvm_warnings.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include "../../code_builder_lib/pop_llvm_warnings.hpp"

extern "C" void U1_SetStructName(const LLVMTypeRef t, const char* const name)
{
	llvm::dyn_cast<llvm::StructType>(llvm::unwrap(t))->setName(name);
}

extern "C" void U1_DropAllBasicBlockUsersReferences(const LLVMBasicBlockRef basic_block)
{
	for( const auto use : llvm::unwrap(basic_block)->users() )
		use->dropAllReferences();
}
