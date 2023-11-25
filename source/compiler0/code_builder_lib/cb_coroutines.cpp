#include "keywords.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

void CodeBuilder::PerformCoroutineFunctionReferenceNotationChecks( const FunctionType& function_type, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	// Require completeness of value params and return values before performing checks.

	for( const FunctionType::Param& param : function_type.params )
	{
		if( param.value_type == ValueType::Value )
			EnsureTypeComplete( param.type );
	}

	if( function_type.return_value_type == ValueType::Value )
	{
		EnsureTypeComplete( function_type.return_type );
		const size_t return_type_tags_count= function_type.return_type.ReferencesTagsCount();
		// For coroutines use strict criteria - require setting reference notation with exact size.
		if( function_type.return_inner_references.size() != return_type_tags_count )
			REPORT_ERROR( InnerReferenceTagCountMismatch, errors_container, src_loc, return_type_tags_count, function_type.return_inner_references.size() );
	}

	CheckFunctionReferencesNotationInnerReferences( function_type, errors_container, src_loc );
}

void CodeBuilder::TransformCoroutineFunctionType(
	NamesScope& root_namespace,
	FunctionType& coroutine_function_type,
	const FunctionVariable::Kind kind,
	const bool non_sync )
{
	CoroutineTypeDescription coroutine_type_description;
	coroutine_type_description.return_type= coroutine_function_type.return_type;
	coroutine_type_description.return_value_type= coroutine_function_type.return_value_type;
	coroutine_type_description.non_sync= non_sync;

	switch( kind )
	{
	case FunctionVariable::Kind::Regular:
		U_ASSERT(false);
		break;
	case FunctionVariable::Kind::Generator:
		coroutine_type_description.kind= CoroutineKind::Generator;
		break;
	case FunctionVariable::Kind::Async:
		coroutine_type_description.kind= CoroutineKind::AsyncFunc;
		break;
	}

	// Calculate inner references.
	// Each reference param adds new inner reference.
	// Each value param creates number of references equal to number of inner references of its type.
	// For now reference params of types with references inside are not supported.

	// If this changed, "GetCoroutineInnerReferenceForParamNode" function must be changed too!

	llvm::SmallVector< size_t, 16 > param_to_first_inner_reference_tag;
	std::vector<std::set<FunctionType::ParamReference>> coroutine_return_inner_ferences;

	for( const FunctionType::Param& param : coroutine_function_type.params )
	{
		const size_t param_index= size_t(&param - coroutine_function_type.params.data());
		param_to_first_inner_reference_tag.push_back( coroutine_type_description.inner_references.size() );
		if( param.value_type == ValueType::Value )
		{
			// Require type completeness for value params in order to know inner references.
			if( EnsureTypeComplete( param.type ) )
			{
				const auto reference_tag_count= param.type.ReferencesTagsCount();
				for( size_t i= 0; i < reference_tag_count; ++i )
				{
					coroutine_type_description.inner_references.push_back( param.type.GetInnerReferenceType(i) );
					coroutine_return_inner_ferences.push_back(
						std::set<FunctionType::ParamReference>{
							FunctionType::ParamReference{ uint8_t(param_index), uint8_t(i) } } );
				}
			}
		}
		else
		{
			coroutine_type_description.inner_references.push_back( param.value_type == ValueType::ReferenceMut ? InnerReferenceType::Mut : InnerReferenceType::Imut );
			coroutine_return_inner_ferences.push_back(
				std::set<FunctionType::ParamReference>{
					FunctionType::ParamReference{ uint8_t(param_index), FunctionType::c_arg_reference_tag_number } } );
		}
	}

	// Fill references of return value.
	for( const FunctionType::ParamReference& param_reference : coroutine_function_type.return_references )
	{
		if( param_reference.first >= coroutine_function_type.params.size() )
			continue;

		FunctionType::ParamReference out_reference;
		out_reference.first= 0; // Always use param0 - coroutine itself.
		if( param_reference.second == FunctionType::c_arg_reference_tag_number )
			out_reference.second= uint8_t( param_to_first_inner_reference_tag[ param_reference.first ] );
		else
			out_reference.second= uint8_t( param_to_first_inner_reference_tag[ param_reference.first ] + param_reference.second );

		coroutine_type_description.return_references.insert( out_reference );
	}

	coroutine_type_description.return_inner_references.resize( coroutine_function_type.return_inner_references.size() );
	for( size_t i= 0u; i < coroutine_function_type.return_inner_references.size(); ++i )
	{
		for( const FunctionType::ParamReference& param_reference : coroutine_function_type.return_inner_references[i] )
		{
			if( param_reference.first >= coroutine_function_type.params.size() )
				continue;

			FunctionType::ParamReference out_reference;
			out_reference.first= 0; // Always use param0 - coroutine itself.
			if( param_reference.second == FunctionType::c_arg_reference_tag_number )
				out_reference.second= uint8_t( param_to_first_inner_reference_tag[ param_reference.first ] );
			else
				out_reference.second= uint8_t( param_to_first_inner_reference_tag[ param_reference.first ] + out_reference.second );

			coroutine_type_description.return_inner_references[i].insert( out_reference );
		}
	}

	// Coroutine function returns value of coroutine type.
	coroutine_function_type.return_type= GetCoroutineType( root_namespace, coroutine_type_description );
	coroutine_function_type.return_value_type= ValueType::Value;

	// Params references and references inside param types are mapped to coroutine type inner references.
	coroutine_function_type.return_inner_references= std::move(coroutine_return_inner_ferences);
	coroutine_function_type.return_references.clear();
}

