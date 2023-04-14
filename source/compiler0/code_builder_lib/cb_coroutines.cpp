#include "keywords.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

ClassPtr CodeBuilder::GetGeneratorFunctionReturnType(
	NamesScope& root_namespace,
	const FunctionType& generator_function_type,
	const bool non_sync )
{
	CoroutineTypeDescription coroutine_type_description;
	coroutine_type_description.kind= CoroutineKind::Generator;
	coroutine_type_description.return_type= generator_function_type.return_type;
	coroutine_type_description.return_value_type= generator_function_type.return_value_type;
	coroutine_type_description.inner_reference_type= InnerReferenceType::None;
	coroutine_type_description.non_sync= non_sync;
	for( const FunctionType::Param& param : generator_function_type.params )
	{
		if( param.value_type == ValueType::Value )
		{
			// Require type completeness for value params in order to know inner reference kind.
			if( EnsureTypeComplete( param.type ) )
			{
				const InnerReferenceType param_type_inner_reference_type= param.type.GetInnerReferenceType();
				if( param_type_inner_reference_type == InnerReferenceType::Mut )
					coroutine_type_description.inner_reference_type= InnerReferenceType::Mut;
				else if( param_type_inner_reference_type == InnerReferenceType::Imut && coroutine_type_description.inner_reference_type == InnerReferenceType::None )
					coroutine_type_description.inner_reference_type= InnerReferenceType::Imut;
			}
		}
		else
		{
			// Assume this is a reference to type with no references inside.
			// This is checked later - when building function code.
			// Do this later in order to avoid full type building for reference params.
			if( param.value_type == ValueType::ReferenceMut )
				coroutine_type_description.inner_reference_type= InnerReferenceType::Mut;
			else if( param.value_type == ValueType::ReferenceImut && coroutine_type_description.inner_reference_type == InnerReferenceType::None )
				coroutine_type_description.inner_reference_type= InnerReferenceType::Imut;
		}
	}

	return GetCoroutineType( root_namespace, coroutine_type_description );
}

std::set<FunctionType::ParamReference> CodeBuilder::GetGeneratorFunctionReturnReferences( const FunctionType& generator_function_type )
{
	std::set<FunctionType::ParamReference> result;
	for( const FunctionType::Param& param : generator_function_type.params )
	{
		const size_t i= size_t(&param - generator_function_type.params.data());
		if( param.value_type == ValueType::Value )
		{
			// Assume, that value can have a reference inside. If it has no reference inside - this is not a problem.
			// TODO - maybe check real inner reference kind here?
			FunctionType::ParamReference param_reference{ uint8_t(i), uint8_t(0) };
			result.insert( param_reference );
		}
		else
		{
			// Assume, that generator function returns a generator, which internal node points to all reference args.
			FunctionType::ParamReference param_reference{ uint8_t(i), FunctionType::c_arg_reference_tag_number };
			result.insert( param_reference );
		}
	}

	return result;
}

