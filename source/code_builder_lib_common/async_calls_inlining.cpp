#include <iostream>
#include <optional>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib_common/assert.hpp"

#include "async_calls_inlining.hpp"

namespace U
{

namespace
{

llvm::Value* GetCallee( llvm::CallInst& call_instruction )
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

struct AwaitOperatorCoroutineInstructions
{
	llvm::LoadInst* coro_handle_load= nullptr;
	llvm::CallInst* destructor_call= nullptr;
};

std::optional<AwaitOperatorCoroutineInstructions> GetAwaitOperatorCoroutineInstructions( llvm::AllocaInst& coroutine_object )
{
	AwaitOperatorCoroutineInstructions res;
	llvm::StoreInst* initial_store_instruction= nullptr;

	for( llvm::User* const user : coroutine_object.users() )
	{
		if( const auto load_instruction= llvm::dyn_cast<llvm::LoadInst>( user ) )
		{
			if( load_instruction->getMetadata("u_await_coro_handle") == nullptr )
			{
				std::cout << "load for coroutine object outside await operator" << std::endl;
				return std::nullopt;
			}
			if( res.coro_handle_load != nullptr )
			{
				std::cout << "duplicated await" << std::endl;
				return std::nullopt;
			}
			res.coro_handle_load= load_instruction;
		}
		else if( const auto store_instruction= llvm::dyn_cast<llvm::StoreInst>( user ) )
		{
			// This is an initial store for this coroutine object.
			if( initial_store_instruction != nullptr )
			{
				std::cout << "Too much store instructions for the coroutine object" << std::endl;
				return std::nullopt;
			}
			initial_store_instruction= store_instruction;
		}
		else if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( user ) )
		{
			if( call_instruction->getMetadata( "u_await_destructor_call" ) != nullptr )
			{
				if( res.destructor_call != nullptr )
				{
					std::cout << "duplicated await" << std::endl;
					return std::nullopt;
				}
				res.destructor_call= call_instruction;
			}
			else
			{
				const llvm::Value* const callee= GetCallee( *call_instruction );
				if( const auto callee_function= llvm::dyn_cast<llvm::Function>( callee ) )
				{
					if( callee_function->getIntrinsicID() == llvm::Intrinsic::lifetime_start ||
						callee_function->getIntrinsicID() == llvm::Intrinsic::lifetime_end ||
						callee_function->getName() == "__U_debug_lifetime_start" ||
						callee_function->getName() == "__U_debug_lifetime_end" )
						continue; // Allow lifetime instructions for the coroutine object.

					//std::cout << "Unsupported call for coroutine object to function: " << callee_function->getName().str() << std::endl;
					//return std::nullopt;
					continue; // TODO - count this destructor call in the destructors branch.
				}

				std::cout << "Unsupported call for coroutine object" << std::endl;
				return std::nullopt;
			}
		}
		else
		{
			std::cout << "Unexpected instruction for coroutine object - await call optimization isn't possible" << std::endl;
			return std::nullopt;
		}
	}

	if( res.coro_handle_load == nullptr || res.destructor_call == nullptr )
		return std::nullopt;

	return res;
}

void CollectPromiseCalls( llvm::Value& coro_handle, llvm::SmallVectorImpl<llvm::Instruction*>& out_values )
{
	for( llvm::User* const user : coro_handle.users() )
	{
		if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( user ) )
		{
			if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
			{
				if( callee_function->getIntrinsicID() == llvm::Intrinsic::coro_promise &&
					call_instruction->getOperand(0u) == &coro_handle )
					out_values.push_back( call_instruction );
			}
		}
	}
}

void CollectDoneCalls( llvm::Value& coro_handle, llvm::SmallVectorImpl<llvm::Instruction*>& out_values )
{
	for( llvm::User* const user : coro_handle.users() )
	{
		if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( user ) )
		{
			if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
			{
				if( callee_function->getIntrinsicID() == llvm::Intrinsic::coro_done &&
					call_instruction->getOperand(0u) == &coro_handle )
					out_values.push_back( call_instruction );
			}
		}
	}
}

