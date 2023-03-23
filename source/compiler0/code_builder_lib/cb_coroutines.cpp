#include "code_builder.hpp"

namespace U
{

Type CodeBuilder::GetGeneratorFunctionReturnType( const FunctionType& generator_function_type )
{
	// TODO - create specail class type.
	(void)generator_function_type;

	RawPointerType raw_pointer_type;
	raw_pointer_type.element_type= FundamentalType( U_FundamentalType::byte8_, fundamental_llvm_types_.byte8_ );
	raw_pointer_type.llvm_type= llvm::PointerType::get( llvm_context_, 0 );

	return std::move( raw_pointer_type );
}

void CodeBuilder::CreateGeneratorEntryBlock( FunctionContext& function_context )
{
	llvm::Value* const null= llvm::ConstantPointerNull::get( llvm::PointerType::get( llvm_context_, 0 ) );

	llvm::Value* const coro_id= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_id ),
		{
			llvm::ConstantInt::get( llvm_context_, llvm::APInt( 32u, uint64_t(0) ) ),
			null,
			null,
			null,
		},
		"coro_id" );

	llvm::Value* const coro_frame_size= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_size, { fundamental_llvm_types_.int_ptr } ),
		{},
		"coro_frame_size" );

	if( false )
	{
		llvm::Value* const coro_frame_align= function_context.llvm_ir_builder.CreateCall(
			llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_align, { fundamental_llvm_types_.int_ptr } ),
			{},
			"coro_frame_align" );
		(void)coro_frame_align;
	}

	llvm::Value* const coro_frame_memory= function_context.llvm_ir_builder.CreateCall(
		coro_.malloc,
		{ coro_frame_size },
		"coro_frame_memory" );

	llvm::Value* const coro_handle= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_begin ),
		{ coro_id, coro_frame_memory },
		"coro_handle" );

	function_context.coro_id= coro_id;
	function_context.coro_handle= coro_handle;

	function_context.coro_cleanup_bb= llvm::BasicBlock::Create( llvm_context_, "coro_cleanup" );
	function_context.coro_suspend_bb= llvm::BasicBlock::Create( llvm_context_, "coro_suspend" );
}

void CodeBuilder::GeneratorSuspend( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc )
{
	llvm::Value* const suspend_value= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_suspend ),
		{ llvm::ConstantTokenNone::get( llvm_context_ ), llvm::ConstantInt::getFalse( llvm_context_ ) },
		"suspend_value" );

	llvm::BasicBlock* const next_block= llvm::BasicBlock::Create( llvm_context_, "suspend_normal" );
	llvm::BasicBlock* const destroy_block= llvm::BasicBlock::Create( llvm_context_, "suspend_destroy" );

	llvm::SwitchInst* const switch_instr= function_context.llvm_ir_builder.CreateSwitch( suspend_value, function_context.coro_suspend_bb, 2 );
	switch_instr->addCase( llvm::ConstantInt::get( fundamental_llvm_types_.i8_, 0u, false ), next_block );
	switch_instr->addCase( llvm::ConstantInt::get( fundamental_llvm_types_.i8_, 1u, false ), destroy_block );

	function_context.function->getBasicBlockList().push_back( destroy_block );
	function_context.llvm_ir_builder.SetInsertPoint( destroy_block );
	{
		ReferencesGraph references_graph= function_context.variables_state;
		CallDestructorsBeforeReturn( names_scope, function_context, src_loc );
		function_context.variables_state= std::move(references_graph);
	}
	function_context.llvm_ir_builder.CreateBr( function_context.coro_cleanup_bb );

	function_context.function->getBasicBlockList().push_back( next_block );
	function_context.llvm_ir_builder.SetInsertPoint( next_block );
}

void CodeBuilder::CreateGeneratorEndBlock( FunctionContext& function_context )
{
	// End suspention point
	llvm::Value* const final_suspend_value= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_suspend ),
		{ llvm::ConstantTokenNone::get( llvm_context_ ), llvm::ConstantInt::getTrue( llvm_context_ ) },
		"final_suspend_value" );

	llvm::SwitchInst* const switch_instr= function_context.llvm_ir_builder.CreateSwitch( final_suspend_value, function_context.coro_suspend_bb, 2 );
	// It's undefined behaviour to resume coroutine from final state. So, use break to cleanup basic block for case of such impossible resume.
	switch_instr->addCase( llvm::ConstantInt::get( fundamental_llvm_types_.i8_, 0u, false ), function_context.coro_cleanup_bb );
	switch_instr->addCase( llvm::ConstantInt::get( fundamental_llvm_types_.i8_, 1u, false ), function_context.coro_cleanup_bb );

	// Cleanup block.

	function_context.function->getBasicBlockList().push_back( function_context.coro_cleanup_bb );
	function_context.llvm_ir_builder.SetInsertPoint( function_context.coro_cleanup_bb );

	llvm::Value* const mem_for_free= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_free ),
		{ function_context.coro_id, function_context.coro_handle },
		"coro_frame_memory_for_free" );

	function_context.llvm_ir_builder.CreateCall( coro_.free, { mem_for_free } );
	function_context.llvm_ir_builder.CreateBr( function_context.coro_suspend_bb );

	// Suspend block.
	function_context.function->getBasicBlockList().push_back( function_context.coro_suspend_bb );
	function_context.llvm_ir_builder.SetInsertPoint( function_context.coro_suspend_bb );

	function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_end ),
		{ function_context.coro_handle, llvm::ConstantInt::getFalse( llvm_context_ ) } );

	function_context.llvm_ir_builder.CreateRet( function_context.coro_handle );
}

} // namespace U
