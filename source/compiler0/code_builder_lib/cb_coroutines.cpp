#include "keywords.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "error_reporting.hpp"
#include "code_builder.hpp"

namespace U
{

void CodeBuilder::PerformCoroutineFunctionReferenceNotationChecks( const FunctionType& function_type, CodeBuilderErrorsContainer& errors_container, const SrcLoc& src_loc )
{
	// Require completeness of all param types and return type in order to perform reference notation checks.

	for( const FunctionType::Param& param : function_type.params )
		EnsureTypeComplete( param.type );

	EnsureTypeComplete( function_type.return_type );

	const size_t return_type_tag_count= function_type.return_type.ReferenceTagCount();
	// For coroutines use strict criteria - require setting reference notation with exact size.
	if( function_type.return_inner_references.size() != return_type_tag_count )
		REPORT_ERROR( InnerReferenceTagCountMismatch, errors_container, src_loc, return_type_tag_count, function_type.return_inner_references.size() );

	CheckFunctionReferencesNotationInnerReferences( function_type, errors_container, src_loc );
}

void CodeBuilder::TransformCoroutineFunctionType(
	FunctionType& coroutine_function_type, FunctionVariable::Kind kind, NamesScope& names_scope, const SrcLoc& src_loc )
{
	CoroutineTypeDescription coroutine_type_description;
	coroutine_type_description.return_type= coroutine_function_type.return_type;
	coroutine_type_description.return_value_type= coroutine_function_type.return_value_type;

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

	// Non-sync property is based on non-sync property of args and return values.
	// Evaluate it immediately.

	coroutine_type_description.non_sync= false;
	if( EnsureTypeComplete( coroutine_function_type.return_type ) &&
		GetTypeNonSync( coroutine_function_type.return_type, names_scope, src_loc ) )
		coroutine_type_description.non_sync= true;

	for( const FunctionType::Param& param : coroutine_function_type.params )
		if( EnsureTypeComplete( param.type ) &&
			GetTypeNonSync( param.type, names_scope, src_loc ) )
			coroutine_type_description.non_sync= true;

	// Calculate inner references.
	// Each reference param adds new inner reference.
	// Each value param creates number of references equal to number of inner references of its type.
	// For now reference params of types with references inside are not supported.

	// If this changed, "GetCoroutineInnerReferenceForParamNode" function must be changed too!

	llvm::SmallVector< size_t, 16 > param_to_first_inner_reference_tag;
	FunctionType::ReturnInnerReferences coroutine_return_inner_ferences;

	for( const FunctionType::Param& param : coroutine_function_type.params )
	{
		const size_t param_index= size_t(&param - coroutine_function_type.params.data());
		param_to_first_inner_reference_tag.push_back( coroutine_type_description.inner_references.size() );
		if( param.value_type == ValueType::Value )
		{
			// Require type completeness for value params in order to know inner references.
			if( EnsureTypeComplete( param.type ) )
			{
				const auto reference_tag_count= param.type.ReferenceTagCount();
				for( size_t i= 0; i < reference_tag_count; ++i )
				{
					coroutine_type_description.inner_references.push_back( param.type.GetInnerReferenceKind(i) );
					coroutine_return_inner_ferences.push_back(
						FunctionType::ReturnReferences{
							FunctionType::ParamReference{ uint8_t(param_index), uint8_t(i) } } );
				}
			}
		}
		else
		{
			// Coroutine is an object, that holds references to reference-args of coroutine function.
			// It's generally not allowed to create types with references to other types with references inside.
			// Second order references are possible in some cases, but for now not for coroutines.
			if( EnsureTypeComplete( param.type ) && param.type.ReferenceTagCount() > 0u )
			{
				std::string field_name= "param ";
				field_name+= std::to_string( size_t( &param - coroutine_function_type.params.data() ) );
				field_name+= " of type ";
				field_name+= param.type.ToString();
				REPORT_ERROR( ReferenceIndirectionDepthExceeded, names_scope.GetErrors(), src_loc, 1, field_name ); // TODO - use separate error code.
			}

			coroutine_type_description.inner_references.push_back( param.value_type == ValueType::ReferenceMut ? InnerReferenceKind::Mut : InnerReferenceKind::Imut );
			coroutine_return_inner_ferences.push_back(
				FunctionType::ReturnReferences{
					FunctionType::ParamReference{ uint8_t(param_index), FunctionType::c_param_reference_number } } );
		}
	}

	// Fill references of return value.
	for( const FunctionType::ParamReference& param_reference : coroutine_function_type.return_references )
	{
		if( param_reference.first >= coroutine_function_type.params.size() )
			continue;

		FunctionType::ParamReference out_reference;
		out_reference.first= 0; // Always use param0 - coroutine itself.
		if( param_reference.second == FunctionType::c_param_reference_number )
			out_reference.second= uint8_t( param_to_first_inner_reference_tag[ param_reference.first ] );
		else
			out_reference.second= uint8_t( param_to_first_inner_reference_tag[ param_reference.first ] + param_reference.second );

		coroutine_type_description.return_references.push_back( out_reference );
	}

	NormalizeParamReferencesList( coroutine_type_description.return_references );

	coroutine_type_description.return_inner_references.resize( coroutine_function_type.return_inner_references.size() );
	for( size_t i= 0u; i < coroutine_function_type.return_inner_references.size(); ++i )
	{
		for( const FunctionType::ParamReference& param_reference : coroutine_function_type.return_inner_references[i] )
		{
			if( param_reference.first >= coroutine_function_type.params.size() )
				continue;

			FunctionType::ParamReference out_reference;
			out_reference.first= 0; // Always use param0 - coroutine itself.
			if( param_reference.second == FunctionType::c_param_reference_number )
				out_reference.second= uint8_t( param_to_first_inner_reference_tag[ param_reference.first ] );
			else
				out_reference.second= uint8_t( param_to_first_inner_reference_tag[ param_reference.first ] + out_reference.second );

			coroutine_type_description.return_inner_references[i].push_back( out_reference );
		}
		NormalizeParamReferencesList( coroutine_type_description.return_inner_references[i] );
	}

	// Coroutine function returns value of coroutine type.
	coroutine_function_type.return_type= GetCoroutineType( *names_scope.GetRoot(), coroutine_type_description );
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

	// TODO - store InnerReference for coroutines?
	coroutine_class->inner_references.reserve( coroutine_type_description.inner_references.size() );
	for( const InnerReferenceKind k : coroutine_type_description.inner_references )
		coroutine_class->inner_references.push_back( InnerReference( k ) );

	coroutine_class->members->SetClass( coroutine_class.get() );
	coroutine_class->kind= Class::Kind::NonPolymorph; // Mark coroutine type as non-struct, to avoid usages it as struct ({} initializer, decompose).
	coroutine_class->parents_list_prepared= true;
	coroutine_class->is_default_constructible= false;
	coroutine_class->is_copy_constructible= false;
	coroutine_class->has_destructor= true;
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
		destructor_variable.has_body= true;
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
		op_variable.has_body= true;

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
	// We need to mark somehow basic blocks for further optimizations.
	// Since it's not possible to associate metadata with blocks, associate it with first block instrucitons.

	llvm::PointerType* const pointer_type= llvm::PointerType::get( llvm_context_, 0 );

	const ClassPtr coroutine_class= function_context.function_type.return_type.GetClassType();
	U_ASSERT( coroutine_class != nullptr );
	const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &coroutine_class->generated_class_data );
	U_ASSERT( coroutine_type_description != nullptr );
	llvm::Type* const promise_type=
		coroutine_type_description->return_value_type == ValueType::Value
		? coroutine_type_description->return_type.GetLLVMType()
		: pointer_type;