llvm::BasicBlock* GetAwaitLoopBlock( llvm::LoadInst& coro_handle_load )
{
	llvm::BasicBlock* result= nullptr;
	for( llvm::User* const user : coro_handle_load.users() )
	{
		if( const auto instruction= llvm::dyn_cast<llvm::Instruction>(user) )
		{
			llvm::BasicBlock* bb= instruction->getParent();
			if( bb == nullptr )
			{
				std::cout << "Instruction with no parent!" << std::endl;
				return nullptr;
			}

			bool has_coro_resume= false;
			for( const llvm::Instruction& instruction : bb->getInstList() )
			{
				if( instruction.getMetadata("u_await_resume") != nullptr )
				{
					has_coro_resume= true;
					break;
				}
			}
			if( !has_coro_resume )
				continue;

			if( result != nullptr && result != bb )
			{
				std::cout << "Duplicated await loop block!" << std::endl;
				return nullptr;
			}
			result= bb;
		}
		else
		{
			std::cout << "Unexpected coro handle load user kind - non-instruction" << std::endl;
			return nullptr;
		}
	}

	if( result == nullptr )
		std::cout << "Can't find await loop block" << std::endl;
	else
		std::cout << "Find await loop block " << result->getName().str() << std::endl;

	return result;
}

struct AwaitLoopBlock
{
	llvm::CallInst* resume_call= nullptr;
	llvm::CallInst* done_call= nullptr;
	llvm::BranchInst* done_br= nullptr;
	llvm::BasicBlock* done_block= nullptr;
	llvm::BasicBlock* not_done_block= nullptr;
};

std::optional<AwaitLoopBlock> ParseAwaitLoopBlock( llvm::BasicBlock& bb )
{
	AwaitLoopBlock result;

	auto it= bb.begin();
	const auto it_end= bb.end();

	if( it == it_end )
	{
		std::cout << "Invalid await loop block" << std::endl;
		return std::nullopt;
	}
	if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( &*it ) )
	{
		if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
		{
			if( callee_function->getIntrinsicID() == llvm::Intrinsic::coro_resume )
				result.resume_call= call_instruction;
			else
			{
				std::cout << "expected resume call, find another function call" << std::endl;
				return std::nullopt;
			}
		}
		else
		{
			std::cout << "expected resume call, find non-function call" << std::endl;
			return std::nullopt;
		}
	}
	else
	{
		std::cout << "expected resume call, foud non-call" << std::endl;
		return std::nullopt;
	}

	++it;
	if( it == it_end )
	{
		std::cout << "Invalid await loop block" << std::endl;
		return std::nullopt;
	}
	if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( &*it ) )
	{
		if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
		{
			if( callee_function->getIntrinsicID() == llvm::Intrinsic::coro_done )
				result.done_call= call_instruction;
			else
			{
				std::cout << "expected done call, find another function call" << std::endl;
				return std::nullopt;
			}
		}
		else
		{
			std::cout << "expected done call, find non-function call" << std::endl;
			return std::nullopt;
		}
	}
	else
	{
		std::cout << "expected done call, foud non-call" << std::endl;
		return std::nullopt;
	}

	++it;
	if( it == it_end )
	{
		std::cout << "Invalid await loop block" << std::endl;
		return std::nullopt;
	}
	if( const auto branch_instruction= llvm::dyn_cast<llvm::BranchInst>( &*it ) )
	{
		result.done_br= branch_instruction;
		if( !branch_instruction->isConditional() )
		{
			std::cout << "Invalid branching at the end of the await loop block" << std::endl;
			return std::nullopt;
		}
		result.done_block= llvm::dyn_cast<llvm::BasicBlock>( branch_instruction->getOperand( 2u ) );
		result.not_done_block= llvm::dyn_cast<llvm::BasicBlock>( branch_instruction->getOperand( 1u ) );
		if( result.done_block == nullptr || result.not_done_block == nullptr )
		{
			std::cout << "Wrong await loop branching destination" << std::endl;
			return std::nullopt;
		}
		std::cout << "Found done block: " << result.done_block->getName().str() << std::endl;
		std::cout << "Found not done block: " << result.not_done_block->getName().str() << std::endl;
	}
	else
	{
		std::cout << "expected branching" << std::endl;
		return std::nullopt;
	}

	std::cout << "Successfully parsed await loop block" << std::endl;
	return result;
}

llvm::Function* CreateCalleeAsyncFunctionClone( llvm::CallInst& call_instruction )
{
	llvm::Value* const callee= GetCallee( call_instruction );
	U_ASSERT( callee != nullptr );
	llvm::Function* const callee_function= llvm::dyn_cast<llvm::Function>( callee );
	U_ASSERT( callee_function != nullptr );
	U_ASSERT( callee_function->hasFnAttribute( llvm::Attribute::PresplitCoroutine ) );

	llvm::ValueToValueMapTy map;
	llvm::Function* const new_function= llvm::CloneFunction( callee_function, map );

	return new_function;
}