ClassPtr CodeBuilder::GetCoroutineType( NamesScope& root_namespace, const CoroutineTypeDescription& coroutine_type_description )
{
	if( const auto it= coroutine_classes_table_.find( coroutine_type_description ); it != coroutine_classes_table_.end() )
		return it->second.get();

	auto coroutine_class= std::make_unique<Class>( Keyword( Keywords::generator_ ), &root_namespace );
	const ClassPtr res_type= coroutine_class.get();

	coroutine_class->coroutine_type_description= coroutine_type_description;
	coroutine_class->inner_reference_type= coroutine_type_description.inner_reference_type;
	coroutine_class->members->SetClass( coroutine_class.get() );
	coroutine_class->parents_list_prepared= true;
	coroutine_class->is_default_constructible= false;
	coroutine_class->is_copy_constructible= false;
	coroutine_class->have_destructor= true;
	coroutine_class->is_copy_assignable= false;
	coroutine_class->is_equality_comparable= true;

	// Coroutines can't be constexpr, because heap memory allocation is required in order to call coroutine function.
	// So, we can't just call constexpr generator and save result into some global variable.
	// We can't allocate heap memory in consexpr context and store it somehow later.
	// And we can't deallocate memory too (for global variables of coroutine types).
	coroutine_class->can_be_constexpr= false;

	llvm::Type* const handle_type= llvm::PointerType::get( llvm_context_, 0 );

	coroutine_class->llvm_type=
		llvm::StructType::create(
			llvm_context_,
			{ handle_type },
			mangler_->MangleType( coroutine_class.get() ) );
	coroutine_class->is_complete= true;

	// Generate destructor.
	{
		FunctionVariable destructor_variable= GenerateDestructorPrototype( coroutine_class.get() );
		destructor_variable.have_body= true;
		{
			llvm::Function* const destructor_function= EnsureLLVMFunctionCreated( destructor_variable );
			llvm::IRBuilder ir_builder( llvm::BasicBlock::Create( llvm_context_, "func_code", destructor_function ) );

			llvm::Argument* const this_arg= destructor_function->getArg(0);
			this_arg->setName( Keyword( Keywords::this_ ) );

			ir_builder.CreateCall(
				llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_destroy ),
				{ ir_builder.CreateLoad( handle_type, this_arg, false, "coro_handle" )} );

			ir_builder.CreateRetVoid();
		}

		OverloadedFunctionsSetPtr functions_set= std::make_shared<OverloadedFunctionsSet>();
		functions_set->functions.push_back( std::move( destructor_variable ) );
		coroutine_class->members->AddName( Keyword( Keywords::destructor_ ), std::move( functions_set ) );
	}

	// Generate equality-comparison operator.
	{
		FunctionType op_type;
		op_type.params.resize(2u);
		for( size_t i= 0; i < 2; ++i )
		{
			op_type.params[i].type= res_type;
			op_type.params[i].value_type= ValueType::ReferenceImut;
		}
		op_type.return_type= bool_type_;
		op_type.return_value_type= ValueType::Value;

		FunctionVariable op_variable;
		const std::string op_name= OverloadedOperatorToString( OverloadedOperator::CompareEqual );
		op_variable.llvm_function= std::make_shared<LazyLLVMFunction>( mangler_->MangleFunction( *coroutine_class->members, op_name, op_type ) );
		op_variable.type= std::move( op_type );
		op_variable.is_generated= true;
		op_variable.is_this_call= false;
		op_variable.have_body= true;

		{ // Generate code.
			llvm::Function* const op_llvm_function= EnsureLLVMFunctionCreated( op_variable );
			llvm::IRBuilder ir_builder( llvm::BasicBlock::Create( llvm_context_, "func_code", op_llvm_function ) );

			llvm::Argument* const l_arg= op_llvm_function->getArg(0);
			llvm::Argument* const r_arg= op_llvm_function->getArg(1);
			l_arg->setName( "l" );
			r_arg->setName( "r" );
			ir_builder.CreateRet(
					ir_builder.CreateICmpEQ(
						ir_builder.CreateLoad( handle_type, l_arg, false, "coro_handle_l" ),
						ir_builder.CreateLoad( handle_type, r_arg, false, "coro_handle_r" ) ) );
		}

		// Insert operator.
		OverloadedFunctionsSetPtr operators= std::make_shared<OverloadedFunctionsSet>();
		operators->functions.push_back( std::move( op_variable ) );
		coroutine_class->members->AddName( op_name, std::move( operators ) );
	}

	coroutine_classes_table_[coroutine_type_description]= std::move(coroutine_class);
	return res_type;
}

void CodeBuilder::PrepareGeneratorBlocks( FunctionContext& function_context )
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
		{ llvm::ConstantInt::get( llvm_context_, llvm::APInt( 32u, uint64_t(0) ) ), promise, null, null, },
		"coro_id" );

	llvm::Value* const coro_need_to_alloc= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_alloc ),
		{ coro_id },
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

	llvm::Value* const coro_frame_memory_allocated=
		function_context.llvm_ir_builder.CreateCall( malloc_func_, { coro_frame_size }, "coro_frame_memory_allocated" );

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
			null,
			"coro_need_to_free" );

	const auto block_need_to_free= llvm::BasicBlock::Create( llvm_context_, "need_to_free" );
	function_context.llvm_ir_builder.CreateCondBr( need_to_free, block_need_to_free, function_context.coro_suspend_bb );

	function_context.function->getBasicBlockList().push_back( block_need_to_free );
	function_context.llvm_ir_builder.SetInsertPoint( block_need_to_free );
	function_context.llvm_ir_builder.CreateCall( free_func_, { mem_for_free } );
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