ClassPtr CodeBuilder::GetCoroutineType( NamesScope& root_namespace, const CoroutineTypeDescription& coroutine_type_description )
{
	if( const auto it= coroutine_classes_table_.find( coroutine_type_description ); it != coroutine_classes_table_.end() )
		return it->second.get();

	std::string_view class_base_name;
	switch( coroutine_type_description.kind )
	{
	case CoroutineKind::Generator:
		class_base_name= Keyword( Keywords::generator_ );
		break;
	case CoroutineKind::AsyncFunc:
		class_base_name= Keyword( Keywords::async_ );
		break;
	};
	U_ASSERT( !class_base_name.empty() );

	auto coroutine_class= std::make_unique<Class>( std::string( class_base_name ), &root_namespace );
	const ClassPtr res_type= coroutine_class.get();

	coroutine_class->generated_class_data= coroutine_type_description;
	coroutine_class->inner_references= coroutine_type_description.inner_references;
	coroutine_class->members->SetClass( coroutine_class.get() );
	coroutine_class->parents_list_prepared= true;
	coroutine_class->is_default_constructible= false;
	coroutine_class->is_copy_constructible= false;
	coroutine_class->have_destructor= true;
	coroutine_class->is_copy_assignable= false;
	coroutine_class->is_equality_comparable= true;

	// Coroutines can't be constexpr, because heap memory allocation is required in order to call coroutine function.
	// So, we can't just call constexpr coroutine and save result into some global variable.
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
			this_arg->setName( StringViewToStringRef( Keyword( Keywords::this_ ) ) );

			ir_builder.CreateCall(
				llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_destroy ),
				{ ir_builder.CreateLoad( handle_type, this_arg, false, "coro_handle" )} );

			ir_builder.CreateRetVoid();
		}

		OverloadedFunctionsSetPtr functions_set= std::make_shared<OverloadedFunctionsSet>();
		functions_set->functions.push_back( std::move( destructor_variable ) );
		functions_set->base_class= res_type;
		coroutine_class->members->AddName( Keyword( Keywords::destructor_ ), NamesScopeValue( std::move( functions_set ), SrcLoc() ) );
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
		const std::string_view op_name= OverloadedOperatorToString( OverloadedOperator::CompareEqual );
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
		operators->base_class= res_type;
		coroutine_class->members->AddName( op_name, NamesScopeValue( std::move( operators ), SrcLoc() ) );
	}

	coroutine_classes_table_[coroutine_type_description]= std::move(coroutine_class);
	return res_type;
}

