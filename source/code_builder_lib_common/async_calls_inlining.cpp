#include <iostream>
#include <optional>
#include <unordered_map>

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

// This function may still return "false" is some indirect recursive call and recursive call via other function exist.
bool FunctionIsDirectlyRecursive( llvm::Function& function )
{
	for( llvm::BasicBlock& basic_block : function.getBasicBlockList() )
		for( llvm::Instruction& instruction : basic_block.getInstList() )
			if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( &instruction ) )
				if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
					if( callee_function == &function )
						return true;

	return false;
}

void ExtractAllACoroutineFunctionCalls( llvm::Function& function, llvm::SmallVectorImpl<llvm::CallInst*>& out )
{
	for( llvm::BasicBlock& basic_block : function.getBasicBlockList() )
	{
		for( llvm::Instruction& instruction : basic_block.getInstList() )
		{
			if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( &instruction ) )
				if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
					if( callee_function->hasFnAttribute( llvm::Attribute::PresplitCoroutine ) )
						out.push_back( call_instruction );
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

// Returns a result only if coroutine object used for single await operator and nothing else.
std::optional<AwaitOperatorCoroutineInstructions> GetAwaitOperatorCoroutineInstructions( llvm::AllocaInst& coroutine_object )
{
	AwaitOperatorCoroutineInstructions res;
	llvm::StoreInst* initial_store_instruction= nullptr;

	llvm::Function* destructor_function= nullptr;
	llvm::SmallVector<llvm::Function*, 16> additional_calls;

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

				if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
					destructor_function= callee_function;
			}
			else
			{
				if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
				{
					if( callee_function->getIntrinsicID() == llvm::Intrinsic::lifetime_start ||
						callee_function->getIntrinsicID() == llvm::Intrinsic::lifetime_end ||
						callee_function->getIntrinsicID() == llvm::Intrinsic::dbg_declare ||
						callee_function->getName() == "__U_debug_lifetime_start" ||
						callee_function->getName() == "__U_debug_lifetime_end" )
						continue; // Allow lifetime and debug instructions for the coroutine object.

					additional_calls.push_back( callee_function );
					continue;
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

	// Assume that we have single destructor call except call in at the await end - destructor call in the destruction branch inside "suspend" of "await".
	// For technical reasnos it is not marked with "u_await_destructor_call" metadata.
	if( additional_calls.size() > 1u )
	{
		std::cout << "Find calls except destructors for coroutine object" << std::endl;
		return std::nullopt;
	}
	if( destructor_function == nullptr )
	{
		std::cout << "Can't find destructor for coroutine object" << std::endl;
		return std::nullopt;
	}
	if( additional_calls.front() != destructor_function )
	{
		std::cout << "Unexpected call to non-destructor function " << additional_calls.front()->getName().str() << std::endl;
		return std::nullopt;
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
			llvm::BasicBlock* const bb= instruction->getParent();

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
	}
	else
	{
		std::cout << "expected branching" << std::endl;
		return std::nullopt;
	}

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

struct CoroutineFunctionInfo
{
	llvm::SmallVector<llvm::BasicBlock*, 4> allocations_blocks;
	llvm::BasicBlock* block_before_prepare= nullptr;
	llvm::BasicBlock* initial_suspend_block= nullptr;
	llvm::BasicBlock* suspend_block= nullptr;
	llvm::BasicBlock* suspend_final_block= nullptr;
	llvm::BasicBlock* cleanup_block= nullptr;
	llvm::SmallVector<llvm::BasicBlock*, 8> other_coroutine_blocks;
	llvm::Value* promise= nullptr;
};

std::optional<CoroutineFunctionInfo> CollectCoroutineFunctionInfo( llvm::Function& function )
{
	CoroutineFunctionInfo result;
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
			}
			else if( instruction.getMetadata( "u_coro_block_begin" ) != nullptr )
			{
				result.other_coroutine_blocks.push_back( &basic_block );
				result.initial_suspend_block= basic_block.getSingleSuccessor();
			}
			else if( instruction.getMetadata( "u_coro_block_suspend" ) != nullptr )
				result.suspend_block= &basic_block;
			else if( instruction.getMetadata( "u_coro_block_suspend_final" ) != nullptr )
				result.suspend_final_block= &basic_block;
			else if( instruction.getMetadata( "u_coro_block_cleanup" ) != nullptr )
				result.cleanup_block= &basic_block;
			else if( instruction.getMetadata( "u_coro_block" ) != nullptr )
				result.other_coroutine_blocks.push_back( &basic_block );

		}

		// Normally a function should contain single allocations block.
		// "alloca" instruction may be used only inside it.
		// But if another function already was inlined into the function, multiple allocation blocks are possible.
		bool is_alloca_block= false;
		for( const llvm::Instruction& instruction : basic_block )
		{
			if( llvm::dyn_cast<llvm::AllocaInst>(&instruction) != nullptr )
			{
				is_alloca_block= true;
				break;
			}
		}

		if( is_alloca_block )
			result.allocations_blocks.push_back( &basic_block );
	}

	if( prepare_block != nullptr )
	{
		for( llvm::Instruction& instruction : *prepare_block )
		{
			if( const auto call_instruction= llvm::dyn_cast<llvm::CallInst>( &instruction ) )
				if( const auto callee_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
					if( callee_function->getIntrinsicID() == llvm::Intrinsic::coro_id )
						result.promise= call_instruction->getOperand(1u);
		}
	}

	if( result.allocations_blocks.empty() ||
		result.block_before_prepare == nullptr ||
		result.initial_suspend_block == nullptr ||
		result.suspend_block == nullptr ||
		result.suspend_final_block == nullptr ||
		result.cleanup_block == nullptr ||
		result.promise == nullptr )
	{
		std::cout << "Can't find some of coroutine blocks!" << std::endl;
		return std::nullopt;
	}

	return result;
}

struct SuspedPoint
{
	llvm::BasicBlock* suspend_block= nullptr;
	llvm::BasicBlock* normal_block= nullptr;
	llvm::BasicBlock* destroy_block= nullptr;
};

std::optional<SuspedPoint> ParseSuspendBlock( llvm::BasicBlock& block )
{
	SuspedPoint result;

	auto it= block.begin();
	const auto it_end= block.end();

	if( it == it_end )
	{
		std::cout << "Invalid initial suspend block" << std::endl;
		return std::nullopt;
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
				return std::nullopt;
			}
		}
		else
		{
			std::cout << "expected suspend call, find non-function call" << std::endl;
			return std::nullopt;
		}
	}
	else
	{
		std::cout << "expected suspend call, foud non-call" << std::endl;
		return std::nullopt;
	}

	++it;
	if( it == it_end )
	{
		std::cout << "Invalid initial suspend block" << std::endl;
		return std::nullopt;
	}
	if( const auto switch_instruction= llvm::dyn_cast<llvm::SwitchInst>( &*it ) )
	{
		if( switch_instruction->getNumCases() != 2 )
		{
			std::cout << "Invalid suspend switch" << std::endl;
			return std::nullopt;
		}

		result.suspend_block= switch_instruction->getDefaultDest();
		result.normal_block= switch_instruction->getSuccessor(1);
		result.destroy_block= switch_instruction->getSuccessor(2);
	}
	else
	{

		std::cout << "expected switch instruction" << std::endl;
		return std::nullopt;
	}

	if( result.suspend_block == nullptr ||
		result.normal_block == nullptr ||
		result.destroy_block == nullptr )
	{
		std::cout << "Invalid suspend block" << std::endl;
		return std::nullopt;
	}

	return result;
}