void CodeBuilder::GeneratorYield( NamesScope& names, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc )
{
	if( function_context.coro_suspend_bb == nullptr )
	{
		REPORT_ERROR( YieldOutsideGenerator, names.GetErrors(), src_loc );
		return;
	}

	const ClassPtr coroutine_class= function_context.return_type->GetClassType();
	U_ASSERT( coroutine_class != nullptr );
	U_ASSERT( coroutine_class->coroutine_type_description != std::nullopt );
	const CoroutineTypeDescription& coroutine_type_description= *coroutine_class->coroutine_type_description;

	const Type& yield_type= coroutine_type_description.return_type;

	if( std::get_if<Synt::EmptyVariant>(&expression) != nullptr )
	{
		// Allow empty expression "yield" for void-return coroutines.
		if( !( yield_type == void_type_ && coroutine_type_description.return_value_type == ValueType::Value ) )
			REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, yield_type, void_type_ );

		GeneratorSuspend( names, function_context, src_loc );
		return;
	}

	llvm::Value* const promise= function_context.s_ret_;
	U_ASSERT( promise != nullptr );

	// Fill promise.
	{
		const StackVariablesStorage temp_variables_storage( function_context );

		VariablePtr expression_result= BuildExpressionCodeEnsureVariable( expression, names, function_context );
		if( coroutine_type_description.return_value_type == ValueType::Value )
		{
			if( expression_result->type != yield_type )
			{
				if( const auto conversion_contructor= GetConversionConstructor( expression_result->type, yield_type, names.GetErrors(), src_loc ) )
					expression_result= ConvertVariable( expression_result, yield_type, *conversion_contructor, names, function_context, src_loc );
				else
				{
					REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, yield_type, expression_result->type );
					return;
				}
			}

			// Check correctness of returning references.
			if( expression_result->type.ReferencesTagsCount() > 0u )
			{
				for( const VariablePtr& inner_reference : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( expression_result ) )
				{
					for( const VariablePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes( inner_reference ) )
					{
						if( !IsReferenceAllowedForReturn( function_context, var_node ) )
							REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), src_loc );
					}
				}
			}

			if( expression_result->type.GetFundamentalType() != nullptr||
				expression_result->type.GetEnumType() != nullptr ||
				expression_result->type.GetRawPointerType() != nullptr ||
				expression_result->type.GetFunctionPointerType() != nullptr ) // Just copy simple scalar.
			{
				if( expression_result->type != void_type_ )
					CreateTypedStore( function_context, yield_type, CreateMoveToLLVMRegisterInstruction( *expression_result, function_context ), promise );
			}
			else if( expression_result->value_type == ValueType::Value ) // Move composite value.
			{
				CopyBytes( promise, expression_result->llvm_value, yield_type, function_context );

				function_context.variables_state.MoveNode( expression_result );

				if( expression_result->location == Variable::Location::Pointer )
					CreateLifetimeEnd( function_context, expression_result->llvm_value );
			}
			else // Copy composite value.
			{
				if( !expression_result->type.IsCopyConstructible() )
					REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, expression_result->type );
				else if( yield_type.IsAbstract() )
					REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), src_loc, yield_type );
				else
				{
					BuildCopyConstructorPart(
						promise,
						CreateReferenceCast( expression_result->llvm_value, expression_result->type, yield_type, function_context ),
						yield_type,
						function_context );
				}
			}
		}
		else
		{
			if( !ReferenceIsConvertible( expression_result->type, yield_type, names.GetErrors(), src_loc ) )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, yield_type, expression_result->type );
				return;
			}

			if( expression_result->value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), src_loc );
				return;
			}
			if( expression_result->value_type == ValueType::ReferenceImut && coroutine_type_description.return_value_type == ValueType::ReferenceMut )
			{
				REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), src_loc );
			}

			// Check correctness of returning reference.
			for( const VariablePtr& var_node : function_context.variables_state.GetAllAccessibleVariableNodes( expression_result ) )
				if( !IsReferenceAllowedForReturn( function_context, var_node ) )
					REPORT_ERROR( ReturningUnallowedReference, names.GetErrors(), src_loc );

			// TODO - Add link to return value in order to catch error, when reference to local variable is returned.

			llvm::Value* const ref_casted= CreateReferenceCast( expression_result->llvm_value, expression_result->type, yield_type, function_context );
			CreateTypedReferenceStore( function_context, yield_type, ref_casted, promise );
		}

		// Destroy temporaries of expression evaluation frame.
		CallDestructors( temp_variables_storage, names, function_context, src_loc );
	}

	// Suspend generator. Now generator caller will recieve filled promise.
	GeneratorSuspend( names, function_context, src_loc );
}

// Perform initial suspend or suspend in "yield".
void CodeBuilder::GeneratorSuspend( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc )
{
	llvm::Value* const suspend_value= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_suspend ),
		{ llvm::ConstantTokenNone::get( llvm_context_ ), llvm::ConstantInt::getFalse( llvm_context_ ) },
		"suspend_value" );

	const auto next_block= llvm::BasicBlock::Create( llvm_context_, "suspend_normal" );
	const auto destroy_block= llvm::BasicBlock::Create( llvm_context_, "suspend_destroy" );

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
	CallDestructorsBeforeReturn( names_scope, function_context, src_loc );
	CheckReferencesPollutionBeforeReturn( function_context, names_scope.GetErrors(), src_loc );

	function_context.llvm_ir_builder.CreateBr( function_context.coro_final_suspend_bb );
}

} // namespace U