struct CoroutineBlocks
{
	llvm::BasicBlock* allocations_block= nullptr;
	llvm::BasicBlock* block_before_prepare= nullptr;
	llvm::BasicBlock* first_block_after_coroutine_blocks= nullptr;
	llvm::BasicBlock* suspend= nullptr;
	llvm::BasicBlock* suspend_final= nullptr;
	llvm::SmallVector<llvm::BasicBlock*, 8> other_coroutine_blocks;
	llvm::BasicBlock* cleanup= nullptr;
	llvm::SmallVector<llvm::BasicBlock*, 8> cleanup_predecessors; // Excluding suspend_final.
	llvm::Value* promise= nullptr;
};

std::optional<CoroutineBlocks> CollectCoroutineBlocks( llvm::Function& function )
{
	CoroutineBlocks result;
	llvm::BasicBlock* prepare_block= nullptr;
	for( llvm::BasicBlock& basic_block : function.getBasicBlockList() )
	{
		const auto instructions_it= basic_block.begin();
		const auto instructions_end= basic_block.end();
		if( instructions_it != instructions_end )
		{
			llvm::Instruction& instruction= *instructions_it;
			if( instruction.getMetadata( "u_coro_block_prepare" ) != nullptr )
			{
				prepare_block= &basic_block;
				result.other_coroutine_blocks.push_back( &basic_block );
				result.block_before_prepare= basic_block.getSinglePredecessor();
				std::cout << "Found block before prepare: " << result.block_before_prepare->getName().str() << std::endl;
			}
			else if( instruction.getMetadata( "u_coro_block_begin" ) != nullptr )
			{
				result.other_coroutine_blocks.push_back( &basic_block );
				result.first_block_after_coroutine_blocks= basic_block.getSingleSuccessor();
				std::cout << "Found block after coroutine blocks: " << result.first_block_after_coroutine_blocks->getName().str() << std::endl;
			}
			else if( instruction.getMetadata( "u_coro_block_suspend" ) != nullptr )
			{
				result.suspend= &basic_block;
				std::cout << "Found suspend block: " << result.suspend->getName().str() << std::endl;
			}
			else if( instruction.getMetadata( "u_coro_block_suspend_final" ) != nullptr )
			{
				result.suspend_final= &basic_block;
				std::cout << "Found suspend final block: " << result.suspend_final->getName().str() << std::endl;
			}
			else if( instruction.getMetadata( "u_coro_block" ) != nullptr )
			{
				result.other_coroutine_blocks.push_back( &basic_block );
				std::cout << "Found other coroutine block: " << basic_block.getName().str() << std::endl;
			}
			else if( instruction.getMetadata( "u_coro_block_cleanup" ) != nullptr )
			{
				result.cleanup= &basic_block;
			}
		}
	}

	for( llvm::pred_iterator it= llvm::pred_begin(result.cleanup), it_end= llvm::pred_end(result.cleanup); it != it_end; ++it )
	{
		llvm::BasicBlock* bb= *it;
		if( bb != result.suspend_final )
		{
			std::cout << "Found cleanup redecessor: " << bb->getName().str() << std::endl;
			result.cleanup_predecessors.push_back(bb);
		}
	}

	if( prepare_block != nullptr )
	{
		for( llvm::Instruction& instruction : *prepare_block )
		{
			if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( &instruction ) )
			{
				if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
				{
					if( callee_function->getIntrinsicID() == llvm::Intrinsic::coro_id )
					{
						result.promise= call_instruction->getOperand(1u);
						std::cout << "Find promise : " << result.promise->getName().str() << std::endl;
					}
				}
			}
		}
	}

	result.allocations_block= &*function.begin();

	if( result.allocations_block == nullptr ||
		result.block_before_prepare == nullptr ||
		result.first_block_after_coroutine_blocks == nullptr ||
		result.suspend == nullptr ||
		result.suspend_final == nullptr ||
		result.cleanup == nullptr ||
		result.promise == nullptr )
	{
		std::cout << "Can't find some of coroutine blocks!" << std::endl;
		return std::nullopt;
	}

	std::cout << "Susscessfully found coroutine blocks" << std::endl;

	return result;
}