void CodeBuilder::PrepareCoroutineBlocks( FunctionContext& function_context )
{
	llvm::PointerType* const pointer_type= llvm::PointerType::get( llvm_context_, 0 );

	const ClassPtr coroutine_class= function_context.return_type->GetClassType();
	U_ASSERT( coroutine_class != nullptr );
	const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &coroutine_class->generated_class_data );
	U_ASSERT( coroutine_type_description != nullptr );
	llvm::Type* const promise_type=
		coroutine_type_description->return_value_type == ValueType::Value
		? coroutine_type_description->return_type.GetLLVMType()
		: pointer_type;

	function_context.llvm_ir_builder.GetInsertBlock()->setName( "coro_prepare" );

	// Yes, create "alloca" not in "alloca" block. It is safe to do such here.
	llvm::Value* const promise= function_context.llvm_ir_builder.CreateAlloca( promise_type, nullptr, "coro_promise" );

	U_ASSERT( function_context.s_ret == nullptr );
	function_context.s_ret= promise;

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

void CodeBuilder::CoroutineYield( NamesScope& names, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc )
{
	if( function_context.coro_suspend_bb == nullptr )
	{
		REPORT_ERROR( YieldOutsideCoroutine, names.GetErrors(), src_loc );
		return;
	}

	const ClassPtr coroutine_class= function_context.return_type->GetClassType();
	U_ASSERT( coroutine_class != nullptr );
	const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &coroutine_class->generated_class_data );
	U_ASSERT( coroutine_type_description != nullptr );

	if( coroutine_type_description->kind == CoroutineKind::AsyncFunc )
	{
		// Allow empty "yield" for async functions.
		if( std::get_if<Synt::EmptyVariant>(&expression) == nullptr )
			REPORT_ERROR( NonEmptyYieldInAsyncFunction, names.GetErrors(), src_loc );

		CoroutineSuspend( names, function_context, src_loc );
		return;
	}

	// Proces "yield" for generators.

	const Type& yield_type= coroutine_type_description->return_type;

	if( std::get_if<Synt::EmptyVariant>(&expression) != nullptr )
	{
		// Allow empty expression "yield" for void-return generators.
		if( !( yield_type == void_type_ && coroutine_type_description->return_value_type == ValueType::Value ) )
			REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, yield_type, void_type_ );

		CoroutineSuspend( names, function_context, src_loc );
		return;
	}

	llvm::Value* const promise= function_context.s_ret;
	U_ASSERT( promise != nullptr );

	// Fill promise.
	{
		const StackVariablesStorage temp_variables_storage( function_context );

		VariablePtr expression_result= BuildExpressionCodeEnsureVariable( expression, names, function_context );
		if( coroutine_type_description->return_value_type == ValueType::Value )
		{
			if( expression_result->type.ReferenceIsConvertibleTo( yield_type ) )
			{}
			else if( const auto conversion_contructor= GetConversionConstructor( expression_result->type, yield_type, names.GetErrors(), src_loc ) )
				expression_result= ConvertVariable( expression_result, yield_type, *conversion_contructor, names, function_context, src_loc );
			else
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, yield_type, expression_result->type );
				return;
			}

			CheckAsyncReturnInnerReferencesAreAllowed( names, function_context, *coroutine_type_description, expression_result, src_loc );

			if( expression_result->type.GetFundamentalType() != nullptr||
				expression_result->type.GetEnumType() != nullptr ||
				expression_result->type.GetRawPointerType() != nullptr ||
				expression_result->type.GetFunctionPointerType() != nullptr ) // Just copy simple scalar.
			{
				if( expression_result->type != void_type_ )
					CreateTypedStore( function_context, yield_type, CreateMoveToLLVMRegisterInstruction( *expression_result, function_context ), promise );
			}
			else if( expression_result->value_type == ValueType::Value && expression_result->type == yield_type ) // Move composite value.
			{
				CopyBytes( promise, expression_result->llvm_value, yield_type, function_context );

				function_context.variables_state.MoveNode( expression_result );

				if( expression_result->location == Variable::Location::Pointer )
					CreateLifetimeEnd( function_context, expression_result->llvm_value );
			}
			else // Copy composite value.
			{
				if( !expression_result->type.IsCopyConstructible() )
					REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), src_loc, expression_result->type );
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
			if( expression_result->value_type == ValueType::ReferenceImut && coroutine_type_description->return_value_type == ValueType::ReferenceMut )
			{
				REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), src_loc );
			}

			CheckAsyncReturnReferenceIsAllowed( names, function_context, *coroutine_type_description, expression_result, src_loc );

			// TODO - Add link to return value in order to catch error, when reference to local variable is returned.

			llvm::Value* const ref_casted= CreateReferenceCast( expression_result->llvm_value, expression_result->type, yield_type, function_context );
			CreateTypedReferenceStore( function_context, yield_type, ref_casted, promise );
		}

		// Destroy temporaries of expression evaluation frame.
		CallDestructors( temp_variables_storage, names, function_context, src_loc );
	}

	// Suspend generator. Now generator caller will receive filled promise.
	CoroutineSuspend( names, function_context, src_loc );
}

