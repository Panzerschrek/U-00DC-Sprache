#include <iostream>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Instructions.h>
#include "pop_llvm_warnings.hpp"
#include "async_calls_inlining.hpp"

namespace U
{

namespace
{

const llvm::Value* GetCallee( const llvm::CallInst& call_instruction )
{
	return call_instruction.getOperand( call_instruction.getNumOperands() - 1u ); // Function is last operand
}

void ExtractAllACoroutineFunctionCalls( llvm::Function& function, llvm::SmallVectorImpl<llvm::CallInst*>& out )
{
	for( llvm::BasicBlock& basic_block : function.getBasicBlockList() )
	{
		for( llvm::Instruction& instruction : basic_block.getInstList() )
		{
			if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( &instruction ) )
			{
				const llvm::Value* const callee= GetCallee( *call_instruction );
				if( const auto callee_function= llvm::dyn_cast<llvm::Function>( callee ) )
				{
					if( callee_function->hasFnAttribute( llvm::Attribute::PresplitCoroutine ) )
					{
						std::cout << "Find call to coroutine " << callee_function->getName().str() << std::endl;
						out.push_back( call_instruction );
					}
				}
			}
		}
	}
}

// Assume that the compiler uses coroutine call result only once - to store it in a local variable.
// Return the address of this variable.
llvm::AllocaInst* GetCoroutineObject( llvm::CallInst& call_instruction )
{
	llvm::AllocaInst* result= nullptr;
	for( llvm::User* const user : call_instruction.users() )
	{
		if( const auto store_instruction= llvm::dyn_cast<llvm::StoreInst>( user ) )
		{
			const auto address= store_instruction->getOperand(1u);
			if( const auto alloca_instruction= llvm::dyn_cast<llvm::AllocaInst>( address ) )
			{
				if( result != nullptr )
				{
				}
				result= alloca_instruction;
			}
			else
			{
				std::cout << "Store a coroutine not in \"alloca\"" << std::endl;
				return nullptr;
			}
		}
		else
		{
			// Something is wrong.
			std::cout << "Not a store instruction" << std::endl;
			return nullptr;
		}
	}

	if( result == nullptr )
		std::cout << "Can't fund \"alloca\" for coroutine object" << std::endl;

	return result;
}

void TryToInlineAsyncCall( llvm::Function& function, llvm::CallInst& call_instruction )
{
	llvm::AllocaInst* const coroutine_object= GetCoroutineObject( call_instruction );
	if( coroutine_object == nullptr )
		return;
	std::cout << "Work with coroutine object " << coroutine_object->getName().str() << std::endl;
}

void ProcessFunction( llvm::Function& function )
{
	// Apply the optimization only for async fnctions, that must have this attribute.
	if( !function.hasFnAttribute( llvm::Attribute::PresplitCoroutine ) )
		return;

	llvm::SmallVector<llvm::CallInst*, 8> async_calls;
	ExtractAllACoroutineFunctionCalls( function, async_calls );

	for( llvm::CallInst* const call_instruction : async_calls )
		TryToInlineAsyncCall( function, *call_instruction );
}

} // namespace

void InlineAsyncCalls( llvm::Module& module )
{
	for( llvm::Function& function : module.functions() )
		ProcessFunction( function );
}

} // namespace U