	llvm::Value* const promise= function_context.alloca_ir_builder.CreateAlloca( promise_type, nullptr, "coro_promise" );

	const auto coro_prepare_block= llvm::BasicBlock::Create( llvm_context_, "coro_prepare" );
	function_context.llvm_ir_builder.CreateBr( coro_prepare_block );

	// Prepare block.
	coro_prepare_block->insertInto( function_context.function );
	function_context.llvm_ir_builder.SetInsertPoint( coro_prepare_block );

	U_ASSERT( function_context.s_ret == nullptr );
	function_context.s_ret= promise;

	llvm::Value* const null= llvm::ConstantPointerNull::get( pointer_type );

	llvm::CallInst* const coro_id= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_id ),
		{ llvm::ConstantInt::get( llvm_context_, llvm::APInt( 32u, uint64_t(0) ) ), promise, null, null, },
		"coro_id" );
	coro_id->setMetadata( llvm::StringRef( "u_coro_block_prepare" ), llvm::MDNode::get( llvm_context_, {} ) );

	llvm::Value* const coro_need_to_alloc= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_alloc ),
		{ coro_id },
		"coro_need_to_alloc" );

	llvm::BasicBlock* const coro_need_to_alloc_check_block= function_context.llvm_ir_builder.GetInsertBlock();

	const auto block_need_to_alloc= llvm::BasicBlock::Create( llvm_context_, "need_to_alloc" );
	const auto block_coro_begin= llvm::BasicBlock::Create( llvm_context_, "coro_begin" );

	function_context.llvm_ir_builder.CreateCondBr( coro_need_to_alloc, block_need_to_alloc, block_coro_begin );

	// Need to alloc block.
	block_need_to_alloc->insertInto( function_context.function );
	function_context.llvm_ir_builder.SetInsertPoint( block_need_to_alloc );

	llvm::CallInst* const coro_frame_size= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_size, { fundamental_llvm_types_.size_type_ } ),
		{},
		"coro_frame_size" );
	coro_frame_size->setMetadata( llvm::StringRef( "u_coro_block" ), llvm::MDNode::get( llvm_context_, {} ) );

	llvm::CallInst* const coro_frame_memory_allocated=
		function_context.llvm_ir_builder.CreateCall( malloc_func_, { coro_frame_size }, "coro_frame_memory_allocated" );
	coro_frame_memory_allocated->setCallingConv( malloc_func_->getCallingConv() );

	function_context.llvm_ir_builder.CreateBr( block_coro_begin );

	// Coro begin block.
	block_coro_begin->insertInto( function_context.function );
	function_context.llvm_ir_builder.SetInsertPoint( block_coro_begin );

	llvm::PHINode* const coro_frame_memory= function_context.llvm_ir_builder.CreatePHI( pointer_type, 2, "coro_frame_memory" );
	coro_frame_memory->addIncoming( null, coro_need_to_alloc_check_block );
	coro_frame_memory->addIncoming( coro_frame_memory_allocated, block_need_to_alloc );

	coro_frame_memory->setMetadata( llvm::StringRef( "u_coro_block_begin" ), llvm::MDNode::get( llvm_context_, {} ) );

	llvm::Value* const coro_handle= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_begin ),
		{ coro_id, coro_frame_memory },
		"coro_handle" );

	function_context.coro_suspend_bb= llvm::BasicBlock::Create( llvm_context_, "coro_suspend" );

	const auto func_code_block= llvm::BasicBlock::Create( llvm_context_, "func_code_after_coro_blocks" );
	function_context.llvm_ir_builder.CreateBr( func_code_block );

	// Cleanup block.
	function_context.coro_cleanup_bb= llvm::BasicBlock::Create( llvm_context_, "coro_cleanup" );
	function_context.coro_cleanup_bb->insertInto( function_context.function );
	function_context.llvm_ir_builder.SetInsertPoint( function_context.coro_cleanup_bb );

	llvm::CallInst* const mem_for_free= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_free ),
		{ coro_id, coro_handle },
		"coro_frame_memory_for_free" );
	mem_for_free->setMetadata( llvm::StringRef( "u_coro_block_cleanup" ), llvm::MDNode::get( llvm_context_, {} ) );

	llvm::Value* const need_to_free=
		function_context.llvm_ir_builder.CreateICmpNE(
			mem_for_free,
			null,
			"coro_need_to_free" );

	const auto block_need_to_free= llvm::BasicBlock::Create( llvm_context_, "need_to_free" );
	function_context.llvm_ir_builder.CreateCondBr( need_to_free, block_need_to_free, function_context.coro_suspend_bb );

	// Need to free block.
	block_need_to_free->insertInto( function_context.function );
	function_context.llvm_ir_builder.SetInsertPoint( block_need_to_free );
	llvm::CallInst* const free_call= function_context.llvm_ir_builder.CreateCall( free_func_, { mem_for_free } );
	free_call->setCallingConv( free_func_->getCallingConv() );
	free_call->setMetadata( llvm::StringRef( "u_coro_block" ), llvm::MDNode::get( llvm_context_, {} ) );
	function_context.llvm_ir_builder.CreateBr( function_context.coro_suspend_bb );

	// Suspend block.
	function_context.coro_suspend_bb->insertInto( function_context.function );
	function_context.llvm_ir_builder.SetInsertPoint( function_context.coro_suspend_bb );

	llvm::CallInst* const end_call= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_end ),
		{ coro_handle, llvm::ConstantInt::getFalse( llvm_context_ ) } );
	end_call->setMetadata( llvm::StringRef( "u_coro_block_suspend" ), llvm::MDNode::get( llvm_context_, {} ) );

	function_context.llvm_ir_builder.CreateRet( coro_handle );

	// End suspention point.
	function_context.coro_final_suspend_bb= llvm::BasicBlock::Create( llvm_context_, "coro_suspend_final" );
	function_context.coro_final_suspend_bb->insertInto( function_context.function );
	function_context.llvm_ir_builder.SetInsertPoint( function_context.coro_final_suspend_bb );

	llvm::CallInst* const final_suspend_value= function_context.llvm_ir_builder.CreateCall(
		llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_suspend ),
		{ llvm::ConstantTokenNone::get( llvm_context_ ), llvm::ConstantInt::getTrue( llvm_context_ ) },
		"final_suspend_value" );
	final_suspend_value->setMetadata( llvm::StringRef( "u_coro_block_suspend_final" ), llvm::MDNode::get( llvm_context_, {} ) );

	const auto unreachable_block= llvm::BasicBlock::Create( llvm_context_, "coro_final_suspend_unreachable" );

	llvm::SwitchInst* const switch_instr= function_context.llvm_ir_builder.CreateSwitch( final_suspend_value, function_context.coro_suspend_bb, 2 );
	switch_instr->addCase( llvm::ConstantInt::get( fundamental_llvm_types_.i8_, 0u, false ), unreachable_block );
	switch_instr->addCase( llvm::ConstantInt::get( fundamental_llvm_types_.i8_, 1u, false ), function_context.coro_cleanup_bb );

	// Final suspend unreachable block.
	// It's undefined behaviour to resume coroutine in final suspention state. So, just add unreachable instruction here.
	unreachable_block->insertInto( function_context.function );
	function_context.llvm_ir_builder.SetInsertPoint( unreachable_block );
	llvm::UnreachableInst* unreachable_instruction= function_context.llvm_ir_builder.CreateUnreachable();
	unreachable_instruction->setMetadata( llvm::StringRef( "u_coro_block" ), llvm::MDNode::get( llvm_context_, {} ) );

	// Block for further function code.
	func_code_block->insertInto( function_context.function );
	function_context.llvm_ir_builder.SetInsertPoint( func_code_block );
}