void CodeBuilder::AsyncReturn( NamesScope& names, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc )
{
	const ClassPtr coroutine_class= function_context.return_type->GetClassType();
	U_ASSERT( coroutine_class != nullptr );
	const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &coroutine_class->generated_class_data );
	U_ASSERT( coroutine_type_description != nullptr );

	const Type& return_type= coroutine_type_description->return_type;

	llvm::Value* const promise= function_context.s_ret;
	U_ASSERT( promise != nullptr );

	// Destruction frame for temporary variables of result expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	VariablePtr expression_result= BuildExpressionCodeEnsureVariable( expression, names, function_context );
	if( expression_result->type == invalid_type_ )
		return;

	const VariablePtr return_value_node=
		Variable::Create(
			return_type,
			coroutine_type_description->return_value_type,
			Variable::Location::Pointer,
			"return value lock" );
	function_context.variables_state.AddNode( return_value_node );

	if( coroutine_type_description->return_value_type == ValueType::Value )
	{
		if( expression_result->type.ReferenceIsConvertibleTo( return_type ) )
		{}
		else if( const auto conversion_contructor= GetConversionConstructor( expression_result->type, return_type, names.GetErrors(), src_loc ) )
			expression_result= ConvertVariable( expression_result, return_type, *conversion_contructor, names, function_context, src_loc );
		else
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, return_type, expression_result->type );
			function_context.variables_state.RemoveNode( return_value_node );
			return;
		}

		CheckAsyncReturnInnerReferencesAreAllowed( names, function_context, *coroutine_type_description, expression_result, src_loc );
		function_context.variables_state.TryAddInnerLinks( expression_result, return_value_node, names.GetErrors(), src_loc );

		if( expression_result->type.GetFundamentalType() != nullptr||
			expression_result->type.GetEnumType() != nullptr ||
			expression_result->type.GetRawPointerType() != nullptr ||
			expression_result->type.GetFunctionPointerType() != nullptr ) // Just copy simple scalar.
		{
			if( expression_result->type != void_type_ )
				CreateTypedStore( function_context, return_type, CreateMoveToLLVMRegisterInstruction( *expression_result, function_context ), promise );
		}
		else if( expression_result->value_type == ValueType::Value && expression_result->type == return_type ) // Move composite value.
		{
			CopyBytes( promise, expression_result->llvm_value, return_type, function_context );

			function_context.variables_state.MoveNode( expression_result );

			if( expression_result->location == Variable::Location::Pointer )
				CreateLifetimeEnd( function_context, expression_result->llvm_value );
		}
		else // Copy composite value.
		{
			if( !expression_result->type.IsCopyConstructible() )
				REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), src_loc, expression_result->type );
			else if( return_type.IsAbstract() )
				REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), src_loc, return_type );
			else
			{
				BuildCopyConstructorPart(
					promise,
					CreateReferenceCast( expression_result->llvm_value, expression_result->type, return_type, function_context ),
					return_type,
					function_context );
			}
		}
	}
	else
	{
		if( !ReferenceIsConvertible( expression_result->type, return_type, names.GetErrors(), src_loc ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, return_type, expression_result->type );
			function_context.variables_state.RemoveNode( return_value_node );
			return;
		}

		if( expression_result->value_type == ValueType::Value )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), src_loc );
			function_context.variables_state.RemoveNode( return_value_node );
			return;
		}
		if( expression_result->value_type == ValueType::ReferenceImut && coroutine_type_description->return_value_type == ValueType::ReferenceMut )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), src_loc );
		}

		CheckAsyncReturnReferenceIsAllowed( names, function_context, *coroutine_type_description, expression_result, src_loc );

		// Add link to return value in order to catch error, when reference to local variable is returned.
		function_context.variables_state.TryAddLink( expression_result, return_value_node, names.GetErrors(), src_loc );
		function_context.variables_state.TryAddInnerLinks( expression_result, return_value_node, names.GetErrors(), src_loc );

		llvm::Value* const ref_casted= CreateReferenceCast( expression_result->llvm_value, expression_result->type, return_type, function_context );
		CreateTypedReferenceStore( function_context, return_type, ref_casted, promise );
	}

	CallDestructorsBeforeReturn( names, function_context, src_loc );
	CheckReferencesPollutionBeforeReturn( function_context, names.GetErrors(), src_loc );
	function_context.variables_state.RemoveNode( return_value_node );

	function_context.llvm_ir_builder.CreateBr( function_context.coro_final_suspend_bb );
}