bool IsAsyncFunctionCallWithSingleFurtherAwait( llvm::CallInst& call_instruction )
{
	llvm::AllocaInst* const coroutine_object= GetCoroutineObject( call_instruction );
	if( coroutine_object == nullptr )
		return false;

	return GetAwaitOperatorCoroutineInstructions( *coroutine_object ) != std::nullopt;
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

	llvm::SmallVector<llvm::Instruction*, 4> done_calls;
	CollectDoneCalls( *await_instructions->coro_handle_load, done_calls );
	if( done_calls.empty() )
	{
		std::cout << "Can't find any done call" << std::endl;
		return;
	}

	const auto await_loop_block_parsed= ParseAwaitLoopBlock( *await_loop_block );
	if( await_loop_block_parsed == std::nullopt )
		return;

	const auto await_loop_suspend_point= ParseSuspendBlock( *await_loop_block_parsed->not_done_block );
	if( await_loop_suspend_point == std::nullopt )
		return;

	const auto destination_coroutine_info= CollectCoroutineFunctionInfo( function );
	if( destination_coroutine_info == std::nullopt )
		return;

	// Clone callee and replace call to original with call to its clone.
	// This is needed later for taking instructions and basic blocks from the clone and placing them into this function.
	llvm::Function* const callee_clone= CreateCalleeAsyncFunctionClone( call_instruction );
	call_instruction.setCalledFunction( callee_clone );

	const auto source_coroutine_info= CollectCoroutineFunctionInfo( *callee_clone );
	if( source_coroutine_info == std::nullopt )
	{
		callee_clone->eraseFromParent();
		return;
	}

	const auto source_initial_suspend_point= ParseSuspendBlock( *source_coroutine_info->initial_suspend_block );
	if( source_initial_suspend_point == std::nullopt )
	{
		callee_clone->eraseFromParent();
		return;
	}

	std::cout << "Tranform cloned inlined function" << std::endl;

	// Replace "llvm.coro.promise" calls with promise value itself.
	for( llvm::Instruction* const call : promise_calls )
	{
		call->replaceAllUsesWith( source_coroutine_info->promise );
		call->eraseFromParent();
	}

	// Replace "llvm.core.done" calls.
	// Assume we have only one "done" call - at the start of the "await" operator.
	// We can replace it with "false".
	for( llvm::Instruction* const call : done_calls )
	{
		llvm::LLVMContext& context= call->getContext();
		call->replaceAllUsesWith( llvm::ConstantInt::getFalse( context ) );
		call->eraseFromParent();
	}

	// Replace args in inlined function with given values from the call instruction.
	{
		uint32_t i= 0;
		for( llvm::Argument& arg : callee_clone->args() )
		{
			llvm::Value* const op= call_instruction.getArgOperand(i);
			arg.replaceAllUsesWith( op );
			++i;
		}
	}

	// Insert allocation blocks before allocation blocks of the current function.
	{
		const auto it= function.begin();
		const auto it_end= function.end();
		U_ASSERT( it != it_end );
		llvm::BasicBlock* const current_start_block= &*it;

		for( llvm::BasicBlock* const alloca_block : source_coroutine_info->allocations_blocks )
		{
			alloca_block->removeFromParent();
			alloca_block->setName( alloca_block->getName() + "_inlined" );

			alloca_block->insertInto( &function, current_start_block );
		}

		// Assume that last allocation block contains "br func_code".
		// Replace it with "br" to the allocations block of this function.
		U_ASSERT( !source_coroutine_info->allocations_blocks.empty() );
		U_ASSERT( !destination_coroutine_info->allocations_blocks.empty() );

		llvm::BasicBlock* const last_alloca_block= source_coroutine_info->allocations_blocks.back();
		llvm::BasicBlock* const first_dst_alloca_block= destination_coroutine_info->allocations_blocks.front();
		U_ASSERT( first_dst_alloca_block == current_start_block );

		llvm::Instruction* const terminator= last_alloca_block->getTerminator();
		U_ASSERT( terminator != nullptr );
		llvm::BranchInst::Create( first_dst_alloca_block, last_alloca_block );
		terminator->eraseFromParent();
	}

	// Create break from the start block to the first block after coroutine blocks.
	{
		llvm::Instruction* const terminator= source_coroutine_info->block_before_prepare->getTerminator();
		U_ASSERT( terminator != nullptr );

		const auto br= llvm::BranchInst::Create( source_coroutine_info->initial_suspend_block, source_coroutine_info->block_before_prepare );

		terminator->replaceAllUsesWith(br);
		terminator->eraseFromParent();
	}

	// Inline async function call itself.
	// Assume that blocks in ther suource function are located in control flow order.
	// So, we just extract all blocks up to given.
	{
		// Collect source blocks.
		llvm::SmallVector<llvm::BasicBlock*, 32> blocks_to_inline;
		for( llvm::BasicBlock& basic_block : *callee_clone )
		{
			if( &basic_block == source_coroutine_info->initial_suspend_block )
				break;

			blocks_to_inline.push_back( &basic_block );
		}

		llvm::BasicBlock* const call_instruction_original_bb= call_instruction.getParent();
		llvm::BasicBlock* const block_rest_after_call= call_instruction_original_bb->splitBasicBlock( &call_instruction, "after_async_call_inlined" );

		block_rest_after_call->replaceAllUsesWith( blocks_to_inline.front() );

		// Remove blocks from the source function and insert into destination.
		llvm::BasicBlock* append_block= call_instruction_original_bb;
		for( llvm::BasicBlock* const block : blocks_to_inline )
		{
			block->moveAfter( append_block );
			block->setName( block->getName() + "_inlined" );
			append_block= block;
		}

		// Remove initial suspend block.
		source_coroutine_info->initial_suspend_block->replaceAllUsesWith( block_rest_after_call );
		source_coroutine_info->initial_suspend_block->eraseFromParent();
	}

	// Temporary replace call result with undef value.
	call_instruction.replaceAllUsesWith( llvm::UndefValue::get( call_instruction.getType() ) );
	call_instruction.eraseFromParent();

	// Inline remaining blocks.
	{
		// Collect source blocks.
		llvm::SmallVector<llvm::BasicBlock*, 32> blocks_to_inline;
		for( llvm::BasicBlock& basic_block : *callee_clone )
			blocks_to_inline.push_back( &basic_block );

		// Remove blocks from the source function and insert into destination.
		llvm::BasicBlock* append_block= await_loop_block;
		for( llvm::BasicBlock* const block : blocks_to_inline )
		{
			block->moveAfter( append_block );
			block->setName( block->getName() + "_inlined" );
			append_block= block;
		}
	}

	source_coroutine_info->suspend_block->replaceAllUsesWith( destination_coroutine_info->suspend_block );
	source_coroutine_info->suspend_final_block->replaceAllUsesWith( await_loop_block_parsed->done_block );
	source_coroutine_info->cleanup_block->replaceAllUsesWith( await_loop_suspend_point->destroy_block );

	// Jump to normal block of initial suspend point of the inlined function, instead of intering await loop.
	await_loop_block->replaceAllUsesWith( source_initial_suspend_point->normal_block );
	await_loop_block->eraseFromParent();

	source_initial_suspend_point->destroy_block->eraseFromParent(); // It is unreachable.

	await_loop_block_parsed->not_done_block->eraseFromParent(); // Not done block (which triggers suspend and goes to await block) is not needed anymore.
	await_loop_suspend_point->normal_block->eraseFromParent(); // It's also not reachable anymore.

	// Remove leftover inlined function blocks.
	source_coroutine_info->cleanup_block->dropAllReferences();
	source_coroutine_info->suspend_block->dropAllReferences();
	source_coroutine_info->suspend_final_block->dropAllReferences();
	for( llvm::BasicBlock* const other_block : source_coroutine_info->other_coroutine_blocks )
		other_block->dropAllReferences();

	source_coroutine_info->cleanup_block->eraseFromParent();
	source_coroutine_info->suspend_block->eraseFromParent();
	source_coroutine_info->suspend_final_block->eraseFromParent();
	for( llvm::BasicBlock* const other_block : source_coroutine_info->other_coroutine_blocks )
		other_block->eraseFromParent();

	// At this moment there should be no users of this instruciton.
	await_instructions->coro_handle_load->eraseFromParent();

	// Remove all usages of the coroutine object.
	// Assume that at this moment nobody uses results of operations over this coroutine obect.
	{
		llvm::SmallVector<llvm::Instruction*, 8> users;
		for( llvm::User* const user : coroutine_object->users() )
			if( const auto instruction= llvm::dyn_cast<llvm::Instruction>(user) )
				users.push_back( instruction );

		for( llvm::Instruction* const instruction : users )
			instruction->eraseFromParent();

		coroutine_object->eraseFromParent();
	}

	// Erase temporary inlined function clone, since all basic blocks were moved into the destination.
	callee_clone->eraseFromParent();
}