void BypassInlinedFunctionCoroutineBlocks( const CoroutineBlocks& blocks)
{
	// Create break from the start block to the first block after coroutine blocks.

	const auto terminator= blocks.block_before_prepare->getTerminator();
	U_ASSERT( terminator != nullptr );

	const auto br= llvm::BranchInst::Create( blocks.first_block_after_coroutine_blocks, blocks.block_before_prepare );

	terminator->replaceAllUsesWith( br );
	terminator->eraseFromParent();
}

struct SuspedPoint
{
	llvm::BasicBlock* suspend_block= nullptr;
	llvm::BasicBlock* normal_block= nullptr;
	llvm::BasicBlock* destroy_block= nullptr;
};

SuspedPoint ParseSuspendBlock( llvm::BasicBlock& block )
{
	SuspedPoint result;

	auto it= block.begin();
	const auto it_end= block.end();

	if( it == it_end )
	{
		std::cout << "Invalid initial suspend block" << std::endl;
		return result;
	}

	if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( &*it ) )
	{
		if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
		{
			if( callee_function->getIntrinsicID() == llvm::Intrinsic::coro_suspend )
			{}
			else
			{
				std::cout << "expected suspend call, find another function call: " << callee_function->getName().str() << std::endl;
				std::cout << "Note, bb is: " << block.getName().str() << std::endl;
				return result;
			}
		}
		else
		{
			std::cout << "expected suspend call, find non-function call" << std::endl;
			return result;
		}
	}
	else
	{
		std::cout << "expected suspend call, foud non-call" << std::endl;
		return result;
	}

	++it;
	if( it == it_end )
	{
		std::cout << "Invalid initial suspend block" << std::endl;
		return result;
	}
	if( const auto switch_instruction= llvm::dyn_cast<llvm::SwitchInst>( &*it ) )
	{
		if( switch_instruction->getNumCases() != 2 )
		{
			std::cout << "Invalid suspend switch" << std::endl;
			return result;
		}

		result.suspend_block= switch_instruction->getDefaultDest();
		result.normal_block= switch_instruction->getSuccessor(1);
		result.destroy_block= switch_instruction->getSuccessor(2);
	}
	else
	{

		std::cout << "expected switch instruction" << std::endl;
		return result;
	}

	return result;
}

// Destination should have at least two blocks.
void InlineAllocationsBlock( llvm::Function& destination, llvm::BasicBlock& alloca_block )
{
	// Try to insert allocations block before allocations block of the current function.
	alloca_block.removeFromParent();
	alloca_block.setName( alloca_block.getName() + "_inlined" );

	auto it= destination.begin();
	const auto it_end= destination.end();
	if( it == it_end )
		alloca_block.insertInto( &destination );
	else
	{
		llvm::BasicBlock* current_start_block= &*it;
		alloca_block.insertInto( &destination, current_start_block );

		const auto terminator= alloca_block.getTerminator();
		U_ASSERT( terminator != nullptr );
		llvm::BranchInst::Create( current_start_block, &alloca_block );
		terminator->eraseFromParent();
	}
}

void InlineAsyncFunctionCallItself( llvm::CallInst& call_instruction, llvm::Function& inlined_function, llvm::BasicBlock& first_block_after_coroutine_blocks )
{
	// Assume that blocks in ther suource function are located in control flow order.
	// So, we just extract all blocks up to given.

	// Collect blocks.
	llvm::SmallVector<llvm::BasicBlock*, 16> blocks_to_inline;
	for( llvm::BasicBlock& basic_block : inlined_function )
	{
		if( &basic_block == &first_block_after_coroutine_blocks )
			break;
		blocks_to_inline.push_back( &basic_block );

		basic_block.setName( basic_block.getName() + "_inlined" );
	}

	llvm::BasicBlock* const call_instruction_original_bb= call_instruction.getParent();
	llvm::BasicBlock* const block_rest_after_call= call_instruction_original_bb->splitBasicBlock( &call_instruction, "after_async_call_inlined" );

	block_rest_after_call->replaceAllUsesWith( blocks_to_inline.front() );

	// Remove them from the source function and insert into destination.
	for( auto it= blocks_to_inline.rbegin(); it != blocks_to_inline.rend(); ++it )
		(*it)->moveAfter( call_instruction_original_bb );

	// For now replace call result with undef value.
	const auto undef= llvm::UndefValue::get( call_instruction.getType() );
	call_instruction.replaceAllUsesWith( undef );
	call_instruction.eraseFromParent();

	// This blocks should contain only initial suspend. Remove it.
	first_block_after_coroutine_blocks.replaceAllUsesWith( block_rest_after_call );
	first_block_after_coroutine_blocks.eraseFromParent();
}

