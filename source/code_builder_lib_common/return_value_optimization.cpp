#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include "pop_llvm_warnings.hpp"

#include "return_value_optimization.hpp"

namespace U
{

void TryToPerformReturnValueAllocationOptimization( llvm::Function& function )
{
	llvm::FunctionType* const function_type= function.getFunctionType();
	if( function_type->getNumParams() == 0 )
		return;

	const llvm::Type* first_param_type= function_type->getParamType(0);
	if( !first_param_type->isPointerTy() )
		return;

	const llvm::Type* const s_ret_type= function.getParamStructRetType(0);
	if( s_ret_type == nullptr )
		return;

	llvm::Value* const s_ret_value= function.arg_begin();

	// Check if all uses of "s_ret" is actually a uses with memcpy
	llvm::AllocaInst* alloca_for_replacing= nullptr;
	for( const llvm::User* const user : s_ret_value->users() )
	{
		if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( user ) )
		{
			const llvm::Value* const callee= call_instruction->getOperand(0);
			if( const auto callee_function= llvm::dyn_cast<llvm::Function>( callee ) )
			{
				if( callee_function->getIntrinsicID() != llvm::Intrinsic::memcpy )
					return; // Returnning non-alloc - can't perform the optimization.

				if( call_instruction->getOperand(1) != s_ret_value ) // Destination of "memcpy" should be "s_ret".
					return; // This should not actually happen.

				llvm::Value* const memcpy_src= call_instruction->getOperand(2);
				if( const auto memcpy_src_alloca= llvm::dyn_cast<llvm::AllocaInst>(memcpy_src) )
				{
					if( alloca_for_replacing == nullptr )
						alloca_for_replacing= memcpy_src_alloca;
					else if( memcpy_src_alloca != alloca_for_replacing )
						return; // Returning different "alloca" in different places - can't perform optimization.
				}
				else
					return; // Calling not a "memcpy" for s_ret".
			}
			else
				return; // Calling some function via pointer - wtf?
		}
		else
			return; // This is strange - "s_ret" used somowhere else.
	}

	if( alloca_for_replacing == nullptr )
		return;

	// Ok - can perform return value optimization.

	// First delete all "memcpy" instructions.
	llvm::SmallVector<llvm::User*, 4> instr_to_remove;
	for( llvm::User* const user : s_ret_value->users() )
		instr_to_remove.push_back(user);

	for( llvm::User* const user : instr_to_remove )
	{
		if( const auto instr= llvm::dyn_cast<llvm::Instruction>(user) )
			instr->eraseFromParent();
	}

	// TODO - remove also "lifetime.end" before "return".

	// Replace "alloca" with "s_ret".
	alloca_for_replacing->replaceAllUsesWith( s_ret_value );
}

} // namespace U