struct AsyncFunctionCall
{
	llvm::CallInst* instruction= nullptr;
	llvm::Function* function= nullptr;
};

using AsyncFunctionCalls= llvm::SmallVector<AsyncFunctionCall, 4>;

struct AsyncCallGraphNode
{
	AsyncFunctionCalls calls;
	llvm::SmallVector<llvm::Function*, 4> out_nodes;
	llvm::SmallVector<llvm::Function*, 4> in_nodes;
};

using AsyncCallsGraph= std::unordered_map<llvm::Function*, AsyncCallGraphNode>;

AsyncCallsGraph BuildAsyncCallsGraph( llvm::Module& module )
{
	AsyncCallsGraph result;
	for( llvm::Function& function : module.functions() )
	{
		// Apply the optimization only for async fnctions, that must have this attribute.
		if( !function.hasFnAttribute( llvm::Attribute::PresplitCoroutine ) )
			continue;

		llvm::SmallVector<llvm::CallInst*, 16> async_calls;
		ExtractAllACoroutineFunctionCalls( function, async_calls );

		for( llvm::CallInst* const call_instruction : async_calls )
			if( IsAsyncFunctionCallWithSingleFurtherAwait( *call_instruction ) )
			{
				if( const auto calle_function= llvm::dyn_cast<llvm::Function>( GetCallee( *call_instruction ) ) )
				{
					result[ &function ].calls.push_back( { call_instruction, calle_function } );

					result[ &function ].out_nodes.push_back( calle_function );
					result[ calle_function ].in_nodes.push_back( &function );
				}
			}
	}

	return result;
}

} // namespace

