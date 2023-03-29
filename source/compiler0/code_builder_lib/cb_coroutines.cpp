#include "keywords.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "code_builder.hpp"

namespace U
{

Type CodeBuilder::GetGeneratorFunctionReturnType( NamesScope& root_namespace, const FunctionType& generator_function_type )
{
	CoroutineTypeDescription coroutine_type_description;
	coroutine_type_description.kind= CoroutineKind::Generator;
	coroutine_type_description.return_type= generator_function_type.return_type;
	coroutine_type_description.return_value_type= generator_function_type.return_value_type;
	coroutine_type_description.inner_reference_type= InnerReferenceType::None;
	for( const FunctionType::Param& param : generator_function_type.params )
	{
		if( param.value_type == ValueType::ReferenceMut )
			coroutine_type_description.inner_reference_type= InnerReferenceType::Mut;
		else if( param.value_type == ValueType::ReferenceImut && coroutine_type_description.inner_reference_type == InnerReferenceType::None )
			coroutine_type_description.inner_reference_type= InnerReferenceType::Imut;
	}

	return GetCoroutineType( root_namespace, coroutine_type_description );
}

Type CodeBuilder::GetCoroutineType( NamesScope& root_namespace, const CoroutineTypeDescription& coroutine_type_description )
{
	if( const auto it= coroutine_classes_table_.find( coroutine_type_description ); it != coroutine_classes_table_.end() )
		return it->second.get();

	auto coroutine_class= std::make_unique<Class>( Keyword( Keywords::generator_ ), &root_namespace );

	coroutine_class->coroutine_type_description= coroutine_type_description;
	coroutine_class->members->SetClass( coroutine_class.get() );
	coroutine_class->parents_list_prepared= true;
	coroutine_class->is_default_constructible= false;
	coroutine_class->is_copy_constructible= false;
	coroutine_class->have_destructor= true;
	coroutine_class->is_copy_assignable= false;
	coroutine_class->is_equality_comparable= false; // TDO - maybe implement == operator?
	coroutine_class->can_be_constexpr= false; // TODO - make "constexpr" depending on return type.

	llvm::Type* const handle_type= llvm::PointerType::get( llvm_context_, 0 );

	coroutine_class->llvm_type=
		llvm::StructType::create(
			llvm_context_,
			llvm::ArrayRef<llvm::Type*>{ handle_type },
			mangler_->MangleType( coroutine_class.get() ) );
	coroutine_class->is_complete= true;

	// Generate destructor.
	{
		FunctionVariable destructor_variable= GenerateDestructorPrototype( coroutine_class.get() );
		destructor_variable.have_body= true;
		llvm::Function* const destructor_function= EnsureLLVMFunctionCreated( destructor_variable );

		OverloadedFunctionsSetPtr functions_set= std::make_shared<OverloadedFunctionsSet>();
		functions_set->functions.push_back( std::move( destructor_variable ) );
		coroutine_class->members->AddName( Keyword( Keywords::destructor_ ), std::move( functions_set ) );

		const auto bb= llvm::BasicBlock::Create( llvm_context_, "func_code", destructor_function );
		llvm::IRBuilder<> ir_builder( bb );

		llvm::Argument* const this_arg= destructor_function->getArg(0);
		this_arg->setName( Keyword( Keywords::this_ ) );
		llvm::Value* const coro_handle= ir_builder.CreateLoad( handle_type, this_arg, false, "coro_handle" );

		ir_builder.CreateCall(
			llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_destroy ),
			{ coro_handle} );

		ir_builder.CreateRetVoid();
	}

	const ClassPtr res_type= coroutine_class.get();
	coroutine_classes_table_[coroutine_type_description]= std::move(coroutine_class);
	return res_type;
}