void ReplaceAwaitLoopBlock(
	const CoroutineBlocks& destination_function_blocks,
	llvm::BasicBlock& await_loop_block,
	const AwaitLoopBlock& await_loop_block_parsed,
	const SuspedPoint& initial_suspend_point,
	const CoroutineBlocks& inlined_function_blocks,
	llvm::Function& inlined_function )
{
	// Collect source blocks.
	llvm::SmallVector<llvm::BasicBlock*, 16> blocks_to_inline;
	for( llvm::BasicBlock& basic_block : inlined_function )
	{
		blocks_to_inline.push_back( &basic_block );
		basic_block.setName( basic_block.getName() + "_inlined" );
	}

	// Remove them from the source function and insert into destination.
	for( auto it= blocks_to_inline.rbegin(); it != blocks_to_inline.rend(); ++it )
		(*it)->moveAfter( &await_loop_block );

	(void)await_loop_block_parsed;
	(void)initial_suspend_point;
	(void)inlined_function_blocks;

	inlined_function_blocks.suspend->replaceAllUsesWith( destination_function_blocks.suspend );
	inlined_function_blocks.suspend_final->replaceAllUsesWith( await_loop_block_parsed.done_block );

	const SuspedPoint await_loop_suspend_point= ParseSuspendBlock( *await_loop_block_parsed.not_done_block );

	std::cout << "Use await loop destroy block: " << await_loop_suspend_point.destroy_block->getName().str() << std::endl;
	inlined_function_blocks.cleanup->replaceAllUsesWith( await_loop_suspend_point.destroy_block );

	// Jump to normal block of initial suspend point of the inlined function, instead of intering await loop.
	await_loop_block.replaceAllUsesWith( initial_suspend_point.normal_block );
	await_loop_block.eraseFromParent();

	initial_suspend_point.destroy_block->eraseFromParent(); // It is unreachable.

	// Not done block (which triggers suspend and goes to await block) is not needed anymore.
	await_loop_block_parsed.not_done_block->eraseFromParent();
}

void ReplacePromiseCalls( const llvm::ArrayRef<llvm::Instruction*> promise_calls, llvm::Value& value_for_replacement )
{
	for( llvm::Instruction* const call : promise_calls )
	{
		call->replaceAllUsesWith( &value_for_replacement );
		call->eraseFromParent();
	}
}

void ReplaceDoneCalls( const llvm::ArrayRef<llvm::Instruction*> done_calls )
{
	for( llvm::Instruction* const call : done_calls )
	{
		// Assume we have only one "done" call - at the start of the "await" operator.
		// We can replace it with "false".
		llvm::LLVMContext& context= call->getContext();
		call->replaceAllUsesWith( llvm::ConstantInt::getFalse( context ) );
		call->eraseFromParent();
	}
}

void RemoveCoroHandleLoad( llvm::LoadInst& coro_handle_load )
{
	// At this moment there should be no users of this instruciton.
	coro_handle_load.eraseFromParent();
}

void RemoveAllOperationsWithCoroutineObject( llvm::AllocaInst& coroutine_object )
{
	// Assume that at this moment nobody uses results of operations over this coroutine obect.
	llvm::SmallVector<llvm::Instruction*, 8> users;
	for( llvm::User* const user : coroutine_object.users() )
	{
		if( const auto instruction= llvm::dyn_cast<llvm::Instruction>(user) )
			users.push_back( instruction );
	}

	for( llvm::Instruction* const instruction : users )
		instruction->eraseFromParent();

	coroutine_object.eraseFromParent();
}