void CodeBuilder::CoroutineYield( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc )
{
	if( function_context.coro_suspend_bb == nullptr )
	{
		REPORT_ERROR( YieldOutsideCoroutine, names_scope.GetErrors(), src_loc );
		return;
	}

	const ClassPtr coroutine_class= function_context.function_type.return_type.GetClassType();
	U_ASSERT( coroutine_class != nullptr );
	const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &coroutine_class->generated_class_data );
	U_ASSERT( coroutine_type_description != nullptr );

	if( coroutine_type_description->kind == CoroutineKind::AsyncFunc )
	{
		// Allow empty "yield" for async functions.
		if( !std::holds_alternative<Synt::EmptyVariant>(expression) )
			REPORT_ERROR( NonEmptyYieldInAsyncFunction, names_scope.GetErrors(), src_loc );

		CoroutineSuspend( names_scope, function_context, src_loc );
		return;
	}

	// Proces "yield" for generators.

	const Type& yield_type= coroutine_type_description->return_type;

	if( std::holds_alternative<Synt::EmptyVariant>(expression) )
	{
		// Allow empty expression "yield" for void-return generators.
		if( !( yield_type == void_type_ && coroutine_type_description->return_value_type == ValueType::Value ) )
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, yield_type, void_type_ );

		CoroutineSuspend( names_scope, function_context, src_loc );
		return;
	}

	llvm::Value* const promise= function_context.s_ret;
	U_ASSERT( promise != nullptr );

	// Fill promise.
	{
		const StackVariablesStorage temp_variables_storage( function_context );

		VariablePtr expression_result= BuildExpressionCodeEnsureVariable( expression, names_scope, function_context );
		if( coroutine_type_description->return_value_type == ValueType::Value )
		{
			if( function_context.variables_state.HasOutgoingMutableNodes( expression_result ) )
				REPORT_ERROR( ReferenceProtectionError, names_scope.GetErrors(), src_loc, expression_result->name );

			if( expression_result->type.ReferenceIsConvertibleTo( yield_type ) )
			{}
			else if( const auto conversion_contructor=
					GetConversionConstructor(
						FunctionType::Param{ expression_result->type, expression_result->value_type },
						yield_type,
						names_scope.GetErrors(),
						src_loc ) )
				expression_result= ConvertVariable( expression_result, yield_type, *conversion_contructor, names_scope, function_context, src_loc );
			else
			{
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, yield_type, expression_result->type );
				return;
			}

			CheckAsyncReturnInnerReferencesAreAllowed( names_scope, function_context, *coroutine_type_description, expression_result, src_loc );

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
					REPORT_ERROR( CopyConstructValueOfNoncopyableType, names_scope.GetErrors(), src_loc, expression_result->type );
				else if( yield_type.IsAbstract() )
					REPORT_ERROR( ConstructingAbstractClassOrInterface, names_scope.GetErrors(), src_loc, yield_type );
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
			if( !ReferenceIsConvertible( expression_result->type, yield_type, names_scope.GetErrors(), src_loc ) )
			{
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, yield_type, expression_result->type );
				return;
			}

			if( expression_result->value_type == ValueType::Value )
			{
				REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), src_loc );
				return;
			}
			if( expression_result->value_type == ValueType::ReferenceImut && coroutine_type_description->return_value_type == ValueType::ReferenceMut )
			{
				REPORT_ERROR( BindingConstReferenceToNonconstReference, names_scope.GetErrors(), src_loc );
			}

			CheckAsyncReturnReferenceIsAllowed( names_scope, function_context, *coroutine_type_description, expression_result, src_loc );

			// TODO - Add link to return value in order to catch error, when reference to local variable is returned.

			llvm::Value* const ref_casted= CreateReferenceCast( expression_result->llvm_value, expression_result->type, yield_type, function_context );
			CreateTypedReferenceStore( function_context, yield_type, ref_casted, promise );
		}

		// Destroy temporaries of expression evaluation frame.
		CallDestructors( temp_variables_storage, names_scope, function_context, src_loc );
	}

	// Suspend generator. Now generator caller will receive filled promise.
	CoroutineSuspend( names_scope, function_context, src_loc );
}