void CodeBuilder::CreateGeneratorEntryBlock( FunctionContext& function_context )
{
	llvm::PointerType* const pointer_type= llvm::PointerType::get( llvm_context_, 0 );

	const ClassPtr coroutine_class= function_context.return_type->GetClassType();
	U_ASSERT( coroutine_class != nullptr );
	U_ASSERT( coroutine_class->coroutine_type_description != std::nullopt );
	llvm::Type* const promise_type=
		coroutine_class->coroutine_type_description->return_value_type == ValueType::Value
		? coroutine_class->coroutine_type_description->return_type.GetLLVMType()
		: pointer_type;

	function_context.llvm_ir_builder.GetInsertBlock()->setName( "coro_prepare" );

	// Yes, create "alloca" not in "alloca" block. It is safe to do such here.
	llvm::Value* const promise= function_context.llvm_ir_builder.CreateAlloca( promise_type, nullptr, "coro_promise" );

	U_ASSERT( function_context.s_ret_ == nullptr );
	function_context.s_ret_= promise;

	llvm::Value* const null= llvm::ConstantPointerNull::get( pointer_type );

	llvm::Value* const coro_id= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_id ),
		{
			llvm::ConstantInt::get( llvm_context_, llvm::APInt( 32u, uint64_t(0) ) ),
			promise,
			null,
			null,
		},
		"coro_id" );

	llvm::Value* const coro_need_to_alloc= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_alloc ),
		{
			coro_id,
		},
		"coro_need_to_alloc" );

	llvm::BasicBlock* const coro_need_to_alloc_check_block= function_context.llvm_ir_builder.GetInsertBlock();

	const auto block_need_to_alloc= llvm::BasicBlock::Create( llvm_context_, "need_to_alloc" );
	const auto block_coro_begin= llvm::BasicBlock::Create( llvm_context_, "coro_begin" );

	function_context.llvm_ir_builder.CreateCondBr( coro_need_to_alloc, block_need_to_alloc, block_coro_begin );

	function_context.function->getBasicBlockList().push_back( block_need_to_alloc );
	function_context.llvm_ir_builder.SetInsertPoint( block_need_to_alloc );

	llvm::Value* const coro_frame_size= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_size, { fundamental_llvm_types_.int_ptr } ),
		{},
		"coro_frame_size" );

	llvm::Value* const coro_frame_memory_allocated= function_context.llvm_ir_builder.CreateCall(
		coro_.malloc,
		{ coro_frame_size },
		"coro_frame_memory_allocated" );

	function_context.llvm_ir_builder.CreateBr( block_coro_begin );

	function_context.function->getBasicBlockList().push_back( block_coro_begin );
	function_context.llvm_ir_builder.SetInsertPoint( block_coro_begin );
	llvm::PHINode* const coro_frame_memory= function_context.llvm_ir_builder.CreatePHI( pointer_type, 2, "coro_frame_memory" );
	coro_frame_memory->addIncoming( null, coro_need_to_alloc_check_block );
	coro_frame_memory->addIncoming( coro_frame_memory_allocated, block_need_to_alloc );

	llvm::Value* const coro_handle= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_begin ),
		{ coro_id, coro_frame_memory },
		"coro_handle" );

	function_context.coro_suspend_bb= llvm::BasicBlock::Create( llvm_context_, "coro_suspend" );

	const auto func_code_block= llvm::BasicBlock::Create( llvm_context_, "func_code" );
	function_context.llvm_ir_builder.CreateBr( func_code_block );

	// Cleanup block.
	function_context.coro_cleanup_bb= llvm::BasicBlock::Create( llvm_context_, "coro_cleanup" );
	function_context.function->getBasicBlockList().push_back( function_context.coro_cleanup_bb );
	function_context.llvm_ir_builder.SetInsertPoint( function_context.coro_cleanup_bb );

	llvm::Value* const mem_for_free= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_free ),
		{ coro_id, coro_handle },
		"coro_frame_memory_for_free" );

	llvm::Value* const need_to_free=
		function_context.llvm_ir_builder.CreateICmpNE(
			mem_for_free,
			llvm::ConstantPointerNull::get( llvm::PointerType::get( llvm_context_, 0 ) ),
			"coro_need_to_free" );

	const auto block_need_to_free= llvm::BasicBlock::Create( llvm_context_, "need_to_free" );
	function_context.llvm_ir_builder.CreateCondBr( need_to_free, block_need_to_free, function_context.coro_suspend_bb );

	function_context.function->getBasicBlockList().push_back( block_need_to_free );
	function_context.llvm_ir_builder.SetInsertPoint( block_need_to_free );
	function_context.llvm_ir_builder.CreateCall( coro_.free, { mem_for_free } );
	function_context.llvm_ir_builder.CreateBr( function_context.coro_suspend_bb );

	// Suspend block.
	function_context.function->getBasicBlockList().push_back( function_context.coro_suspend_bb );
	function_context.llvm_ir_builder.SetInsertPoint( function_context.coro_suspend_bb );

	function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_end ),
		{ coro_handle, llvm::ConstantInt::getFalse( llvm_context_ ) } );

	function_context.llvm_ir_builder.CreateRet( coro_handle );

	// End suspention point.
	function_context.coro_final_suspend_bb= llvm::BasicBlock::Create( llvm_context_, "coro_suspend_final" );
	function_context.function->getBasicBlockList().push_back( function_context.coro_final_suspend_bb );
	function_context.llvm_ir_builder.SetInsertPoint( function_context.coro_final_suspend_bb );

	llvm::Value* const final_suspend_value= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_suspend ),
		{ llvm::ConstantTokenNone::get( llvm_context_ ), llvm::ConstantInt::getTrue( llvm_context_ ) },
		"final_suspend_value" );

	const auto unreachable_block= llvm::BasicBlock::Create( llvm_context_, "coro_final_suspend_unreachable" );

	llvm::SwitchInst* const switch_instr= function_context.llvm_ir_builder.CreateSwitch( final_suspend_value, function_context.coro_suspend_bb, 2 );
	switch_instr->addCase( llvm::ConstantInt::get( fundamental_llvm_types_.i8_, 0u, false ), unreachable_block );
	switch_instr->addCase( llvm::ConstantInt::get( fundamental_llvm_types_.i8_, 1u, false ), function_context.coro_cleanup_bb );

	// Final suspend unreachable block.
	// It's undefined behaviour to resume coroutine in final suspention state. So, just add unreachable instruction here.
	function_context.function->getBasicBlockList().push_back( unreachable_block );
	function_context.llvm_ir_builder.SetInsertPoint( unreachable_block );
	function_context.llvm_ir_builder.CreateUnreachable();

	// Block for further function code.
	function_context.function->getBasicBlockList().push_back( func_code_block );
	function_context.llvm_ir_builder.SetInsertPoint( func_code_block );
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

void CodeBuilder::GeneratorFinalSuspend( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc )
{
	// We can destroy all local variables right now. Leave only coroutine cleanup code in destroy block.
	{
		ReferencesGraph references_graph= function_context.variables_state;
		CallDestructorsBeforeReturn( names_scope, function_context, src_loc );
		function_context.variables_state= std::move(references_graph);
	}

	function_context.llvm_ir_builder.CreateBr( function_context.coro_final_suspend_bb );
}

} // namespace U