void RemoveLeftoverInlinedCoroutineBlocks( const CoroutineBlocks& coroutine_blocks )
{
	const auto module= coroutine_blocks.cleanup->getParent()->getParent();

	const auto sink_function= llvm::Function::Create(
		llvm::FunctionType::get( llvm::Type::getVoidTy( module->getContext() ), false ),
		llvm::Function::ExternalLinkage,
		"sink",
		module );

	coroutine_blocks.cleanup->setName( "cleanup_garbage" );
	coroutine_blocks.cleanup->removeFromParent();
	coroutine_blocks.cleanup->insertInto( sink_function );

	coroutine_blocks.suspend->setName( "suspend_garbage" );
	coroutine_blocks.suspend->removeFromParent();
	coroutine_blocks.suspend->insertInto( sink_function );

	coroutine_blocks.suspend_final->setName( "suspend_final_garbage" );
	coroutine_blocks.suspend_final->removeFromParent();
	coroutine_blocks.suspend_final->insertInto( sink_function );

	for( llvm::BasicBlock* const other_block : coroutine_blocks.other_coroutine_blocks )
	{
		other_block->setName( other_block->getName() + "_garbage" );
		other_block->removeFromParent();
		other_block->insertInto( sink_function );
	}

	sink_function->eraseFromParent();
}

void TryToInlineAsyncCall( llvm::Function& function, llvm::CallInst& call_instruction )
{
	llvm::AllocaInst* const coroutine_object= GetCoroutineObject( call_instruction );
	if( coroutine_object == nullptr )
		return;
	std::cout << "Work with coroutine object " << coroutine_object->getName().str() << std::endl;

	// Now we need to ensure that this coroutine object is used only in single "await" operator.
	const auto await_instructions= GetAwaitOperatorCoroutineInstructions( *coroutine_object );
	if( await_instructions == std::nullopt )
		return;
	std::cout << "Find single await for given coroutine object" << std::endl;

	const auto await_loop_block= GetAwaitLoopBlock( *await_instructions->coro_handle_load );
	if( await_loop_block == nullptr )
		return;

	llvm::SmallVector<llvm::Instruction*, 4> promise_calls;
	CollectPromiseCalls( *await_instructions->coro_handle_load, promise_calls );
	if( promise_calls.empty() )
	{
		std::cout << "Can't find any promise call" << std::endl;
		return;
	}
	std::cout << "Find " << promise_calls.size() << " promise calls" << std::endl;

	llvm::SmallVector<llvm::Instruction*, 4> done_calls;
	CollectDoneCalls( *await_instructions->coro_handle_load, done_calls );
	if( done_calls.empty() )
	{
		std::cout << "Can't find any done call" << std::endl;
		return;
	}
	std::cout << "Find " << done_calls.size() << " done calls" << std::endl;

	const auto await_loop_block_parsed= ParseAwaitLoopBlock( *await_loop_block );
	if( await_loop_block_parsed == std::nullopt )
		return;

	const auto destination_coroutine_blocks= CollectCoroutineBlocks( function );
	if( destination_coroutine_blocks == std::nullopt )
		return;

	// Clone callee and replace call to original with call to its clone.
	// This is needed later for taking instructions and basic blocks from the clone and placing them into this function.
	llvm::Function* const callee_clone= CreateCalleeAsyncFunctionClone( call_instruction );
	call_instruction.setCalledFunction( callee_clone );

	const auto source_coroutine_blocks= CollectCoroutineBlocks( *callee_clone );
	if( source_coroutine_blocks == std::nullopt )
		return;

	const SuspedPoint initial_suspend_point= ParseSuspendBlock( *source_coroutine_blocks->first_block_after_coroutine_blocks );

	std::cout << "Tranform cloned inlined function" << std::endl;

	ReplaceDoneCalls( done_calls );
	InlineAllocationsBlock( function, *source_coroutine_blocks->allocations_block );
	BypassInlinedFunctionCoroutineBlocks( *source_coroutine_blocks );
	InlineAsyncFunctionCallItself( call_instruction, *callee_clone, *source_coroutine_blocks->first_block_after_coroutine_blocks );
	ReplaceAwaitLoopBlock( *destination_coroutine_blocks, *await_loop_block, *await_loop_block_parsed, initial_suspend_point, *source_coroutine_blocks, *callee_clone );
	RemoveLeftoverInlinedCoroutineBlocks( *source_coroutine_blocks );
	ReplacePromiseCalls( promise_calls, *source_coroutine_blocks->promise );
	RemoveCoroHandleLoad( *await_instructions->coro_handle_load );
	RemoveAllOperationsWithCoroutineObject( *coroutine_object );

	// Erase temporary inlined function clone, since all basic blocks were moved into the destination.
	callee_clone->eraseFromParent();
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