void InlineAsyncCalls( llvm::Module& module )
{
	AsyncCallsGraph async_call_graph= BuildAsyncCallsGraph( module );

	llvm::SmallVector< std::pair<llvm::Function*, AsyncFunctionCalls> , 8> inlining_order;

	// Try to build order of iteration.
	// Extract graph nodes with no output nodes until there are such nodes.
	// If the async call graph is acycled, remaining graph will be empty.
	while(true)
	{
		size_t nodes_removed= 0;
		for( auto it= async_call_graph.begin(), it_end= async_call_graph.end(); it != it_end; )
		{
			llvm::Function* const function= it->first;
			if( it->second.out_nodes.empty() )
			{
				// Remove this node.

				// Drop links to this function first.
				for( llvm::Function* const in_function : it->second.in_nodes )
				{
					const auto other_function_it= async_call_graph.find( in_function );
					U_ASSERT( other_function_it != async_call_graph.end() );

					auto& other_function_out_nodes= other_function_it->second.out_nodes;
					for( size_t i= 0; i < other_function_out_nodes.size(); )
					{
						if( other_function_out_nodes[i] == function )
						{
							if( i + 1 < other_function_out_nodes.size() )
								other_function_out_nodes[i]= std::move( other_function_out_nodes.back() );
							other_function_out_nodes.pop_back();
						}
						else
							++i;
					}
				}

				// Populate inlining order container.
				inlining_order.push_back( std::make_pair( function, std::move( it->second.calls ) ) );

				// Erase node from the graph.
				it= async_call_graph.erase(it);
				++nodes_removed;
			}
			else
				++it;
		}

		if( nodes_removed == 0 )
			break;
	}

	if( !async_call_graph.empty() )
	{
		// This graph contains cycles.
		// Perform reverse algorithm for previous one - extract nodes with no input nodes until it is possible.
		// Put extracted nodes into another container, since the order is reverse.

		llvm::SmallVector< std::pair<llvm::Function*, AsyncFunctionCalls> , 8> inlining_order_inverse;
		while(true)
		{
			size_t nodes_removed= 0;
			for( auto it= async_call_graph.begin(), it_end= async_call_graph.end(); it != it_end; )
			{
				llvm::Function* const function= it->first;
				if( it->second.in_nodes.empty() )
				{
					// Remove this node.

					// Drop links to this function first.
					for( llvm::Function* const in_function : it->second.out_nodes )
					{
						const auto other_function_it= async_call_graph.find( in_function );
						U_ASSERT( other_function_it != async_call_graph.end() );

						auto& other_function_in_nodes= other_function_it->second.in_nodes;
						for( size_t i= 0; i < other_function_in_nodes.size(); )
						{
							if( other_function_in_nodes[i] == function )
							{
								if( i + 1 < other_function_in_nodes.size() )
									other_function_in_nodes[i]= std::move( other_function_in_nodes.back() );
								other_function_in_nodes.pop_back();
							}
							else
								++i;
						}
					}

					// Populate inverse inlining order container.
					inlining_order_inverse.push_back( std::make_pair( function, std::move( it->second.calls ) ) );

					// Erase node from the graph.
					it= async_call_graph.erase(it);
					++nodes_removed;
				}
				else
					++it;
			}

			if( nodes_removed == 0 )
				break;
		}

		// Append remaining graph elements into the order container.
		// Now only strongly linked components are left, order for inlining for them is unsignificant.
		for( auto& function_pair : async_call_graph )
			inlining_order.push_back( std::make_pair( function_pair.first, std::move( function_pair.second.calls ) ) );
		async_call_graph.clear();

		// Since elements in this container are reversed, append them with inversion.
		for( auto it= inlining_order_inverse.rbegin(); it != inlining_order_inverse.rend(); ++it )
			inlining_order.push_back( std::move( *it ) );
	}

	std::cout << "Inlining order is: ";
	for( const auto& function_pair : inlining_order )
		std::cout << function_pair.first->getName().str() << ", ";
	std::cout << std::endl;

	for( const auto& function_pair : inlining_order )
	{
		for( const auto& call : function_pair.second )
		{
			if( FunctionIsDirectlyRecursive( *call.function ) )
				continue;

			TryToInlineAsyncCall( *function_pair.first, *call.instruction );
		}
	}

	for( const auto& function_pair : inlining_order )
	{
		llvm::Function* const function= function_pair.first;
		// Remove inlined function, if can do so.
		// Right now do not remove, only populate container for removal, in order to avoid removing functions while iterating over module functions.
		if( function->getLinkage() == llvm::GlobalValue::PrivateLinkage &&
			!function->hasNUsesOrMore(1) )
			function->eraseFromParent();
	}
}

} // namespace U