Value CodeBuilder::BuildAwait( NamesScope& names, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc )
{
	const VariablePtr async_func_variable= BuildExpressionCodeEnsureVariable( expression, names, function_context );
	if( async_func_variable->type == invalid_type_ )
		return ErrorValue();

	if( async_func_variable->value_type != ValueType::Value )
	{
		REPORT_ERROR( ImmediateValueExpectedInAwaitOperator, names.GetErrors(), src_loc );
		return ErrorValue();
	}

	const Class* const class_type= async_func_variable->type.GetClassType();
	if( class_type == nullptr )
	{
		REPORT_ERROR( AwaitForNonAsyncFunctionValue, names.GetErrors(), src_loc );
		return ErrorValue();
	}

	const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &class_type->generated_class_data );
	if( coroutine_type_description == nullptr || coroutine_type_description->kind != CoroutineKind::AsyncFunc )
	{
		REPORT_ERROR( AwaitForNonAsyncFunctionValue, names.GetErrors(), src_loc );
		return ErrorValue();
	}

	if( function_context.coro_suspend_bb == nullptr )
	{
		REPORT_ERROR( AwaitOutsideAsyncFunction, names.GetErrors(), src_loc );
		return ErrorValue();
	}
	if( function_context.return_type != std::nullopt )
	{
		if( const auto function_class_type= function_context.return_type->GetClassType() )
		{
			if( const auto function_coroutine_type_description= std::get_if<CoroutineTypeDescription>( &function_class_type->generated_class_data ) )
			{
				if( function_coroutine_type_description->kind != CoroutineKind::AsyncFunc )
				{
					// Prevent usage of "await" in generators.
					REPORT_ERROR( AwaitOutsideAsyncFunction, names.GetErrors(), src_loc );
					return ErrorValue();
				}
			}
		}
	}

	const Type& return_type= coroutine_type_description->return_type;

	const VariableMutPtr result=
		Variable::Create(
			return_type,
			coroutine_type_description->return_value_type,
			Variable::Location::Pointer,
			async_func_variable->name + " await result" );
	function_context.variables_state.AddNode( result );

	if( !function_context.is_functionless_context )
	{
		const auto already_done_block= llvm::BasicBlock::Create( llvm_context_, "already_done" );
		const auto loop_block= llvm::BasicBlock::Create( llvm_context_, "await_loop" );
		const auto not_done_block= llvm::BasicBlock::Create( llvm_context_, "await_not_done" );
		const auto done_block= llvm::BasicBlock::Create( llvm_context_, "await_done" );

		llvm::Value* const coro_handle=
			function_context.llvm_ir_builder.CreateLoad( llvm::PointerType::get( llvm_context_, 0 ), async_func_variable->llvm_value, false, "coro_handle" );

		llvm::Value* const done= function_context.llvm_ir_builder.CreateCall(
			llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_done ),
			{ coro_handle },
			"coro_done" );

		function_context.llvm_ir_builder.CreateCondBr( done, already_done_block, loop_block );

		// Already done block.
		function_context.function->getBasicBlockList().push_back( already_done_block );
		function_context.llvm_ir_builder.SetInsertPoint( already_done_block );
		// Halt if coroutine is already finished. There is no other way to create a fallback in such case.
		// Normally this should not happen - in most case "await" operator should be used directly for async function call result.
		function_context.llvm_ir_builder.CreateCall( halt_func_ );
		function_context.llvm_ir_builder.CreateUnreachable();

		// Loop block.
		function_context.function->getBasicBlockList().push_back( loop_block );
		function_context.llvm_ir_builder.SetInsertPoint( loop_block );

		function_context.llvm_ir_builder.CreateCall(
			llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_resume ),
			{ coro_handle } );

		llvm::Value* const done_after_resume= function_context.llvm_ir_builder.CreateCall(
			llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_done ),
			{ coro_handle },
			"coro_done_after_resume" );

		function_context.llvm_ir_builder.CreateCondBr( done_after_resume, done_block, not_done_block );

		// Not done block.
		function_context.function->getBasicBlockList().push_back( not_done_block );
		function_context.llvm_ir_builder.SetInsertPoint( not_done_block );

		// TODO - perform context save independent on suspend?
		CoroutineSuspend( names, function_context, src_loc );
		function_context.llvm_ir_builder.CreateBr( loop_block ); // Continue to check if the coroutine is done.

		// Done block.
		function_context.function->getBasicBlockList().push_back( done_block );
		function_context.llvm_ir_builder.SetInsertPoint( done_block );

		llvm::Type* const promise_llvm_type=
			coroutine_type_description->return_value_type == ValueType::Value
				? return_type.GetLLVMType()
				: llvm::PointerType::get( llvm_context_, 0 );

		llvm::Value* const promise=
			function_context.llvm_ir_builder.CreateCall(
				llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_promise ),
				{
					coro_handle,
					llvm::ConstantInt::get( llvm_context_, llvm::APInt( 32u, data_layout_.getABITypeAlignment( promise_llvm_type ) ) ),
					llvm::ConstantInt::getFalse( llvm_context_ ),
				},
				"await_promise" );

		if( result->value_type == ValueType::Value )
		{
			result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( return_type.GetLLVMType(), nullptr, result->name );
			CreateLifetimeStart( function_context, result->llvm_value );
			CopyBytes( result->llvm_value, promise, return_type, function_context );
		}
		else
			result->llvm_value= CreateTypedReferenceLoad( function_context, return_type, promise );
	}

	if( result->value_type == ValueType::Value )
	{
		for( size_t i= 0; i < std::min( result->inner_reference_nodes.size(), coroutine_type_description->return_inner_references.size() ); ++i )
		{
			for( const FunctionType::ParamReference& param_reference : coroutine_type_description->return_inner_references[i] )
			{
				U_ASSERT( param_reference.first == 0u );
				U_ASSERT( param_reference.second != FunctionType::c_arg_reference_tag_number );
				if( param_reference.second < async_func_variable->inner_reference_nodes.size() )
					function_context.variables_state.TryAddLink(
						async_func_variable->inner_reference_nodes[param_reference.second],
						result->inner_reference_nodes[i],
						names.GetErrors(),
						src_loc );
			}
		}
	}
	else
	{
		for( const FunctionType::ParamReference& param_reference : coroutine_type_description->return_references )
		{
			U_ASSERT( param_reference.first == 0u );
			U_ASSERT( param_reference.second != FunctionType::c_arg_reference_tag_number );
			if( param_reference.second < async_func_variable->inner_reference_nodes.size() )
				function_context.variables_state.TryAddLink( async_func_variable->inner_reference_nodes[param_reference.second], result, names.GetErrors(), src_loc );
		}
	}

	// Move async function value, call destructor and create lifetime end.
	// TODO - does it make sense to call a destructor for finished coroutine?

	function_context.variables_state.MoveNode( async_func_variable );

	if( !function_context.is_functionless_context )
	{
		if( async_func_variable->type.HaveDestructor() )
			CallDestructor( async_func_variable->llvm_value, async_func_variable->type, function_context, names.GetErrors(), src_loc );

		U_ASSERT( async_func_variable->location == Variable::Location::Pointer );
		CreateLifetimeEnd( function_context, async_func_variable->llvm_value );
	}

	RegisterTemporaryVariable( function_context, result );
	return result;
}

// Perform initial suspend or suspend in "yield".
void CodeBuilder::CoroutineSuspend( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc )
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

void CodeBuilder::CoroutineFinalSuspend( NamesScope& names_scope, FunctionContext& function_context, const SrcLoc& src_loc )
{
	// We can destroy all local variables right now. Leave only coroutine cleanup code in destroy block.
	CallDestructorsBeforeReturn( names_scope, function_context, src_loc );
	CheckReferencesPollutionBeforeReturn( function_context, names_scope.GetErrors(), src_loc );

	function_context.llvm_ir_builder.CreateBr( function_context.coro_final_suspend_bb );
}

} // namespace U