void CodeBuilder::AsyncReturn( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc )
{
	const ClassPtr coroutine_class= function_context.function_type.return_type.GetClassType();
	U_ASSERT( coroutine_class != nullptr );
	const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &coroutine_class->generated_class_data );
	U_ASSERT( coroutine_type_description != nullptr );

	const Type& return_type= coroutine_type_description->return_type;

	llvm::Value* const promise= function_context.s_ret;
	U_ASSERT( promise != nullptr );

	// Destruction frame for temporary variables of result expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	VariablePtr expression_result=
		coroutine_type_description->return_value_type == ValueType::Value
			? BuildExpressionCodeForValueReturn( expression, names_scope, function_context )
			: BuildExpressionCodeEnsureVariable( expression, names_scope, function_context );
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
		if( function_context.variables_state.HasOutgoingMutableNodes( expression_result ) )
			REPORT_ERROR( ReferenceProtectionError, names_scope.GetErrors(), src_loc, expression_result->name );

		if( expression_result->type.ReferenceIsConvertibleTo( return_type ) )
		{}
		else if( const auto conversion_contructor=
				GetConversionConstructor(
					FunctionType::Param{ expression_result->type, expression_result->value_type },
					return_type,
					names_scope.GetErrors(),
					src_loc ) )
			expression_result= ConvertVariable( expression_result, return_type, *conversion_contructor, names_scope, function_context, src_loc );
		else
		{
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, return_type, expression_result->type );
			function_context.variables_state.RemoveNode( return_value_node );
			return;
		}

		CheckAsyncReturnInnerReferencesAreAllowed( names_scope, function_context, *coroutine_type_description, expression_result, src_loc );
		function_context.variables_state.TryAddInnerLinks( expression_result, return_value_node, names_scope.GetErrors(), src_loc );

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
				REPORT_ERROR( CopyConstructValueOfNoncopyableType, names_scope.GetErrors(), src_loc, expression_result->type );
			else if( return_type.IsAbstract() )
				REPORT_ERROR( ConstructingAbstractClassOrInterface, names_scope.GetErrors(), src_loc, return_type );
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
		if( !ReferenceIsConvertible( expression_result->type, return_type, names_scope.GetErrors(), src_loc ) )
		{
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, return_type, expression_result->type );
			function_context.variables_state.RemoveNode( return_value_node );
			return;
		}

		if( expression_result->value_type == ValueType::Value )
		{
			REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), src_loc );
			function_context.variables_state.RemoveNode( return_value_node );
			return;
		}
		if( expression_result->value_type == ValueType::ReferenceImut && coroutine_type_description->return_value_type == ValueType::ReferenceMut )
		{
			REPORT_ERROR( BindingConstReferenceToNonconstReference, names_scope.GetErrors(), src_loc );
		}

		CheckAsyncReturnReferenceIsAllowed( names_scope, function_context, *coroutine_type_description, expression_result, src_loc );

		// Add link to return value in order to catch error, when reference to local variable is returned.
		function_context.variables_state.TryAddLink( expression_result, return_value_node, names_scope.GetErrors(), src_loc );
		function_context.variables_state.TryAddInnerLinks( expression_result, return_value_node, names_scope.GetErrors(), src_loc );

		llvm::Value* const ref_casted= CreateReferenceCast( expression_result->llvm_value, expression_result->type, return_type, function_context );
		CreateTypedReferenceStore( function_context, return_type, ref_casted, promise );
	}

	CallDestructorsBeforeReturn( names_scope, function_context, src_loc );
	CheckReferencesPollutionBeforeReturn( function_context, names_scope.GetErrors(), src_loc );
	function_context.variables_state.RemoveNode( return_value_node );

	function_context.llvm_ir_builder.CreateBr( function_context.coro_final_suspend_bb );
}

