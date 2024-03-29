#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include "pop_llvm_warnings.hpp"

#include "return_value_optimization.hpp"

namespace U
{

namespace
{

const llvm::Value* GetCallee( const llvm::CallInst* const call_instruction )
{
	return call_instruction->getOperand( call_instruction->getNumOperands() - 1u ); // Function is last operand
}

} // namespace

void TryToPerformReturnValueAllocationOptimization( llvm::Function& function )
{
	/*
	In case of "s_ret" function (with composite return value) try to perform return value optimization.
	Check all usages of "s_ret".
	If all usages are just call to "memcpy" with "s_ret" as destination and source of memcpy is always same "alloca" instruction,
	replace this "alloca" with "s_ret" and remove "memcpy".
	Additionaly remove "lifetime.end", associated with this allocation.

	Sich optimization removes unnecessary "memcpy" call in case of single move-return or multtiple move-return of same value.

	Usage of "s_ret" (as destination for copy constructor call, for exaple) prevents this optimization.
	Also prevents it usage of different variables for move-return, including different temp variables.
	Obviously it's not possible to replace function argument with "s_ret", if function just move-returns its value argument.
	*/

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
			const llvm::Value* const callee= GetCallee( call_instruction );
			if( const auto callee_function= llvm::dyn_cast<llvm::Function>( callee ) )
			{
				if( callee_function->getIntrinsicID() != llvm::Intrinsic::memcpy )
					return; // Returnning non-alloc - can't perform the optimization.

				if( call_instruction->getOperand(0) != s_ret_value ) // Destination of "memcpy" should be "s_ret".
					return; // This should not actually happen.

				llvm::Value* const memcpy_src= call_instruction->getOperand(1);
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
	{
		llvm::SmallVector<llvm::User*, 8> memcpy_calls_to_remove;
		for( llvm::User* const user : s_ret_value->users() )
			memcpy_calls_to_remove.push_back(user);

		for( llvm::User* const user : memcpy_calls_to_remove )
		{
			if( const auto instr= llvm::dyn_cast<llvm::Instruction>(user) )
				instr->eraseFromParent();
		}
	}

	// Remove lifetime.end instructions, associated with initial "alloca"
	{
		// TODO - is it correct to remove all "lifetime.end" instructions?
		llvm::SmallVector<llvm::Instruction*, 8> lifetime_end_call_to_remove;
		for( llvm::User* const user : alloca_for_replacing->users() )
		{
			if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( user ) )
			{
				const llvm::Value* const callee= GetCallee( call_instruction );
				if( const auto callee_function= llvm::dyn_cast<llvm::Function>( callee ) )
					if( callee_function->getIntrinsicID() == llvm::Intrinsic::lifetime_end  ||
						callee_function->getName() == "__U_debug_lifetime_end" )
						lifetime_end_call_to_remove.push_back( call_instruction );
			}
		}

		for( llvm::Instruction* const instr : lifetime_end_call_to_remove )
			instr->eraseFromParent();
	}

	// Replace "alloca" with "s_ret".
	alloca_for_replacing->replaceAllUsesWith( s_ret_value );
}

} // namespace U