Value CodeBuilder::BuildAwait( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression, const SrcLoc& src_loc )
{
	const VariablePtr async_func_variable= BuildExpressionCodeEnsureVariable( expression, names_scope, function_context );
	if( async_func_variable->type == invalid_type_ )
		return ErrorValue();

	if( async_func_variable->value_type != ValueType::Value )
	{
		REPORT_ERROR( ImmediateValueExpectedInAwaitOperator, names_scope.GetErrors(), src_loc );
		return ErrorValue();
	}

	const Class* const class_type= async_func_variable->type.GetClassType();
	if( class_type == nullptr )
	{
		REPORT_ERROR( AwaitForNonAsyncFunctionValue, names_scope.GetErrors(), src_loc );
		return ErrorValue();
	}

	const auto coroutine_type_description= std::get_if< CoroutineTypeDescription >( &class_type->generated_class_data );
	if( coroutine_type_description == nullptr || coroutine_type_description->kind != CoroutineKind::AsyncFunc )
	{
		REPORT_ERROR( AwaitForNonAsyncFunctionValue, names_scope.GetErrors(), src_loc );
		return ErrorValue();
	}

	if( function_context.coro_suspend_bb == nullptr )
	{
		REPORT_ERROR( AwaitOutsideAsyncFunction, names_scope.GetErrors(), src_loc );
		return ErrorValue();
	}
	if( const auto function_class_type= function_context.function_type.return_type.GetClassType() )
	{
		if( const auto function_coroutine_type_description= std::get_if<CoroutineTypeDescription>( &function_class_type->generated_class_data ) )
		{
			if( function_coroutine_type_description->kind != CoroutineKind::AsyncFunc )
			{
				// Prevent usage of "await" in generators.
				REPORT_ERROR( AwaitOutsideAsyncFunction, names_scope.GetErrors(), src_loc );
				return ErrorValue();
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

		llvm::LoadInst* const coro_handle=
			function_context.llvm_ir_builder.CreateLoad( llvm::PointerType::get( llvm_context_, 0 ), async_func_variable->llvm_value, false, "coro_handle" );

		coro_handle->setMetadata( llvm::StringRef("u_await_coro_handle"), llvm::MDNode::get( llvm_context_, {} ) );

		llvm::Value* const done= function_context.llvm_ir_builder.CreateCall(
			llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_done ),
			{ coro_handle },
			"coro_done" );

		function_context.llvm_ir_builder.CreateCondBr( done, already_done_block, loop_block );

		// Already done block.
		already_done_block->insertInto( function_context.function );
		function_context.llvm_ir_builder.SetInsertPoint( already_done_block );
		// Halt if coroutine is already finished. There is no other way to create a fallback in such case.
		// Normally this should not happen - in most case "await" operator should be used directly for async function call result.
		function_context.llvm_ir_builder.CreateCall( halt_func_ );
		function_context.llvm_ir_builder.CreateUnreachable();

		// Loop block.
		loop_block->insertInto( function_context.function );
		function_context.llvm_ir_builder.SetInsertPoint( loop_block );

		llvm::CallInst* const resume_call= function_context.llvm_ir_builder.CreateCall(
			llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_resume ),
			{ coro_handle } );

		resume_call->setMetadata( llvm::StringRef("u_await_resume"), llvm::MDNode::get( llvm_context_, {} ) );

		llvm::Value* const done_after_resume= function_context.llvm_ir_builder.CreateCall(
			llvm::Intrinsic::getDeclaration( module_.get(), llvm::Intrinsic::coro_done ),
			{ coro_handle },
			"coro_done_after_resume" );

		function_context.llvm_ir_builder.CreateCondBr( done_after_resume, done_block, not_done_block );

		// Not done block.
		not_done_block->insertInto( function_context.function );
		function_context.llvm_ir_builder.SetInsertPoint( not_done_block );

		// TODO - perform context save independent on suspend?
		CoroutineSuspend( names_scope, function_context, src_loc );
		function_context.llvm_ir_builder.CreateBr( loop_block ); // Continue to check if the coroutine is done.

		// Done block.
		done_block->insertInto( function_context.function );
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
					llvm::ConstantInt::get( llvm_context_, llvm::APInt( 32u, data_layout_.getABITypeAlign( promise_llvm_type ).value() ) ),
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
				U_ASSERT( param_reference.second != FunctionType::c_param_reference_number );
				if( param_reference.second < async_func_variable->inner_reference_nodes.size() )
					function_context.variables_state.TryAddLink(
						async_func_variable->inner_reference_nodes[param_reference.second],
						result->inner_reference_nodes[i],
						names_scope.GetErrors(),
						src_loc );
			}
		}
	}
	else
	{
		for( const FunctionType::ParamReference& param_reference : coroutine_type_description->return_references )
		{
			U_ASSERT( param_reference.first == 0u );
			U_ASSERT( param_reference.second != FunctionType::c_param_reference_number );
			if( param_reference.second < async_func_variable->inner_reference_nodes.size() )
				function_context.variables_state.TryAddLink( async_func_variable->inner_reference_nodes[param_reference.second], result, names_scope.GetErrors(), src_loc );
		}
	}

	// Move async function value, call destructor and create lifetime end.
	// TODO - does it make sense to call a destructor for finished coroutine?

	function_context.variables_state.MoveNode( async_func_variable );

	if( !function_context.is_functionless_context )
	{
		U_ASSERT( async_func_variable->type.HasDestructor() );

		const NamesScopeValue* const destructor_value= class_type->members->GetThisScopeValue( Keyword( Keywords::destructor_ ) );
		U_ASSERT( destructor_value != nullptr );
		const OverloadedFunctionsSetConstPtr destructors= destructor_value->value.GetFunctionsSet();
		U_ASSERT(destructors != nullptr && destructors->functions.size() == 1u );

		const FunctionVariable& destructor= destructors->functions.front();
		llvm::CallInst* const call_instruction= function_context.llvm_ir_builder.CreateCall( EnsureLLVMFunctionCreated( destructor ), { async_func_variable->llvm_value } );
		call_instruction->setCallingConv( GetLLVMCallingConvention( destructor.type.calling_convention ) );

		call_instruction->setMetadata( llvm::StringRef("u_await_destructor_call"), llvm::MDNode::get( llvm_context_, {} ) );

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

	destroy_block->insertInto( function_context.function );
	function_context.llvm_ir_builder.SetInsertPoint( destroy_block );
	{
		ReferencesGraph references_graph= function_context.variables_state;
		CallDestructorsBeforeReturn( names_scope, function_context, src_loc );
		function_context.variables_state= std::move(references_graph);
	}
	function_context.llvm_ir_builder.CreateBr( function_context.coro_cleanup_bb );

	next_block->insertInto( function_context.function );
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
