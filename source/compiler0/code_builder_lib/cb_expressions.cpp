#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/MD5.h>
#include <llvm/Support/ConvertUTF.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "../../code_builder_lib_common/source_file_contents_hash.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

#define CHECK_RETURN_ERROR_VALUE(value) if( value.GetErrorValue() != nullptr ) { return value; }

namespace U
{

namespace
{

bool AreCharArraysWithSameElementType( const Type& l, const Type& r )
{
	if( const auto l_array= l.GetArrayType() )
		if( const auto r_array= r.GetArrayType() )
			if( l_array->element_type == r_array->element_type )
				if( const auto fundamental_type= l_array->element_type.GetFundamentalType() )
					if( IsChar( fundamental_type->fundamental_type ) )
						return true;

	return false;
}

} // namespace

VariablePtr CodeBuilder::BuildExpressionCodeEnsureVariable(
	const Synt::Expression& expression,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	Value result= BuildExpressionCode( expression, names_scope, function_context );

	const VariablePtr result_variable= result.GetVariable();
	if( result_variable != nullptr )
		return result_variable;

	OverloadedFunctionsSetPtr functions_set= result.GetFunctionsSet();
	if( const auto this_overloaded_methods_set= result.GetThisOverloadedMethodsSet() )
		functions_set= this_overloaded_methods_set->overloaded_methods_set;

	if( functions_set != nullptr && functions_set->functions.size() == 1 && functions_set->template_functions.empty() )
	{
		// If an variable is expected but given value is a functions set with exactly one non-template function,
		// create function pointer for this function.
		FunctionVariable& function= functions_set->functions.front();
		function.referenced= true;

		FunctionPointerType fp;
		fp.function_type= function.type;
		fp.llvm_type= llvm::PointerType::get( llvm_context_, 0 );

		const auto llvm_function= EnsureLLVMFunctionCreated( function );

		const VariablePtr function_pointer=
			Variable::Create(
				std::move(fp),
				ValueType::Value,
				Variable::Location::LLVMRegister,
				"single function pointer",
				llvm_function,
				llvm_function );

		function_context.variables_state.AddNode( function_pointer );
		RegisterTemporaryVariable( function_context, function_pointer );
		return function_pointer;
	}

	if( result.GetErrorValue() == nullptr )
		REPORT_ERROR( ExpectedVariable, names_scope.GetErrors(), Synt::GetSrcLoc( expression ), result.GetKindName() );

	const VariablePtr dummy_result=
		Variable::Create(
			invalid_type_,
			ValueType::Value,
			Variable::Location::Pointer,
			"error value",
			llvm::UndefValue::get( invalid_type_.GetLLVMType()->getPointerTo() ) );

	function_context.variables_state.AddNode( dummy_result );
	RegisterTemporaryVariable( function_context, dummy_result );

	return dummy_result;
}

VariablePtr CodeBuilder::BuildExpressionCodeForValueReturn(
	const Synt::Expression& expression,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	// Try to automatically move local variable in return.
	if(const auto name_lookup= std::get_if<Synt::NameLookup>( &expression ) )
	{
		VariablePtr resolved_variable;
		if( name_lookup->name == Keywords::this_ )
		{
			if( function_context.this_ != nullptr&& !function_context.whole_this_is_unavailable )
				resolved_variable= function_context.this_;
		}
		else
		{
			NamesScopeValue* const resolved_value_ptr= LookupName( names_scope, name_lookup->name, name_lookup->src_loc ).value;
			if( resolved_value_ptr != nullptr )
			{
				resolved_value_ptr->referenced= true;
				CollectDefinition( *resolved_value_ptr, name_lookup->src_loc );

				const Value& resolved_value= resolved_value_ptr->value;
				resolved_variable= resolved_value.GetVariable();

				if( resolved_variable != nullptr && function_context.lambda_preprocessing_context != nullptr )
				{
					LambdaPreprocessingCheckVariableUsage( names_scope, function_context, resolved_variable, name_lookup->name, name_lookup->src_loc );
					if( function_context.lambda_preprocessing_context->external_variables.count( resolved_variable ) > 0 )
					{
						// Do not try to auto-move lambda external variables.
						return LambdaPreprocessingAccessExternalVariable( function_context, resolved_variable, name_lookup->name );
					}
				}
			}
		}

		if( resolved_variable != nullptr &&
			// Enable auto-move in "return" for immutable local variables too.
			(resolved_variable->value_type == ValueType::ReferenceMut || resolved_variable->value_type == ValueType::ReferenceImut) &&
			!IsGlobalVariable( resolved_variable ) &&
			!function_context.variables_state.HasOutgoingLinks( resolved_variable ) &&
			!function_context.variables_state.NodeMoved( resolved_variable ) )
		{
			const auto input_nodes= function_context.variables_state.GetNodeInputLinks( resolved_variable );
			if( input_nodes.size() == 1u )
			{
				const VariablePtr variable_for_move= *input_nodes.begin();
				if( variable_for_move->value_type == ValueType::Value ) // It's a variable, not a reference.
				{
					bool found_in_variables= false;
					for( const auto& stack_frame : function_context.stack_variables_stack )
					for( const VariablePtr& arg : stack_frame->variables_ )
					{
						if( arg == variable_for_move )
						{
							found_in_variables= true;
							goto end_variable_search;
						}
					}

					end_variable_search:
					if( found_in_variables )
					{
						U_ASSERT( !function_context.variables_state.NodeMoved( variable_for_move ) );

						const VariablePtr result=
							Variable::Create(
								variable_for_move->type,
								ValueType::Value,
								variable_for_move->location,
								"_moved_" + variable_for_move->name,
								variable_for_move->llvm_value,
								// Preserve "constexpr" value (if has one) for immutable variables.
								resolved_variable->value_type == ValueType::ReferenceImut ? variable_for_move->constexpr_value : nullptr );
						function_context.variables_state.AddNode( result );

						function_context.variables_state.TryAddInnerLinks( resolved_variable, result, names_scope.GetErrors(), name_lookup->src_loc );

						// Move both reference node and variable node.
						function_context.variables_state.MoveNode( resolved_variable );
						function_context.variables_state.MoveNode( variable_for_move );

						RegisterTemporaryVariable( function_context, result );
						return result;
					}
				}
			}
		}
	}

	return BuildExpressionCodeEnsureVariable( expression, names_scope, function_context );
}

Value CodeBuilder::BuildExpressionCode(
	const Synt::Expression& expression,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	return
		std::visit(
			[&]( const auto& t )
			{
				return BuildExpressionCodeImpl( names_scope, function_context, t );
			},
			expression );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext&,
	const Synt::EmptyVariant& )
{
	// This should not happens in normal situation - when syntax is valid.
	// But for calls from language server we need to handle this case somehow.
	REPORT_ERROR( NotImplemented, names_scope.GetErrors(), SrcLoc(), "empty expression" );
	return ErrorValue();
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::CallOperator& call_operator )
{
	const Value function_value= BuildExpressionCode( call_operator.expression, names_scope, function_context );
	CHECK_RETURN_ERROR_VALUE(function_value);

	std::optional<SrcLoc> value_src_loc;
	if( collect_definition_points_ )
	{
		// Choose src_loc for definitions collection.
		// In most cases proper src_loc is just src_loc of NameLookup/NamesScopeNameFetch, which is proper function name lexem.
		// But this also may be call to template function with provided template args.
		// In such case extract underlying name.
		if( const auto template_parameterization= std::get_if< std::unique_ptr< const Synt::TemplateParameterization > >( &call_operator.expression ) )
			value_src_loc= Synt::GetSrcLoc( (*template_parameterization)->base );
		else
			value_src_loc= Synt::GetSrcLoc(call_operator.expression);
	}

	return CallFunctionValue( function_value, call_operator.arguments, call_operator.src_loc, value_src_loc, names_scope, function_context );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::CallOperatorSignatureHelp& call_operator_signature_help )
{
	PerformSignatureHelp( BuildExpressionCode( call_operator_signature_help.expression, names_scope, function_context ) );
	return ErrorValue();
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::IndexationOperator& indexation_operator )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( indexation_operator.expression, names_scope, function_context );

	if( variable->type.GetClassType() != nullptr ) // If this is class - try call overloaded [] operator.
	{
		if( auto res=
				TryCallOverloadedPostfixOperator(
					variable,
					llvm::ArrayRef<Synt::Expression>(indexation_operator.index),
					OverloadedOperator::Indexing,
					indexation_operator.src_loc,
					names_scope,
					function_context ) )
			return std::move(*res);

		REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), indexation_operator.src_loc, variable->type );
		return ErrorValue();
	}

	if( const ArrayType* const array_type= variable->type.GetArrayType() )
	{
		// Lock variable. We must prevent modification of this variable in index calcualtion.
		// Do not forget to unregister it in case of error-return!
		const VariableMutPtr variable_lock=
			Variable::Create(
				variable->type,
				variable->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				variable->location,
				variable->name + " lock" );
		function_context.variables_state.AddNode( variable_lock );
		function_context.variables_state.TryAddLink( variable, variable_lock, names_scope.GetErrors(), indexation_operator.src_loc );
		function_context.variables_state.TryAddInnerLinks( variable, variable_lock, names_scope.GetErrors(), indexation_operator.src_loc );

		variable_lock->preserve_temporary= true;
		RegisterTemporaryVariable( function_context, variable_lock );

		const VariablePtr index= BuildExpressionCodeEnsureVariable( indexation_operator.index, names_scope, function_context );

		const FundamentalType* const index_fundamental_type= index->type.GetFundamentalType();
		if( !( index_fundamental_type != nullptr && (
			( index->constexpr_value != nullptr && IsInteger( index_fundamental_type->fundamental_type ) ) ||
			( index->constexpr_value == nullptr && IsUnsignedInteger( index_fundamental_type->fundamental_type ) ) ) ) )
		{
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), indexation_operator.src_loc, "any integer", index->type );
			return ErrorValue();
		}

		// If index is constant and not undefined statically check index.
		if( index->constexpr_value != nullptr )
		{
			const llvm::APInt index_value= index->constexpr_value->getUniqueInteger();
			if( IsSignedInteger(index_fundamental_type->fundamental_type) )
			{
				if( index_value.getLimitedValue() >= array_type->element_count || index_value.isNegative() )
					REPORT_ERROR( ArrayIndexOutOfBounds, names_scope.GetErrors(), indexation_operator.src_loc, index_value.getSExtValue(), array_type->element_count );
			}
			else
			{
				if( index_value.getLimitedValue() >= array_type->element_count )
					REPORT_ERROR( ArrayIndexOutOfBounds, names_scope.GetErrors(), indexation_operator.src_loc, index_value.getLimitedValue(), array_type->element_count );
			}
		}

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_value= CreateMoveToLLVMRegisterInstruction( *index, function_context );

		DestroyUnusedTemporaryVariables( function_context, names_scope.GetErrors(), indexation_operator.src_loc ); // Destroy temporaries of index expression.

		const VariableMutPtr result=
			Variable::Create(
				array_type->element_type,
				variable->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				variable->name + "[]" );

		if( variable->constexpr_value != nullptr && index->constexpr_value != nullptr )
			result->constexpr_value= variable->constexpr_value->getAggregateElement( index->constexpr_value );

		// If index is not constant - check bounds.
		if( index->constexpr_value == nullptr && !function_context.is_functionless_context )
		{
			const uint64_t index_size= GetFundamentalTypeSize( index_fundamental_type->fundamental_type );
			const uint64_t size_type_size= GetFundamentalTypeSize( U_FundamentalType::size_type_ );
			if( index_size > size_type_size )
				index_value= function_context.llvm_ir_builder.CreateTrunc( index_value, fundamental_llvm_types_.size_type_ );
			else if( index_size < size_type_size )
				index_value= function_context.llvm_ir_builder.CreateZExt( index_value, fundamental_llvm_types_.size_type_ );

			llvm::Value* const condition=
				function_context.llvm_ir_builder.CreateICmpUGE( // if( index >= array_size ) {halt;}
					index_value,
					llvm::Constant::getIntegerValue( fundamental_llvm_types_.size_type_, llvm::APInt( fundamental_llvm_types_.size_type_->getIntegerBitWidth(), array_type->element_count ) ) );

			llvm::BasicBlock* const halt_block= llvm::BasicBlock::Create( llvm_context_ );
			llvm::BasicBlock* const block_after_if= llvm::BasicBlock::Create( llvm_context_ );
			function_context.llvm_ir_builder.CreateCondBr( condition, halt_block, block_after_if );

			halt_block->insertInto( function_context.function );
			function_context.llvm_ir_builder.SetInsertPoint( halt_block );
			function_context.llvm_ir_builder.CreateCall( halt_func_ );
			function_context.llvm_ir_builder.CreateUnreachable(); // terminate block.

			block_after_if->insertInto( function_context.function );
			function_context.llvm_ir_builder.SetInsertPoint( block_after_if );
		}

		result->llvm_value= CreateArrayElementGEP( function_context, *variable, index_value );

		function_context.variables_state.AddNode( result );
		function_context.variables_state.TryAddLink( variable_lock, result, names_scope.GetErrors(), indexation_operator.src_loc );
		function_context.variables_state.TryAddInnerLinks( variable_lock, result, names_scope.GetErrors(), indexation_operator.src_loc );

		function_context.variables_state.MoveNode( variable_lock );

		RegisterTemporaryVariable( function_context, result );
		return result;
	}
	else if( const TupleType* const tuple_type= variable->type.GetTupleType() )
	{
		VariablePtr index;
		// Create separate variables storage for index calculation.
		// This is needed to prevent destruction of "variable" during index calculation without creating lock node and reference to it.
		// This is needed to properly access multiple mutable child nodes of same tuple variable.
		{
			const StackVariablesStorage temp_variables_storage( function_context );
			index= BuildExpressionCodeEnsureVariable( indexation_operator.index, names_scope, function_context );
			CallDestructors( temp_variables_storage, names_scope, function_context, indexation_operator.src_loc );
			// It is fine if "index" will be destroyed here. We needed only "constexpr" value of index here.
		}

		const FundamentalType* const index_fundamental_type= index->type.GetFundamentalType();
		if( index_fundamental_type == nullptr || !IsInteger( index_fundamental_type->fundamental_type ) )
		{
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), indexation_operator.src_loc, "any integer", index->type );
			return ErrorValue();
		}

		if( variable->location != Variable::Location::Pointer )
		{
			// TODO - Strange variable location.
			return ErrorValue();
		}

		// For tuple indexing only constexpr indices are valid.
		if( index->constexpr_value == nullptr )
		{
			REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), indexation_operator.src_loc );
			return ErrorValue();
		}
		const llvm::APInt index_value_raw= index->constexpr_value->getUniqueInteger();
		const uint64_t index_value= index_value_raw.getLimitedValue();
		if( IsSignedInteger(index_fundamental_type->fundamental_type) )
		{
			if( index_value >= uint64_t(tuple_type->element_types.size()) || index_value_raw.isNegative() )
			{
				REPORT_ERROR( TupleIndexOutOfBounds, names_scope.GetErrors(), indexation_operator.src_loc, index_value_raw.getSExtValue(), tuple_type->element_types.size() );
				return ErrorValue();
			}
		}
		else
		{
			if( index_value >= uint64_t(tuple_type->element_types.size()) )
			{
				REPORT_ERROR( TupleIndexOutOfBounds, names_scope.GetErrors(), indexation_operator.src_loc, index_value, tuple_type->element_types.size() );
				return ErrorValue();
			}
		}

		variable->children.resize( size_t(tuple_type->llvm_type->getNumElements()), nullptr );
		if( const auto prev_node= variable->children[ size_t(index_value) ] )
		{
			function_context.variables_state.AddNodeIfNotExists( prev_node );
			return prev_node; // Child node already created.
		}

		// Create variable child node.

		const VariableMutPtr result=
			Variable::CreateChildNode(
				variable,
				tuple_type->element_types[size_t(index_value)],
				variable->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				variable->name + "[" + std::to_string(index_value) + "]",
				ForceCreateConstantIndexGEP( function_context, tuple_type->llvm_type, variable->llvm_value, uint32_t(index_value) ) );

		if( const size_t element_type_reference_tag_count= result->type.ReferenceTagCount(); element_type_reference_tag_count != 0 )
		{
			size_t offset= 0;
			for( size_t i= 0; i < size_t(index_value); ++i )
				offset+= tuple_type->element_types[i].ReferenceTagCount();

			U_ASSERT( offset <= variable->inner_reference_nodes.size() );
			U_ASSERT( offset + element_type_reference_tag_count <= variable->inner_reference_nodes.size() );
			U_ASSERT( result->inner_reference_nodes.size() == element_type_reference_tag_count );
			for( size_t i= 0; i < element_type_reference_tag_count; ++i )
				result->inner_reference_nodes[i]= variable->inner_reference_nodes[i + offset];
		}

		if( variable->constexpr_value != nullptr )
			result->constexpr_value= variable->constexpr_value->getAggregateElement( uint32_t(index_value) );

		variable->children[ size_t(index_value) ]= result;

		function_context.variables_state.AddNode( result );

		return result;
	}
	else
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), indexation_operator.src_loc, variable->type );
		return ErrorValue();
	}
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::MemberAccessOperator& member_access_operator )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( member_access_operator.expression, names_scope, function_context );

	Class* const class_type= variable->type.GetClassType();
	if( class_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), member_access_operator.src_loc, variable->type );
		return ErrorValue();
	}

	if( !EnsureTypeComplete( variable->type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), member_access_operator.src_loc, variable->type );
		return ErrorValue();
	}

	// Try to perform lazy typeinfo field fetch.
	if( std::holds_alternative< TypeinfoClassDescription >( class_type->generated_class_data ) &&
		!member_access_operator.has_template_args )
	{
		if( const VariablePtr fetch_result= TryFetchTypeinfoClassLazyField( variable->type, member_access_operator.member_name ) )
		{
			U_ASSERT( fetch_result->constexpr_value != nullptr );
			function_context.variables_state.AddNodeIfNotExists( fetch_result );

			if( variable->constexpr_value != nullptr )
				return fetch_result; // Pass constexpr fetch result for constexpr typeinfo variable.
			else
			{
				// This is a typeinfo element access via non-constexpr typeinfo variable.
				// Since typeinfo classes have no constructors it's impossible to construct typeinfo variable.
				// So, assume this instance is instance of single true (generated) typeinfo variable.
				// It is still possible to create value of typeinfo class via unsafe-hacks, but ignore such possibility and return one legit possible value.
				//
				// Note that we create here immutable reference, even if source variable is mutable.
				// So, typeinfo list pseudo-field behaves like immutable field.
				// Doing so we prevent possible modification of returned variable, since it is de-facto global constant and thus can't be modified.

				const VariablePtr non_constexpr_ref=
					Variable::Create(
						fetch_result->type,
						ValueType::ReferenceImut,
						Variable::Location::Pointer,
						fetch_result->name,
						fetch_result->llvm_value,
						nullptr ); // Create reference node with null constexpr value, since source variable is not constexpr.

				function_context.variables_state.AddNode( non_constexpr_ref );
				function_context.variables_state.TryAddLink( fetch_result, non_constexpr_ref, names_scope.GetErrors(), member_access_operator.src_loc );
				function_context.variables_state.TryAddInnerLinks( fetch_result, non_constexpr_ref, names_scope.GetErrors(), member_access_operator.src_loc );

				RegisterTemporaryVariable( function_context, non_constexpr_ref );
				return non_constexpr_ref;
			}
		}
	}

	const auto class_value= ResolveClassValue( class_type, member_access_operator.member_name );
	NamesScopeValue* const class_member= class_value.first;
	if( class_member == nullptr )
	{
		REPORT_ERROR( NameNotFound, names_scope.GetErrors(), member_access_operator.src_loc, member_access_operator.member_name );
		return ErrorValue();
	}

	CollectDefinition( *class_member, member_access_operator.src_loc );

	if( !function_context.is_in_unsafe_block &&
		( member_access_operator.member_name == Keywords::constructor_ || member_access_operator.member_name == Keywords::destructor_ ) )
		REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names_scope.GetErrors(), member_access_operator.src_loc,  member_access_operator.member_name );

	if( names_scope.GetAccessFor( variable->type.GetClassType() ) < class_value.second )
		REPORT_ERROR( AccessingNonpublicClassMember, names_scope.GetErrors(), member_access_operator.src_loc, member_access_operator.member_name, class_type->members->ToString() );

	if( const OverloadedFunctionsSetPtr functions_set= class_member->value.GetFunctionsSet() )
	{
		PrepareFunctionsSetAndBuildConstexprBodies( *class_type->members, *functions_set );

		ThisOverloadedMethodsSet this_overloaded_methods_set;
		this_overloaded_methods_set.this_= variable;
		this_overloaded_methods_set.overloaded_methods_set= functions_set;

		if( member_access_operator.has_template_args )
		{
			if( functions_set->template_functions.empty() )
				REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), member_access_operator.src_loc );
			else
			{
				const OverloadedFunctionsSetPtr parameterized_functions=
					ParameterizeFunctionTemplate(
						member_access_operator.src_loc,
						functions_set,
						member_access_operator.template_args,
						names_scope,
						function_context );

				if( parameterized_functions != nullptr )
					this_overloaded_methods_set.overloaded_methods_set= parameterized_functions;
			}
		}

		return std::move(this_overloaded_methods_set);
	}

	if( member_access_operator.has_template_args )
		REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), member_access_operator.src_loc );

	if( const ClassFieldPtr field= class_member->value.GetClassField() )
	{
		class_member->referenced= true;
		return AccessClassField( names_scope, function_context, variable, *field, member_access_operator.member_name, member_access_operator.src_loc );
	}

	REPORT_ERROR( NotImplemented, names_scope.GetErrors(), member_access_operator.src_loc, "class members, except fields or methods" );
	return ErrorValue();
}

Value CodeBuilder::BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::MemberAccessOperatorCompletion& member_access_operator_completion )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( member_access_operator_completion.expression, names_scope, function_context );
	MemberAccessCompleteImpl( variable, member_access_operator_completion.member_name );
	return ErrorValue();
}

Value CodeBuilder::BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::VariableInitialization& variable_initialization )
{
	const Value type_value= BuildExpressionCode( variable_initialization.type, names_scope, function_context );
	CHECK_RETURN_ERROR_VALUE(type_value)

	const Type* const type= type_value.GetTypeName();
	if(type == nullptr)
	{
		REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), variable_initialization.src_loc, type_value.GetKindName() );
		return ErrorValue();
	}

	if( !EnsureTypeComplete( *type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), variable_initialization.src_loc, type );
		return ErrorValue();
	}
	else if( type->IsAbstract() )
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names_scope.GetErrors(), variable_initialization.src_loc, type );

	const VariableMutPtr variable=
		Variable::Create(
			*type,
			ValueType::Value,
			Variable::Location::Pointer,
			"temp " + type->ToString() );
	function_context.variables_state.AddNode( variable );

	if( !function_context.is_functionless_context )
	{
		variable->llvm_value= function_context.alloca_ir_builder.CreateAlloca( type->GetLLVMType() );
		CreateLifetimeStart( function_context, variable->llvm_value );
	}

	{
		const VariablePtr variable_for_initialization=
			Variable::Create(
				*type,
				ValueType::ReferenceMut,
				Variable::Location::Pointer,
				variable->name,
				variable->llvm_value );
		function_context.variables_state.AddNode( variable_for_initialization );
		function_context.variables_state.AddLink( variable, variable_for_initialization );
		function_context.variables_state.TryAddInnerLinks( variable, variable_for_initialization, names_scope.GetErrors(), variable_initialization.src_loc );

		variable->constexpr_value= ApplyInitializer( variable_for_initialization, names_scope, function_context, variable_initialization.initializer );

		function_context.variables_state.RemoveNode( variable_for_initialization );
	}

	RegisterTemporaryVariable( function_context, variable );
	return variable;
}

Value CodeBuilder::BuildExpressionCodeImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::AwaitOperator& await_operator )
{
	return BuildAwait( names_scope, function_context, await_operator.expression, await_operator.src_loc );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::UnaryMinus& unary_minus )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( unary_minus.expression, names_scope, function_context );

	if( auto res= TryCallOverloadedUnaryOperator( variable, OverloadedOperator::Sub, unary_minus.src_loc, names_scope, function_context ) )
		return std::move(*res);

	const FundamentalType* const fundamental_type= variable->type.GetFundamentalType();
	if( fundamental_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), unary_minus.src_loc, variable->type );
		return ErrorValue();
	}

	const bool is_float= IsFloatingPoint( fundamental_type->fundamental_type );
	if( !( IsInteger( fundamental_type->fundamental_type ) || is_float ) )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), unary_minus.src_loc, variable->type );
		return ErrorValue();
	}
	// TODO - maybe not support unary minus for 8 and 16 bit integer types?

	const VariableMutPtr result=
		Variable::Create(
			variable->type,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			std::string( OverloadedOperatorToString(OverloadedOperator::Sub) ) );

	if( llvm::Value* const value_for_neg= CreateMoveToLLVMRegisterInstruction( *variable, function_context ) )
	{
		if( is_float )
			result->llvm_value= function_context.llvm_ir_builder.CreateFNeg( value_for_neg );
		else
			result->llvm_value= function_context.llvm_ir_builder.CreateNeg( value_for_neg );

		result->constexpr_value= llvm::dyn_cast<llvm::Constant>(result->llvm_value);
	}

	function_context.variables_state.AddNode( result );
	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::LogicalNot& logical_not )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( logical_not.expression, names_scope, function_context );

	if( auto res= TryCallOverloadedUnaryOperator( variable, OverloadedOperator::LogicalNot, logical_not.src_loc, names_scope, function_context ) )
		return std::move(*res);

	if( variable->type != bool_type_ )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), logical_not.src_loc, variable->type );
		return ErrorValue();
	}

	const VariableMutPtr result=
		Variable::Create(
			variable->type,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			std::string( OverloadedOperatorToString(OverloadedOperator::LogicalNot) ) );

	if( const auto src_val= CreateMoveToLLVMRegisterInstruction( *variable, function_context ) )
	{
		result->llvm_value= function_context.llvm_ir_builder.CreateNot( src_val );
		result->constexpr_value= llvm::dyn_cast<llvm::Constant>( result->llvm_value );
	}

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::BitwiseNot& bitwise_not )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( bitwise_not.expression, names_scope, function_context );

	if( auto res= TryCallOverloadedUnaryOperator( variable, OverloadedOperator::BitwiseNot, bitwise_not.src_loc, names_scope, function_context ) )
		return std::move(*res);

	const FundamentalType* const fundamental_type= variable->type.GetFundamentalType();
	if( fundamental_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), bitwise_not.src_loc, variable->type );
		return ErrorValue();
	}
	if( !IsInteger( fundamental_type->fundamental_type ) )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), bitwise_not.src_loc, variable->type );
		return ErrorValue();
	}

	const VariableMutPtr result=
		Variable::Create(
			variable->type,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			std::string( OverloadedOperatorToString(OverloadedOperator::BitwiseNot) ) );

	if( const auto src_val= CreateMoveToLLVMRegisterInstruction( *variable, function_context ) )
	{
		result->llvm_value= function_context.llvm_ir_builder.CreateNot( src_val );
		result->constexpr_value= llvm::dyn_cast<llvm::Constant>(result->llvm_value);
	}

	function_context.variables_state.AddNode( result);

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::BinaryOperator& binary_operator	)
{
	if( binary_operator.operator_type == BinaryOperatorType::LazyLogicalAnd ||
		binary_operator.operator_type == BinaryOperatorType::LazyLogicalOr )
	{
		return
			BuildLazyBinaryOperator(
				binary_operator.left,
				binary_operator.right,
				binary_operator,
				binary_operator.src_loc,
				names_scope,
				function_context );
	}

	const auto overloaded_operator= GetOverloadedOperatorForBinaryOperator( binary_operator.operator_type );

	std::optional<Value> overloaded_operator_call_try=
		TryCallOverloadedBinaryOperator(
			overloaded_operator,
			binary_operator.left, binary_operator.right,
			false,
			binary_operator.src_loc,
			names_scope,
			function_context );
	if( overloaded_operator_call_try != std::nullopt )
	{
		if( const VariablePtr call_variable= overloaded_operator_call_try->GetVariable())
		{
			if( binary_operator.operator_type == BinaryOperatorType::NotEqual && call_variable->type == bool_type_ )
			{
				const VariableMutPtr variable=
					Variable::Create(
						bool_type_,
						ValueType::Value,
						Variable::Location::LLVMRegister,
						std::string( OverloadedOperatorToString( overloaded_operator ) ) );

				// "!=" is implemented via "==", so, invert result.
				if( const auto call_value_in_register= CreateMoveToLLVMRegisterInstruction( *call_variable, function_context ) )
				{
					variable->llvm_value= function_context.llvm_ir_builder.CreateNot( call_value_in_register );
					variable->constexpr_value= llvm::dyn_cast<llvm::Constant>( variable->llvm_value );
				}

				function_context.variables_state.AddNode( variable );
				RegisterTemporaryVariable( function_context, variable );
				return variable;

			}
			else if( overloaded_operator == OverloadedOperator::CompareOrder &&
				binary_operator.operator_type != BinaryOperatorType::CompareOrder &&
				call_variable->type.GetFundamentalType() != nullptr &&
				IsSignedInteger( call_variable->type.GetFundamentalType()->fundamental_type ) )
			{
				const VariableMutPtr variable=
					Variable::Create(
						bool_type_,
						ValueType::Value,
						Variable::Location::LLVMRegister,
						std::string( OverloadedOperatorToString( overloaded_operator ) ) );

				if( const auto call_value_in_register= CreateMoveToLLVMRegisterInstruction( *call_variable, function_context ) )
				{
					const auto zero= llvm::Constant::getNullValue( call_variable->type.GetLLVMType() );
					if( binary_operator.operator_type == BinaryOperatorType::Less )
						variable->llvm_value= function_context.llvm_ir_builder.CreateICmpSLT( call_value_in_register, zero );
					else if( binary_operator.operator_type == BinaryOperatorType::LessEqual )
						variable->llvm_value= function_context.llvm_ir_builder.CreateICmpSLE( call_value_in_register, zero );
					else if( binary_operator.operator_type == BinaryOperatorType::Greater )
						variable->llvm_value= function_context.llvm_ir_builder.CreateICmpSGT( call_value_in_register, zero );
					else if( binary_operator.operator_type == BinaryOperatorType::GreaterEqual )
						variable->llvm_value= function_context.llvm_ir_builder.CreateICmpSGE( call_value_in_register, zero );
					else U_ASSERT(false);

					variable->constexpr_value= llvm::dyn_cast<llvm::Constant>( variable->llvm_value );
				}

				function_context.variables_state.AddNode( variable );
				RegisterTemporaryVariable( function_context, variable );
				return variable;
			}
		}

		return std::move(*overloaded_operator_call_try);
	}

	VariablePtr l_var=
		BuildExpressionCodeEnsureVariable(
			binary_operator.left,
			names_scope,
			function_context );

	if( l_var->type.GetFundamentalType() != nullptr ||
		l_var->type.GetEnumType() != nullptr ||
		l_var->type.GetRawPointerType() != nullptr ||
		l_var->type.GetFunctionPointerType() != nullptr )
	{
		// Save l_var in register, because build-in binary operators require value-parameters.
		l_var=
			Variable::Create(
				l_var->type,
				ValueType::Value,
				Variable::Location::LLVMRegister,
				l_var->name + " in register",
				l_var->location == Variable::Location::LLVMRegister
					? l_var->llvm_value
					: CreateMoveToLLVMRegisterInstruction( *l_var, function_context ),
				l_var->constexpr_value );

		DestroyUnusedTemporaryVariables( function_context, names_scope.GetErrors(), binary_operator.src_loc );

		function_context.variables_state.AddNode( l_var );
		RegisterTemporaryVariable( function_context, l_var );
	}
	else
		DestroyUnusedTemporaryVariables( function_context, names_scope.GetErrors(), binary_operator.src_loc );

	const VariablePtr r_var=
		BuildExpressionCodeEnsureVariable(
			binary_operator.right,
			names_scope,
			function_context );

	return BuildBinaryOperator( *l_var, *r_var, binary_operator.operator_type, binary_operator.src_loc, names_scope, function_context );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::TernaryOperator& ternary_operator )
{
	const VariablePtr condition= BuildExpressionCodeEnsureVariable( ternary_operator.condition, names_scope, function_context );
	if( condition->type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), ternary_operator.src_loc, bool_type_, condition->type );
		return ErrorValue();
	}

	const auto condition_in_register= CreateMoveToLLVMRegisterInstruction( *condition, function_context );

	// Preevaluate branches for selection of type and value type for operator result.
	Type branches_types[2u];
	ValueType branches_value_types[2u];
	{
		const bool prev_is_functionless_context= function_context.is_functionless_context;
		function_context.is_functionless_context= true;
		for( size_t i= 0u; i < 2u; ++i )
		{
			const auto state= SaveFunctionContextState( function_context );
			{
				const StackVariablesStorage dummy_stack_variables_storage( function_context );
				const VariablePtr var= BuildExpressionCodeEnsureVariable( ternary_operator.branches[i], names_scope, function_context );
				branches_types[i]= var->type;
				branches_value_types[i]= var->value_type;
				DestroyUnusedTemporaryVariables( function_context, names_scope.GetErrors(), ternary_operator.src_loc );
			}
			RestoreFunctionContextState( function_context, state );
		}
		function_context.is_functionless_context= prev_is_functionless_context;
	}

	if( branches_types[0] != branches_types[1] )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), ternary_operator.src_loc, branches_types[0], branches_types[1] );
		return ErrorValue();
	}

	const VariableMutPtr result= Variable::Create(
		branches_types[0],
		ValueType::Value, // Set later
		Variable::Location::Pointer,
		"ternary_operator_result" );
	if( branches_value_types[0] == ValueType::Value || branches_value_types[1] == ValueType::Value )
	{
		result->value_type= ValueType::Value;
		if( !EnsureTypeComplete( result->type ) )
		{
			REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), ternary_operator.src_loc, result->type );
			return ErrorValue();
		}

		if( !function_context.is_functionless_context )
		{
			result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( result->type.GetLLVMType() );
			result->llvm_value->setName( "ternary_operator_result" );

			CreateLifetimeStart( function_context, result->llvm_value );
		}
	}
	else if( branches_value_types[0] == ValueType::ReferenceImut || branches_value_types[1] == ValueType::ReferenceImut )
		result->value_type= ValueType::ReferenceImut;
	else
		result->value_type= ValueType::ReferenceMut;

	// Do not forget to remove node in case of error-return!!!
	function_context.variables_state.AddNode( result );

	llvm::BasicBlock* result_block= nullptr;
	llvm::BasicBlock* branches_basic_blocks[2]{nullptr, nullptr};

	if( !function_context.is_functionless_context )
	{
		result_block= llvm::BasicBlock::Create( llvm_context_ );
		branches_basic_blocks[0]= llvm::BasicBlock::Create( llvm_context_ );
		branches_basic_blocks[1]= llvm::BasicBlock::Create( llvm_context_ );

		function_context.llvm_ir_builder.CreateCondBr( condition_in_register, branches_basic_blocks[0], branches_basic_blocks[1] );
	}

	llvm::Value* branches_reference_values[2] { nullptr, nullptr };
	llvm::Constant* branches_constexpr_values[2] { nullptr, nullptr };
	llvm::BasicBlock* branches_end_basic_blocks[2]{ nullptr, nullptr };
	ReferencesGraph variables_state_before= function_context.variables_state;
	ReferencesGraph branches_variables_state[2];
	for( size_t i= 0u; i < 2u; ++i )
	{
		function_context.variables_state= variables_state_before;
		{
			const StackVariablesStorage branch_temp_variables_storage( function_context );

			if( !function_context.is_functionless_context )
			{
				U_ASSERT( branches_basic_blocks[i] != nullptr );
				branches_basic_blocks[i]->insertInto( function_context.function );
				function_context.llvm_ir_builder.SetInsertPoint( branches_basic_blocks[i] );
			}

			const VariablePtr branch_result= BuildExpressionCodeEnsureVariable( ternary_operator.branches[i], names_scope, function_context );

			branches_constexpr_values[i]= branch_result->constexpr_value;

			function_context.variables_state.TryAddInnerLinks( branch_result, result, names_scope.GetErrors(), ternary_operator.src_loc );

			if( result->value_type == ValueType::Value )
			{
				// Move or create copy.
				if(
					result->type.GetFundamentalType() != nullptr ||
					result->type.GetEnumType() != nullptr ||
					result->type.GetRawPointerType() != nullptr ||
					result->type.GetFunctionPointerType() != nullptr )
					CreateTypedStore( function_context, result->type, CreateMoveToLLVMRegisterInstruction( *branch_result, function_context ), result->llvm_value );
				else if(
					result->type.GetClassType() != nullptr ||
					result->type.GetTupleType() != nullptr ||
					result->type.GetArrayType() != nullptr )
				{
					if( branch_result->value_type == ValueType::Value )
					{
						// Move.
						function_context.variables_state.MoveNode( branch_result );
						U_ASSERT( branch_result->location == Variable::Location::Pointer );
						if( !function_context.is_functionless_context )
						{
							CopyBytes( result->llvm_value, branch_result->llvm_value, result->type, function_context );
							CreateLifetimeEnd( function_context, branch_result->llvm_value );
						}
					}
					else
					{
						// Copy.
						if( !result->type.IsCopyConstructible() )
							REPORT_ERROR( CopyConstructValueOfNoncopyableType, names_scope.GetErrors(), ternary_operator.src_loc, result->type );
						else if( result->type.IsAbstract() )
							REPORT_ERROR( ConstructingAbstractClassOrInterface, names_scope.GetErrors(), ternary_operator.src_loc, result->type );
						else if( !function_context.is_functionless_context )
							BuildCopyConstructorPart( result->llvm_value, branch_result->llvm_value, result->type, function_context );
					}
				}
				else U_ASSERT( false );
			}
			else
			{
				branches_reference_values[i]= branch_result->llvm_value;
				function_context.variables_state.TryAddLink( branch_result, result, names_scope.GetErrors(), ternary_operator.src_loc );
			}

			CallDestructors( branch_temp_variables_storage, names_scope, function_context, Synt::GetSrcLoc( ternary_operator.branches[i] ) );

			if( !function_context.is_functionless_context )
				function_context.llvm_ir_builder.CreateBr( result_block );
		}
		branches_end_basic_blocks[i]= function_context.llvm_ir_builder.GetInsertBlock();
		branches_variables_state[i]= function_context.variables_state;
	}

	function_context.variables_state= MergeVariablesStateAfterIf( branches_variables_state, names_scope.GetErrors(), ternary_operator.src_loc );

	if( !function_context.is_functionless_context )
	{
		U_ASSERT( result_block != nullptr );
		result_block->insertInto( function_context.function );
		function_context.llvm_ir_builder.SetInsertPoint( result_block );

		if( result->value_type != ValueType::Value )
		{
			llvm::PHINode* const phi= function_context.llvm_ir_builder.CreatePHI( result->type.GetLLVMType()->getPointerTo(), 2u );
			U_ASSERT( branches_end_basic_blocks[0] != nullptr );
			U_ASSERT( branches_end_basic_blocks[1] != nullptr );
			phi->addIncoming( branches_reference_values[0], branches_end_basic_blocks[0] );
			phi->addIncoming( branches_reference_values[1], branches_end_basic_blocks[1] );
			result->llvm_value= phi;
		}
	}

	if( condition->constexpr_value != nullptr )
		result->constexpr_value= condition->constexpr_value->isAllOnesValue() ? branches_constexpr_values[0] : branches_constexpr_values[1];

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator )
{
	const VariablePtr v= BuildExpressionCodeEnsureVariable( reference_to_raw_pointer_operator.expression, names_scope, function_context );
	if( v->value_type == ValueType::Value )
	{
		REPORT_ERROR( ValueIsNotReference, names_scope.GetErrors(), reference_to_raw_pointer_operator.src_loc );
		return ErrorValue();
	}
	if( v->value_type == ValueType::ReferenceImut )
	{
		// Disable immutable reference to pointer conversion, because pointer dereference produces mutable value.
		REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), reference_to_raw_pointer_operator.src_loc );
		return ErrorValue();
	}

	U_ASSERT( v->location == Variable::Location::Pointer );

	// Reference to pointer conversion can break functional purity, so, disable such conversions in constexpr functions.
	function_context.has_non_constexpr_operations_inside= true;

	RawPointerType raw_pointer_type;
	raw_pointer_type.element_type= v->type;
	raw_pointer_type.llvm_type= llvm::PointerType::get( v->type.GetLLVMType(), 0u );

	const VariablePtr result=
		Variable::Create(
			std::move(raw_pointer_type),
			ValueType::Value,
			Variable::Location::LLVMRegister,
			"ptr",
			v->llvm_value );

	function_context.variables_state.AddNode( result );
	RegisterTemporaryVariable( function_context, result );

	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( RawPointerToReferenceConversionOutsideUnsafeBlock, names_scope.GetErrors(), raw_pointer_to_reference_operator.src_loc );

	const VariablePtr v= BuildExpressionCodeEnsureVariable( raw_pointer_to_reference_operator.expression, names_scope, function_context );
	const RawPointerType* const raw_pointer_type= v->type.GetRawPointerType();

	if( raw_pointer_type == nullptr )
	{
		REPORT_ERROR( ValueIsNotPointer, names_scope.GetErrors(), raw_pointer_to_reference_operator.src_loc, v->type );
		return ErrorValue();
	}

	const auto llvm_value= CreateMoveToLLVMRegisterInstruction( *v, function_context );

	// If a pointer dereference result is an instruction produced in current basic block,
	// mark it as non-null, because references are never null.
	// Current basic block check if needed in cases,
	// where this pointer may be null and dereference happens only in a non-null branch.
	// Generally more deep control flow analysis is required, but same basic block check is fine enough.
	// Non-null marking is needed in order to give a hint to the optimizer that possible further null checks are not needed.
	if( llvm_value != nullptr )
	{
		// nonnull metadata is allowed only for "load" instructions.
		if( const auto load_instr= llvm::dyn_cast<llvm::LoadInst>( llvm_value ) )
		{
			if( load_instr->getParent() == function_context.llvm_ir_builder.GetInsertBlock() )
			{
				if( !load_instr->hasMetadata( llvm::LLVMContext::MD_nonnull ) )
					load_instr->setMetadata( llvm::LLVMContext::MD_nonnull, llvm::MDNode::get( llvm_context_, {} ) );
			}
		}
		// For "call" instructions use return value attribute "nonnull".
		if( const auto call_instr= llvm::dyn_cast<llvm::CallInst>( llvm_value ) )
		{
			if( call_instr->getParent() == function_context.llvm_ir_builder.GetInsertBlock() )
				call_instr->addRetAttr( llvm::Attribute::NonNull );
		}
	}

	// Create mutable reference node without any links. TODO - check if this is correct.
	const VariablePtr result=
		Variable::Create(
			raw_pointer_type->element_type,
			ValueType::ReferenceMut,
			Variable::Location::Pointer,
			"$>(" + v->name + ")",
			llvm_value );

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );

	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::NumericConstant& numeric_constant )
{
	U_FundamentalType type= U_FundamentalType::InvalidType;

	const NumberLexemData& num= numeric_constant.num;
	const std::string type_suffix= num.type_suffix.data();

	if( type_suffix.empty() )
		type= num.has_fractional_point ? U_FundamentalType::f64_ : U_FundamentalType::i32_;
	else if( type_suffix == "u" )
		type= U_FundamentalType::u32_;
	// Suffix for size_type
	else if( type_suffix == "s" )
		type= U_FundamentalType::size_type_;
	// Simple "f" suffix for 32bit floats.
	else if( type_suffix == "f" )
		type= U_FundamentalType::f32_;
	// Short suffixes for chars
	else if( type_suffix ==  "c8" )
		type= U_FundamentalType::char8_ ;
	else if( type_suffix == "c16" )
		type= U_FundamentalType::char16_;
	else if( type_suffix == "c32" )
		type= U_FundamentalType::char32_;
	else
		type=GetFundamentalTypeByName( type_suffix );

	if( type == U_FundamentalType::InvalidType )
	{
		REPORT_ERROR( UnknownNumericConstantType, names_scope.GetErrors(), numeric_constant.src_loc, num.type_suffix.data() );
		return ErrorValue();
	}
	llvm::Type* const llvm_type= GetFundamentalLLVMType( type );

	const VariableMutPtr result=
		Variable::Create(
			FundamentalType( type, llvm_type ),
			ValueType::Value,
			Variable::Location::LLVMRegister,
			"numeric constant " + std::to_string(num.value_double) );

	if( IsInteger( type ) || IsChar( type ) )
		result->constexpr_value=
			llvm::Constant::getIntegerValue( llvm_type, llvm::APInt( llvm_type->getIntegerBitWidth(), num.value_int ) );
	else if( IsFloatingPoint( type ) )
		result->constexpr_value=
			llvm::ConstantFP::get( llvm_type, num.value_double );
	else
		U_ASSERT(false);

	result->llvm_value= result->constexpr_value;

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::BooleanConstant& boolean_constant )
{
	U_UNUSED(names_scope);
	
	const VariableMutPtr result=
		Variable::Create(
			bool_type_,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			std::string( Keyword( boolean_constant.value ? Keywords::true_ : Keywords::false_ ) ) );

	result->llvm_value= result->constexpr_value= llvm::ConstantInt::getBool( llvm_context_, boolean_constant.value );

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::StringLiteral& string_literal )
{
	U_UNUSED( function_context );

	U_FundamentalType char_type= U_FundamentalType::InvalidType;
	uint64_t array_size= ~0u; // ~0 - means non-array
	llvm::Constant* initializer= nullptr;

	const std::string& type_suffix= string_literal.type_suffix;

	if( type_suffix.empty() || type_suffix == "u8" )
	{
		char_type= U_FundamentalType::char8_;
		array_size= string_literal.value.size();
		initializer= llvm::ConstantDataArray::getString( llvm_context_, string_literal.value, false /* not null terminated */ );
	}
	else if( type_suffix == "u16" )
	{
		llvm::SmallVector<llvm::UTF16, 32> str;
		llvm::convertUTF8ToUTF16String( string_literal.value, str );

		char_type= U_FundamentalType::char16_;
		array_size= str.size();
		initializer= llvm::ConstantDataArray::get( llvm_context_, str );
	}
	else if( type_suffix == "u32" )
	{
		llvm::SmallVector<uint32_t, 32> str;
		for( auto it = string_literal.value.data(), it_end= it + string_literal.value.size(); it < it_end; )
			str.push_back( ReadNextUTF8Char( it, it_end ) );

		char_type= U_FundamentalType::char32_;
		array_size= str.size();
		initializer= llvm::ConstantDataArray::get( llvm_context_, str );
	}
	// If string literal has char suffix, process it as single char literal.
	else if( type_suffix == "c8" || type_suffix == GetFundamentalTypeName( U_FundamentalType::char8_  ) )
	{
		if( string_literal.value.size() == 1u )
		{
			char_type= U_FundamentalType::char8_ ;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char8_ , uint64_t(string_literal.value[0]), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names_scope.GetErrors(), string_literal.src_loc, string_literal.value );
	}
	else if( type_suffix == "c16" || type_suffix == GetFundamentalTypeName( U_FundamentalType::char16_ ) )
	{
		const char* it_start= string_literal.value.data();
		const char* const it_end= it_start + string_literal.value.size();
		const sprache_char c= ReadNextUTF8Char( it_start, it_end );
		if( it_start == it_end && c <= 65535u )
		{
			char_type= U_FundamentalType::char16_;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char16_, uint64_t(c), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names_scope.GetErrors(), string_literal.src_loc, string_literal.value );
	}
	else if( type_suffix == "c32" || type_suffix== GetFundamentalTypeName( U_FundamentalType::char32_ ) )
	{
		const char* it_start= string_literal.value.data();
		const char* const it_end= it_start + string_literal.value.size() ;
		const sprache_char c= ReadNextUTF8Char( it_start, it_end );
		if( it_start == it_end )
		{
			char_type= U_FundamentalType::char32_;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char32_, uint64_t(c), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names_scope.GetErrors(), string_literal.src_loc, string_literal.value );
	}
	else
		REPORT_ERROR( UnknownStringLiteralSuffix, names_scope.GetErrors(), string_literal.src_loc, type_suffix );

	if( initializer == nullptr )
		return ErrorValue();

	VariableMutPtr result;
	if( array_size == ~0u )
	{
		result= Variable::Create(
			FundamentalType( char_type, GetFundamentalLLVMType( char_type ) ),
			ValueType::Value,
			Variable::Location::LLVMRegister,
			"",
			initializer,
			initializer );
	}
	else
	{
		ArrayType array_type;
		array_type.element_type= FundamentalType( char_type, GetFundamentalLLVMType( char_type ) );
		array_type.element_count= array_size;
		array_type.llvm_type= llvm::ArrayType::get( GetFundamentalLLVMType( char_type ), array_size );

		result= Variable::Create(
			std::move(array_type),
			ValueType::ReferenceImut,
			Variable::Location::Pointer,
			"",
			nullptr,
			initializer );

		// Use md5 for string literal names.
		llvm::MD5 md5;
		if( const auto constant_data_array = llvm::dyn_cast<llvm::ConstantDataArray>(initializer) )
			md5.update( constant_data_array->getRawDataValues() );
		else if( llvm::dyn_cast<llvm::ConstantAggregateZero>(initializer) != nullptr )
			md5.update( std::string( size_t(array_size * GetFundamentalTypeSize(char_type) ), '\0' ) );
		md5.update( llvm::ArrayRef<uint8_t>( reinterpret_cast<const uint8_t*>(&char_type), sizeof(U_FundamentalType) ) ); // Add type to hash for distinction of zero-sized strings with different types.
		llvm::MD5::MD5Result md5_result;
		md5.final(md5_result);
		result->name= ( "_string_literal_" + md5_result.digest() ).str();

		result->llvm_value= CreateGlobalConstantVariable( result->type, result->name, result->constexpr_value );
	}

	function_context.variables_state.AddNode( result );
	RegisterTemporaryVariable( function_context, result );

	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::MoveOperator& move_operator	)
{
	VariablePtr resolved_variable;
	std::string_view kind_name;
	if( move_operator.var_name == Keywords::this_ )
	{
		if( function_context.this_ == nullptr || function_context.whole_this_is_unavailable )
		{
			REPORT_ERROR( ThisUnavailable, names_scope.GetErrors(), move_operator.src_loc );
			return ErrorValue();
		}
		resolved_variable= function_context.this_;
		kind_name= Keyword( Keywords::this_ );
	}
	else
	{
		NamesScopeValue* const resolved_value_ptr= LookupName( names_scope, move_operator.var_name, move_operator.src_loc ).value;
		if( resolved_value_ptr == nullptr )
			return ErrorValue();

		resolved_value_ptr->referenced= true;
		CollectDefinition( *resolved_value_ptr, move_operator.src_loc );

		const Value& resolved_value= resolved_value_ptr->value;
		resolved_variable= resolved_value.GetVariable();
		kind_name= resolved_value.GetKindName();

		if( resolved_variable != nullptr && function_context.lambda_preprocessing_context != nullptr )
		{
			LambdaPreprocessingCheckVariableUsage( names_scope, function_context, resolved_variable, move_operator.var_name, move_operator.src_loc );
			if( function_context.lambda_preprocessing_context->external_variables.count( resolved_variable ) > 0 )
				return LambdaPreprocessingHandleCapturedVariableMove( names_scope, function_context, resolved_variable, move_operator.var_name, move_operator.src_loc );
		}
	}

	// "resolved_variable" should be mutable reference node pointing to single variable node.

	if( resolved_variable == nullptr || IsGlobalVariable( resolved_variable ) )
	{
		REPORT_ERROR( ExpectedVariable, names_scope.GetErrors(), move_operator.src_loc, kind_name );
		return ErrorValue();
	}
	if( resolved_variable->value_type != ValueType::ReferenceMut )
	{
		REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), move_operator.src_loc );
		return ErrorValue();
	}

	if( function_context.variables_state.HasOutgoingLinks( resolved_variable ) )
	{
		REPORT_ERROR( MovedVariableHasReferences, names_scope.GetErrors(), move_operator.src_loc, resolved_variable->name );
		return ErrorValue();
	}

	if( function_context.variables_state.NodeMoved( resolved_variable ) )
	{
		REPORT_ERROR( AccessingMovedVariable, names_scope.GetErrors(), move_operator.src_loc, resolved_variable->name );
		return ErrorValue();
	}

	const auto input_nodes= function_context.variables_state.GetNodeInputLinks( resolved_variable );
	if( input_nodes.size() != 1u )
	{
		REPORT_ERROR( ExpectedVariable, names_scope.GetErrors(), move_operator.src_loc, kind_name );
		return ErrorValue();
	}

	const VariablePtr variable_for_move= *input_nodes.begin();
	if( variable_for_move->value_type != ValueType::Value )
	{
		// This is not a variable, but some reference.
		REPORT_ERROR( ExpectedVariable, names_scope.GetErrors(), move_operator.src_loc, kind_name );
		return ErrorValue();
	}

	bool found_in_variables= false;
	for( const auto& stack_frame : function_context.stack_variables_stack )
	for( const VariablePtr& arg : stack_frame->variables_ )
	{
		if( arg == variable_for_move )
		{
			found_in_variables= true;
			goto end_variable_search;
		}
	}
	end_variable_search:
	if( !found_in_variables )
	{
		REPORT_ERROR( ExpectedVariable, names_scope.GetErrors(), move_operator.src_loc, kind_name );
		return ErrorValue();
	}

	U_ASSERT( !function_context.variables_state.NodeMoved( variable_for_move ) );

	const VariablePtr result=
		Variable::Create(
			variable_for_move->type,
			ValueType::Value,
			variable_for_move->location,
			"_moved_" + variable_for_move->name,
			variable_for_move->llvm_value );
	function_context.variables_state.AddNode( result );

	function_context.variables_state.TryAddInnerLinks( resolved_variable, result, names_scope.GetErrors(), move_operator.src_loc );

	// Move both reference node and variable node.
	function_context.variables_state.MoveNode( resolved_variable );
	function_context.variables_state.MoveNode( variable_for_move );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::MoveOperatorCompletion& move_operator_completion )
{
	(void)function_context;
	NameLookupCompleteImpl( names_scope, move_operator_completion.var_name );
	return ErrorValue();
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::TakeOperator& take_operator	)
{
	const VariablePtr expression_result= BuildExpressionCodeEnsureVariable( take_operator.expression, names_scope, function_context );
	if( expression_result->value_type == ValueType::Value ) // If it is value - just pass it.
		return expression_result;

	if( expression_result->value_type != ValueType::ReferenceMut )
	{
		REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), take_operator.src_loc );
		return ErrorValue();
	}
	if( function_context.variables_state.HasOutgoingLinks( expression_result ) )
	{
		REPORT_ERROR( MovedVariableHasReferences, names_scope.GetErrors(), take_operator.src_loc, expression_result->name );
		return ErrorValue();
	}
	if( expression_result->type.IsAbstract() )
	{
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names_scope.GetErrors(), take_operator.src_loc, expression_result->type );
		return ErrorValue();
	}
	if( const auto class_type= expression_result->type.GetClassType() )
	{
		// Do not allow taking values of polymorph non-final classes to avoid calling default constructor of base class in place of derived class.
		// It may break derived class invariants and will overwrite virtual table pointer.
		if( class_type->kind == Class::Kind::Interface || class_type->kind == Class::Kind::Abstract || class_type->kind == Class::Kind::PolymorphNonFinal )
		{
			REPORT_ERROR( TakeForNonFinalPolymorphClass, names_scope.GetErrors(), take_operator.src_loc, expression_result->type );
			return ErrorValue();
		}
	}

	// Allocate variable for result.
	const VariableMutPtr result=
		Variable::Create(
			expression_result->type,
			ValueType::Value,
			Variable::Location::Pointer,
			"_moved_" + expression_result->name );

	// Copy content to new variable.
	function_context.variables_state.AddNode( result );

	function_context.variables_state.TryAddInnerLinks( expression_result, result, names_scope.GetErrors(), take_operator.src_loc );

	if( !function_context.is_functionless_context )
	{
		result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( result->type.GetLLVMType() );
		result->llvm_value->setName( result->name );

		CreateLifetimeStart( function_context, result->llvm_value );

		// Copy content to new variable.
		CopyBytes( result->llvm_value, expression_result->llvm_value, result->type, function_context );
	}

	// Construct empty value in old place.
	ApplyEmptyInitializer( expression_result->name, take_operator.src_loc, expression_result, names_scope, function_context );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::Lambda& lambda )
{
	return BuildLambda( names_scope, function_context, lambda );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::CastMut& cast_mut )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( MutableReferenceCastOutsideUnsafeBlock, names_scope.GetErrors(), cast_mut.src_loc );

	const VariablePtr var= BuildExpressionCodeEnsureVariable( cast_mut.expression, names_scope, function_context );

	const VariableMutPtr result=
		Variable::Create(
			var->type,
			ValueType::ReferenceMut,
			Variable::Location::Pointer,
			"cast_mut( " + var->name + " )",
			var->llvm_value,
			nullptr ); // Reset constexprness for mutable reference.

	if( var->location == Variable::Location::LLVMRegister )
	{
		if( !function_context.is_functionless_context )
		{
			result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( var->type.GetLLVMType() );
			CreateTypedStore( function_context, var->type, var->llvm_value, result->llvm_value );
		}
	}

	// TODO - check if it is correct to create mutable links to possible immutable links.
	function_context.variables_state.AddNode( result );
	function_context.variables_state.TryAddLink( var, result, names_scope.GetErrors(), cast_mut.src_loc );
	function_context.variables_state.TryAddInnerLinks( var, result, names_scope.GetErrors(), cast_mut.src_loc );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::CastImut& cast_imut	)
{
	const VariablePtr var= BuildExpressionCodeEnsureVariable( cast_imut.expression, names_scope, function_context );

	const VariableMutPtr result=
		Variable::Create(
			var->type,
			ValueType::ReferenceImut,
			Variable::Location::Pointer,
			"cast_imut(" + var->name + ")",
			nullptr,
			var->constexpr_value );

	if( var->location == Variable::Location::LLVMRegister )
	{
		if( !function_context.is_functionless_context )
		{
			result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( var->type.GetLLVMType() );
			CreateTypedStore( function_context, var->type, var->llvm_value, result->llvm_value );
		}
	}
	else
		result->llvm_value= var->llvm_value;

	function_context.variables_state.AddNode( result );
	function_context.variables_state.TryAddLink( var, result, names_scope.GetErrors(), cast_imut.src_loc );
	function_context.variables_state.TryAddInnerLinks( var, result, names_scope.GetErrors(), cast_imut.src_loc );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::CastRef& cast_ref )
{
	return DoReferenceCast( cast_ref.src_loc, cast_ref.type, cast_ref.expression, false, names_scope, function_context );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::CastRefUnsafe& cast_ref_unsafe )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( UnsafeReferenceCastOutsideUnsafeBlock, names_scope.GetErrors(), cast_ref_unsafe.src_loc );

	return DoReferenceCast( cast_ref_unsafe.src_loc, cast_ref_unsafe.type, cast_ref_unsafe.expression, true, names_scope, function_context );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::Embed& embed )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( embed.expression, names_scope, function_context );
	if( variable->type == invalid_type_ )
		return ErrorValue();

	const auto array_type= variable->type.GetArrayType();
	if( array_type == nullptr ||
		array_type->element_type != FundamentalType( U_FundamentalType::char8_, fundamental_llvm_types_.char8_ ) )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), embed.src_loc, "char8 array", variable->type.ToString() );
		return ErrorValue();
	}

	if( variable->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), embed.src_loc );
		return ErrorValue();
	}

	std::string file_path;
	if( const auto constant_data= llvm::dyn_cast<llvm::ConstantDataArray>( variable->constexpr_value ) )
		file_path= constant_data->getRawDataValues().str();
	else if( llvm::isa<llvm::ConstantAggregateZero>( variable->constexpr_value ) )
		file_path= "";
	else
	{
		REPORT_ERROR( NotImplemented, names_scope.GetErrors(), embed.src_loc, "non-trivial embed file name constants" );
		return ErrorValue();
	}

	const auto file_index= embed.src_loc.GetFileIndex();
	U_ASSERT( file_index < source_graph_->nodes_storage.size() );
	const IVfs::Path& parent_file_path= source_graph_->nodes_storage[file_index].file_path;

	// Load file contents. Use caching in order to avoid loading same file contents more than once.
	IVfs::Path full_file_path= vfs_->GetFullFilePath( file_path, parent_file_path );

	auto cache_it= embed_files_cache_.find( full_file_path );
	if( cache_it == embed_files_cache_.end() )
	{
		if( !vfs_->IsImportingFileAllowed( full_file_path ) )
			REPORT_ERROR( EmbeddingThisFileIsNotAllowed, names_scope.GetErrors(), embed.src_loc, full_file_path );

		std::optional<IVfs::FileContent> loaded_file= vfs_->LoadFileContent( full_file_path );
		cache_it= embed_files_cache_.emplace( std::move(full_file_path), std::move(loaded_file) ).first;
	}

	const std::optional<IVfs::FileContent> loaded_file= cache_it->second;
	if( loaded_file == std::nullopt )
	{
		REPORT_ERROR( EmbedFileNotFound, names_scope.GetErrors(), embed.src_loc, file_path );
		return ErrorValue();
	}

	U_FundamentalType element_type= U_FundamentalType::byte8_;
	if( embed.element_type != std::nullopt )
	{
		const Value v= ResolveValue( names_scope, function_context, *embed.element_type );
		const Type* const t= v.GetTypeName();
		if( t == nullptr )
		{
			REPORT_ERROR( NameIsNotTypeName, names_scope.GetErrors(), embed.src_loc, v.GetKindName() );
			return ErrorValue();
		}

		const auto fundamental_type= t->GetFundamentalType();
		if( fundamental_type == nullptr || GetFundamentalTypeSize( fundamental_type->fundamental_type ) != 1 )
		{
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), Synt::GetSrcLoc( *embed.element_type ), "any fundamental 8-bit type", t->ToString() );
			return ErrorValue();
		}

		element_type= fundamental_type->fundamental_type;

		// Do not care if in case of char8 element embedded file contents isn't valid UTF-8.
	}

	llvm::Type* const element_llvm_type= GetFundamentalLLVMType( element_type );

	ArrayType result_array_type;
	result_array_type.element_type= FundamentalType( element_type, element_llvm_type );
	result_array_type.element_count= loaded_file->size();
	result_array_type.llvm_type= llvm::ArrayType::get( element_llvm_type, result_array_type.element_count );

	const auto result= Variable::Create(
		std::move(result_array_type),
		ValueType::ReferenceImut,
		Variable::Location::Pointer,
		// Use contents hash-based names for embed arrays.
		"_embed_array_" + CalculateSourceFileContentsHash( *loaded_file ),
		nullptr,
		llvm::ConstantDataArray::getString( llvm_context_, *loaded_file, false /* not null terminated */ ) );

	result->llvm_value= CreateGlobalConstantVariable( result->type, result->name, result->constexpr_value );

	function_context.variables_state.AddNode( result );
	RegisterTemporaryVariable( function_context, result );

	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::ExternalFunctionAccess& external_function_access )
{
	// TODO - prevent usage in global context.
	// TODO - prevent usage in safe code.

	const Type type= PrepareType( external_function_access.type, names_scope, function_context );
	const FunctionPointerType* const function_pointer_type= type.GetFunctionPointerType();

	if( function_pointer_type == nullptr )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), external_function_access.src_loc, "any function type", type );
		return ErrorValue();
	}

	llvm::FunctionType* const function_llvm_type= GetLLVMFunctionType( function_pointer_type->function_type );

	llvm::Function* function= nullptr;
	if( const auto prev_function= module_->getFunction( external_function_access.name ) )
	{
		if( prev_function->getFunctionType() != function_llvm_type )
		{
			// TODO - use other error code.
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), external_function_access.src_loc, "old function type", type );
			return ErrorValue();
		}
		function= prev_function;
	}
	else
		function=
			llvm::Function::Create(
				function_llvm_type,
				llvm::GlobalValue::ExternalLinkage,
				external_function_access.name,
				*module_ );

	// Return value of function pointer type.
	const auto result= Variable::Create(
		type,
		ValueType::Value,
		Variable::Location::LLVMRegister,
		"external function " + external_function_access.name,
		function,
		nullptr /* do not produce constexpr value */ );

	function_context.variables_state.AddNode( result );
	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::TypeInfo& typeinfo )
{
	const Type type= PrepareType( typeinfo.type, names_scope, function_context );
	if( type == invalid_type_ )
		return ErrorValue();

	if( !EnsureTypeComplete( type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), typeinfo.src_loc, type );
		return ErrorValue();
	}

	NamesScope& root_namespace= *names_scope.GetRoot();
	BuildTypeInfo( type, *names_scope.GetRoot() );

	const VariableMutPtr& var_ptr= typeinfo_cache_[type].variable;
	BuildFullTypeinfo( type, var_ptr, root_namespace );

	function_context.variables_state.AddNodeIfNotExists( var_ptr );

	return var_ptr;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::SameType& same_type )
{
	const Type type_l= PrepareType( same_type.l, names_scope, function_context );
	const Type type_r= PrepareType( same_type.r, names_scope, function_context );
	const bool same= type_l == type_r;

	llvm::Constant* const constant= llvm::ConstantInt::getBool( llvm_context_, same );

	const VariablePtr result=
		Variable::Create(
			bool_type_,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			std::string( Keyword( Keywords::same_type_ ) ),
			constant,
			constant );

	function_context.variables_state.AddNode( result );
	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::NonSyncExpression& non_sync_expression )
{
	const Type type= PrepareType( non_sync_expression.type, names_scope, function_context );
	const bool is_non_sync= GetTypeNonSync( type, names_scope, non_sync_expression.src_loc );

	const VariableMutPtr result=
		Variable::Create(
			bool_type_,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			std::string( Keyword( Keywords::non_sync_ ) ) );

	result->llvm_value= result->constexpr_value= llvm::ConstantInt::getBool( llvm_context_, is_non_sync );

	function_context.variables_state.AddNode( result);

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::SafeExpression& safe_expression )
{
	const bool prev_unsafe= function_context.is_in_unsafe_block;
	function_context.is_in_unsafe_block= false;
	Value result= BuildExpressionCode( safe_expression.expression, names_scope, function_context );
	function_context.is_in_unsafe_block= prev_unsafe;
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::UnsafeExpression& unsafe_expression )
{
	if( function_context.function == global_function_context_->function )
		REPORT_ERROR( UnsafeExpressionInGlobalContext, names_scope.GetErrors(), unsafe_expression.src_loc );

	// "unsafe" expression usage should prevent function to be "constexpr".
	function_context.has_non_constexpr_operations_inside= true;

	const bool prev_unsafe= function_context.is_in_unsafe_block;
	function_context.is_in_unsafe_block= true;
	Value result= BuildExpressionCode( unsafe_expression.expression, names_scope, function_context );
	function_context.is_in_unsafe_block= prev_unsafe;

	// Avoid passing constexpr values trough unsafe expression.
	// TODO - do we really needs this?
	if( const VariablePtr variable_ptr= result.GetVariable() )
	{
		if( variable_ptr->constexpr_value != nullptr )
		{
			const VariablePtr variable_copy=
				Variable::Create(
				variable_ptr->type,
				variable_ptr->value_type,
				variable_ptr->location,
				"unsafe(" + variable_ptr->name + ")",
				variable_ptr->llvm_value,
				nullptr );

			function_context.variables_state.AddNode( variable_copy );

			if( variable_ptr->value_type != ValueType::Value )
				function_context.variables_state.TryAddLink( variable_ptr, variable_copy, names_scope.GetErrors(), unsafe_expression.src_loc );
			function_context.variables_state.TryAddInnerLinks( variable_ptr, variable_copy, names_scope.GetErrors(), unsafe_expression.src_loc );

			if( variable_ptr->value_type == ValueType::Value )
				function_context.variables_state.MoveNode( variable_ptr );

			RegisterTemporaryVariable( function_context, variable_copy );

			return variable_copy;
		}
	}

	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::RootNamespaceNameLookup& root_namespace_lookup )
{
	return ResolveValueImpl( names_scope, function_context, root_namespace_lookup );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::RootNamespaceNameLookupCompletion& root_namespace_lookup_completion )
{
	return ResolveValueImpl( names_scope, function_context, root_namespace_lookup_completion );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::NameLookup& name_lookup )
{
	return ResolveValueImpl( names_scope, function_context, name_lookup );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::NameLookupCompletion& name_lookup_completion )
{
	return ResolveValueImpl( names_scope, function_context, name_lookup_completion );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::TypeofTypeName& typeof_type_name )
{
	return ResolveValueImpl( names_scope, function_context, typeof_type_name );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::NamesScopeNameFetch& names_scope_fetch )
{
	return ResolveValueImpl( names_scope, function_context, names_scope_fetch );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::NamesScopeNameFetchCompletion& names_scope_fetch_completion )
{
	return ResolveValueImpl( names_scope, function_context, names_scope_fetch_completion );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::TemplateParameterization& template_parameterization )
{
	return ResolveValueImpl( names_scope, function_context, template_parameterization );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::Mixin& mixin )
{
	if( const auto expression= ExpandExpressionMixin( names_scope, function_context, mixin ) )
		return BuildExpressionCode( *expression, names_scope, function_context );
	return ErrorValue(); // Error should be generated prior to this.
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::ArrayTypeName& type_name )
{
	return PrepareTypeImpl( names_scope, function_context, type_name );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::FunctionType& type_name )
{
	return PrepareTypeImpl( names_scope, function_context, type_name );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::TupleType& type_name )
{
	return PrepareTypeImpl( names_scope, function_context, type_name );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::RawPointerType& type_name )
{
	return PrepareTypeImpl( names_scope, function_context, type_name );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::CoroutineType& type_name )
{
	return PrepareTypeImpl( names_scope, function_context, type_name );
}

VariablePtr CodeBuilder::AccessClassBase( const VariablePtr& variable, FunctionContext& function_context )
{
	const Class* const variabe_type_class= variable->type.GetClassType();
	U_ASSERT( variabe_type_class != nullptr );

	const uint32_t c_base_field_index= 0;

	variable->children.resize( variabe_type_class->llvm_type->getNumElements(), nullptr );
	if( const auto prev_node= variable->children[ c_base_field_index ] )
	{
		function_context.variables_state.AddNodeIfNotExists( prev_node );
		return prev_node;
	}

	const VariableMutPtr base=
		Variable::CreateChildNode(
			variable,
			variabe_type_class->base_class,
			variable->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
			Variable::Location::Pointer,
			std::string( Keyword( Keywords::base_ ) ),
			ForceCreateConstantIndexGEP( function_context, variable->type.GetLLVMType(), variable->llvm_value, c_base_field_index ) );

	// Reference nodes of child class are mapped 1 to 1 to nodes of parent class.
	U_ASSERT( base->inner_reference_nodes.size() <= variable->inner_reference_nodes.size() );
	for( size_t i= 0; i < base->inner_reference_nodes.size(); ++i )
		base->inner_reference_nodes[i]= variable->inner_reference_nodes[i];

	variable->children[ c_base_field_index ]= base;

	function_context.variables_state.AddNode( base );

	return base;
}

Value CodeBuilder::AccessClassField(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const VariablePtr& variable,
	const ClassField& field,
	const std::string& field_name,
	const SrcLoc& src_loc )
{
	const Class* const variable_type_class= variable->type.GetClassType();
	U_ASSERT( variable_type_class != nullptr );

	if( field.class_ != variable->type )
	{
		if( variable_type_class->base_class != nullptr )
		{
			// Recursive try to access field in parent class.
			return
				AccessClassField(
					names_scope,
					function_context,
					AccessClassBase( variable, function_context ),
					field,
					field_name,
					src_loc );
		}

		// No base - this is wrong field for this class.
		REPORT_ERROR( AccessOfNonThisClassField, names_scope.GetErrors(), src_loc, field_name );
		return ErrorValue();
	}

	if( field.is_reference )
	{
		const VariableMutPtr result=
			Variable::Create(
				field.type,
				field.is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				 Variable::Location::Pointer,
				variable->name + "." + field_name );

		if( variable->constexpr_value != nullptr )
		{
			if( EnsureTypeComplete( field.type ) )
			{
				// Constexpr references field should be "GlobalVariable" or Constexpr GEP.
				const auto element= variable->constexpr_value->getAggregateElement( field.index );

				result->llvm_value= element;

				if( const auto global_variable= llvm::dyn_cast<llvm::GlobalVariable>(element) )
				{
					if( std::holds_alternative<TypeinfoClassDescription>( field.class_->generated_class_data ) && field_name == "type_id" )
					{
						// HACK!
						// LLVM performs constants folding since poiters are not typed. So, we can't obtain full path to GlobalVariable initializer.
						// This is used for type_id in typeinfo classes.
						result->constexpr_value= global_variable->getInitializer()->getAggregateElement(0u)->getAggregateElement(0u);
					}
					else
						result->constexpr_value= global_variable->getInitializer();
				}
				else if( const auto constant_expression= llvm::dyn_cast<llvm::ConstantExpr>( element ) )
				{
					// TODO - what first operand is constant GEP too?
					llvm::Constant* value= llvm::dyn_cast<llvm::GlobalVariable>(constant_expression->getOperand(0u))->getInitializer();

					// Skip first zero index.
					for( llvm::User::const_op_iterator op= std::next(std::next(constant_expression->op_begin())), op_end= constant_expression->op_end(); op != op_end; ++op )
						value= value->getAggregateElement( llvm::dyn_cast<llvm::Constant>(op->get()) );
					result->constexpr_value= value;
				}
				else U_ASSERT(false);
			}
			else
				return ErrorValue(); // Actual error will be reported in another place.
		}
		else
		{
			if( const auto load_res=
					CreateTypedReferenceLoad(
					function_context,
					field.type,
					ForceCreateConstantIndexGEP(
						function_context,
						variable->type.GetLLVMType(),
						variable->llvm_value,
						field.index ) ) )
			{
				result->llvm_value= load_res;
			}
		}

		function_context.variables_state.AddNode( result );

		U_ASSERT( field.reference_tag < variable->inner_reference_nodes.size() );
		function_context.variables_state.TryAddLink( variable->inner_reference_nodes[ field.reference_tag ], result, names_scope.GetErrors(), src_loc );

		// Setup inner reference node links for reference fields with references inside.
		// Only reference-fields with single inner reference tag are supported.
		if( field.type.ReferenceTagCount() == 1 )
		{
			for( const VariablePtr& accessible_node :
				function_context.variables_state.GetAllAccessibleNonInnerNodes( variable->inner_reference_nodes[ field.reference_tag ] ) )
			{
				// Usually we should have here only one inner reference node.
				// But it may be more if a member of a composite value is referenced.
				// In such case it's necessary to create links for all inner reference nodes, even if sometimes it can create false-positive reference protection errors.
				for( const VariablePtr& node : accessible_node->inner_reference_nodes )
					function_context.variables_state.TryAddLink( node, result->inner_reference_nodes.front(), names_scope.GetErrors(), src_loc );
			}
		}

		RegisterTemporaryVariable( function_context, result );
		return result;
	}
	else
	{
		variable->children.resize( variable_type_class->llvm_type->getNumElements(), nullptr );
		if( const auto prev_node= variable->children[ field.index ] )
		{
			function_context.variables_state.AddNodeIfNotExists( prev_node );
			return prev_node; // Child node already created.
		}

		// Create variable child node.
		const VariableMutPtr result=
			Variable::CreateChildNode(
				variable,
				field.type,
				( variable->value_type == ValueType::ReferenceMut && field.is_mutable ) ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				variable->name + "." + field_name,
				ForceCreateConstantIndexGEP( function_context, variable->type.GetLLVMType(), variable->llvm_value, field.index ) );

		U_ASSERT( result->inner_reference_nodes.size() == field.inner_reference_tags.size() );
		for( size_t i= 0; i < result->inner_reference_nodes.size(); ++i )
		{
			const auto src_tag_number= field.inner_reference_tags[i];
			U_ASSERT( src_tag_number < variable->inner_reference_nodes.size() );
			result->inner_reference_nodes[i]= variable->inner_reference_nodes[src_tag_number];
		}

		if( variable->constexpr_value != nullptr )
			result->constexpr_value= variable->constexpr_value->getAggregateElement( field.index );

		variable->children[ field.index ]= result;

		function_context.variables_state.AddNode( result );

		return result;
	}
}

std::optional<Value> CodeBuilder::TryCallOverloadedBinaryOperator(
	const OverloadedOperator op,
	const Synt::Expression&  left_expr,
	const Synt::Expression& right_expr,
	const bool evaluate_args_in_reverse_order,
	const SrcLoc& src_loc,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	// Know args types.
	llvm::SmallVector<FunctionType::Param, 2> args;
	{
		const bool prev_is_functionless_context= function_context.is_functionless_context;
		function_context.is_functionless_context= true;
		const auto state= SaveFunctionContextState( function_context );
		{
			const StackVariablesStorage dummy_stack_variables_storage( function_context );
			for( const Synt::Expression* const in_arg : { &left_expr, &right_expr } )
				args.push_back( PreEvaluateArg( *in_arg, names_scope, function_context ) );
		}

		RestoreFunctionContextState( function_context, state );
		function_context.is_functionless_context= prev_is_functionless_context;
	}

	// Apply here move-assignment.
	if( op == OverloadedOperator::Assign &&
		args.front().value_type == ValueType::ReferenceMut &&
		args.back().value_type == ValueType::Value &&
		args.front().type == args.back().type &&
		( args.front().type.GetClassType() != nullptr || args.front().type.GetArrayType() != nullptr || args.front().type.GetTupleType() != nullptr ) )
	{
		if( const auto class_type= args.front().type.GetClassType() )
		{
			// Forbid move-assignment for destination of non-final polymorph class.
			// This is needed to prevent changing class fields (including virtual table pointer) relevant to derived class with class fields relevant to base class.
			// For example
			// cast_ref</Base/>(derived)= Base();
			if( class_type->kind == Class::Kind::Interface || class_type->kind == Class::Kind::Abstract || class_type->kind == Class::Kind::PolymorphNonFinal )
				REPORT_ERROR( MoveAssignForNonFinalPolymorphClass, names_scope.GetErrors(), src_loc, args.front().type );
		}

		// Move here, instead of calling copy-assignment operator. Before moving we must also call destructor for destination.
		const VariablePtr r_var_real= BuildExpressionCode( right_expr, names_scope, function_context ).GetVariable();

		const VariableMutPtr r_var_lock=
			Variable::Create(
				r_var_real->type,
				ValueType::ReferenceMut,
				r_var_real->location,
				r_var_real->name + " lock",
				r_var_real->llvm_value );
		function_context.variables_state.AddNode( r_var_lock );
		function_context.variables_state.TryAddLink( r_var_real, r_var_lock, names_scope.GetErrors(), src_loc );
		function_context.variables_state.TryAddInnerLinks( r_var_real, r_var_lock, names_scope.GetErrors(), src_loc );

		r_var_lock->preserve_temporary= true;
		RegisterTemporaryVariable( function_context, r_var_lock );

		const VariablePtr l_var_real= BuildExpressionCode( left_expr, names_scope, function_context ).GetVariable();

		if( function_context.variables_state.HasOutgoingLinks( l_var_real ) )
			REPORT_ERROR( ReferenceProtectionError, names_scope.GetErrors(), src_loc, l_var_real->name );

		SetupReferencesInCopyOrMove( function_context, l_var_real, r_var_lock, names_scope.GetErrors(), src_loc );

		function_context.variables_state.MoveNode( r_var_lock );
		function_context.variables_state.MoveNode( r_var_real );

		if( !function_context.is_functionless_context )
		{
			if( l_var_real->type.HasDestructor() )
				CallDestructor( l_var_real->llvm_value, l_var_real->type, function_context, names_scope.GetErrors(), src_loc );

			U_ASSERT( r_var_real->location == Variable::Location::Pointer );
			CopyBytes( l_var_real->llvm_value, r_var_real->llvm_value, l_var_real->type, function_context );
			CreateLifetimeEnd( function_context, r_var_real->llvm_value );
		}

		return Variable::Create( void_type_, ValueType::Value, Variable::Location::LLVMRegister );
	}
	else if( op == OverloadedOperator::Add && AreCharArraysWithSameElementType( args.front().type, args.back().type ) )
		return ConcatenateCharArrays( left_expr, right_expr, src_loc, names_scope, function_context );
	else if( args.front().type == args.back().type && ( args.front().type.GetArrayType() != nullptr || args.front().type.GetTupleType() != nullptr ) )
		return CallBinaryOperatorForArrayOrTuple( op, left_expr, right_expr, src_loc, names_scope, function_context );

	if( const auto overloaded_operator= GetOverloadedOperator( args, op, names_scope, src_loc ) )
	{
		if( overloaded_operator->is_deleted )
			REPORT_ERROR( AccessingDeletedMethod, names_scope.GetErrors(), src_loc );
		if( !( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
			function_context.has_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

		if( overloaded_operator->virtual_table_index != ~0u )
		{
			// We can not fetch virtual function here, because "this" may be evaluated as second operand for some binary operators.
			REPORT_ERROR( NotImplemented, names_scope.GetErrors(), src_loc, "calling virtual binary operators" );
		}

		overloaded_operator->referenced= true;

		return
			DoCallFunction(
				EnsureLLVMFunctionCreated( *overloaded_operator ),
				overloaded_operator->type,
				src_loc,
				nullptr,
				{ &left_expr, &right_expr },
				evaluate_args_in_reverse_order,
				names_scope,
				function_context,
				overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete );
	}

	return std::nullopt;
}

Value CodeBuilder::CallBinaryOperatorForArrayOrTuple(
	OverloadedOperator op,
	const Synt::Expression&  left_expr,
	const Synt::Expression& right_expr,
	const SrcLoc& src_loc,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	if( op == OverloadedOperator::Assign )
	{
		const VariablePtr r_var= BuildExpressionCodeEnsureVariable( right_expr, names_scope, function_context );
		if( r_var->type == invalid_type_ )
			return ErrorValue();

		const VariableMutPtr r_var_lock=
			Variable::Create(
				r_var->type,
				ValueType::ReferenceImut,
				r_var->location,
				r_var->name + " lock",
				r_var->llvm_value );
		function_context.variables_state.AddNode( r_var_lock );
		function_context.variables_state.TryAddLink( r_var, r_var_lock, names_scope.GetErrors(), src_loc );
		function_context.variables_state.TryAddInnerLinks( r_var, r_var_lock, names_scope.GetErrors(), src_loc );

		r_var_lock->preserve_temporary= true;
		RegisterTemporaryVariable( function_context, r_var_lock );

		const VariablePtr l_var= BuildExpressionCodeEnsureVariable( left_expr, names_scope, function_context );

		if( function_context.variables_state.HasOutgoingLinks( l_var ) )
			REPORT_ERROR( ReferenceProtectionError, names_scope.GetErrors(), src_loc, l_var->name );

		function_context.variables_state.MoveNode( r_var_lock );

		if( l_var->type == invalid_type_ )
			return ErrorValue();
		U_ASSERT( l_var->type == r_var->type ); // Checked before.

		if( !l_var->type.IsCopyAssignable() )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_var->type );
			return ErrorValue();
		}

		if( l_var->value_type != ValueType::ReferenceMut )
		{
			REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), src_loc );
			return ErrorValue();
		}

		SetupReferencesInCopyOrMove( function_context, l_var, r_var, names_scope.GetErrors(), src_loc );

		BuildCopyAssignmentOperatorPart(
			l_var->llvm_value, r_var->llvm_value,
			l_var->type,
			function_context );

		return Variable::Create( void_type_, ValueType::Value, Variable::Location::LLVMRegister );
	}
	else if( op == OverloadedOperator::CompareEqual )
	{
		const VariablePtr l_var= BuildExpressionCodeEnsureVariable(  left_expr, names_scope, function_context );
		if( l_var->type == invalid_type_ )
			return ErrorValue();

		const VariableMutPtr l_var_lock=
			Variable::Create(
				l_var->type,
				ValueType::ReferenceImut,
				l_var->location,
				l_var->name + " lock",
				l_var->llvm_value );
		function_context.variables_state.AddNode( l_var_lock );
		function_context.variables_state.TryAddLink( l_var, l_var_lock, names_scope.GetErrors(), src_loc );
		function_context.variables_state.TryAddInnerLinks( l_var, l_var_lock, names_scope.GetErrors(), src_loc );

		l_var_lock->preserve_temporary= true;
		RegisterTemporaryVariable( function_context, l_var_lock );

		const VariablePtr r_var= BuildExpressionCodeEnsureVariable( right_expr, names_scope, function_context );
		if( function_context.variables_state.HasOutgoingMutableNodes( r_var ) )
			REPORT_ERROR( ReferenceProtectionError, names_scope.GetErrors(), src_loc, r_var->name );

		function_context.variables_state.MoveNode( l_var_lock );

		if( r_var->type == invalid_type_ )
			return ErrorValue();
		U_ASSERT( r_var->type == l_var->type ); // Checked before.

		if( !l_var->type.IsEqualityComparable() )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_var->type );
			return ErrorValue();
		}

		U_ASSERT( l_var->location == Variable::Location::Pointer );
		U_ASSERT( r_var->location == Variable::Location::Pointer );

		const VariableMutPtr result=
			Variable::Create(
				bool_type_,
				ValueType::Value,
				Variable::Location::LLVMRegister,
				std::string( OverloadedOperatorToString(op) ) );

		if( l_var->constexpr_value != nullptr && r_var->constexpr_value != nullptr )
			result->llvm_value= result->constexpr_value= ConstexprCompareEqual( l_var->constexpr_value, r_var->constexpr_value, l_var->type, names_scope, src_loc );
		else
		{
			const auto false_basic_block= llvm::BasicBlock::Create( llvm_context_ );
			const auto end_basic_block= llvm::BasicBlock::Create( llvm_context_ );

			BuildEqualityCompareOperatorPart(
				l_var->llvm_value,
				r_var->llvm_value,
				l_var->type,
				false_basic_block,
				function_context );

			if( false_basic_block->hasNPredecessorsOrMore(1) )
			{
				// True branch.
				const auto true_basic_block= function_context.llvm_ir_builder.GetInsertBlock();
				function_context.llvm_ir_builder.CreateBr( end_basic_block );

				// False branch.
				false_basic_block->insertInto( function_context.function );
				function_context.llvm_ir_builder.SetInsertPoint( false_basic_block );
				function_context.llvm_ir_builder.CreateBr( end_basic_block );

				// End basic block.
				end_basic_block->insertInto( function_context.function );
				function_context.llvm_ir_builder.SetInsertPoint( end_basic_block );

				const auto phi= function_context.llvm_ir_builder.CreatePHI( fundamental_llvm_types_.bool_, 2 );
				phi->addIncoming( llvm::ConstantInt::getFalse( llvm_context_ ), false_basic_block );
				phi->addIncoming( llvm::ConstantInt::getTrue ( llvm_context_ ), true_basic_block  );

				result->llvm_value= phi;
			}
			else
			{
				// Empty tuple or array.
				delete false_basic_block;
				delete end_basic_block;
				result->llvm_value= llvm::ConstantInt::getTrue( llvm_context_ );
			}
		}

		function_context.variables_state.AddNode( result );

		RegisterTemporaryVariable( function_context, result );
		return result;
	}
	else
	{
		const VariablePtr l_var= BuildExpressionCodeEnsureVariable( left_expr, names_scope, function_context );
		REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_var->type );
		return ErrorValue();
	}
}

Value CodeBuilder::ConcatenateCharArrays(
	const Synt::Expression&  left_expr,
	const Synt::Expression& right_expr,
	const SrcLoc& src_loc,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	const VariablePtr l_var= BuildExpressionCodeEnsureVariable(  left_expr, names_scope, function_context );
	if( l_var->type == invalid_type_ )
		return ErrorValue();

	const VariableMutPtr l_var_lock=
		Variable::Create(
			l_var->type,
			ValueType::ReferenceImut,
			l_var->location,
			l_var->name + " lock",
			l_var->llvm_value );
	function_context.variables_state.AddNode( l_var_lock );
	function_context.variables_state.TryAddLink( l_var, l_var_lock, names_scope.GetErrors(), src_loc );
	function_context.variables_state.TryAddInnerLinks( l_var, l_var_lock, names_scope.GetErrors(), src_loc );

	l_var_lock->preserve_temporary= true;
	RegisterTemporaryVariable( function_context, l_var_lock );

	const VariablePtr r_var= BuildExpressionCodeEnsureVariable( right_expr, names_scope, function_context );
	if( function_context.variables_state.HasOutgoingMutableNodes( r_var ) )
		REPORT_ERROR( ReferenceProtectionError, names_scope.GetErrors(), src_loc, r_var->name );

	function_context.variables_state.MoveNode( l_var_lock );

	if( r_var->type == invalid_type_ )
		return ErrorValue();

	const auto l_array_type= l_var->type.GetArrayType();
	const auto r_array_type= r_var->type.GetArrayType();
	U_ASSERT( l_array_type != nullptr );
	U_ASSERT( r_array_type != nullptr );
	U_ASSERT( l_array_type->element_type == r_array_type->element_type );

	ArrayType result_array_type;
	// TODO - handle overflow?
	result_array_type.element_count= l_array_type->element_count + r_array_type->element_count;
	result_array_type.element_type= l_array_type->element_type;
	result_array_type.llvm_type= llvm::ArrayType::get( result_array_type.element_type.GetLLVMType(), result_array_type.element_count );

	const VariableMutPtr result=
		Variable::Create(
			result_array_type,
			ValueType::Value,
			Variable::Location::Pointer,
			std::string( OverloadedOperatorToString( OverloadedOperator::Add ) ) );

	if( !function_context.is_functionless_context )
	{
		result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( result->type.GetLLVMType() );
		CreateLifetimeStart( function_context, result->llvm_value );

		if( l_var->llvm_value != nullptr && r_var->llvm_value != nullptr )
		{
			const auto alignment= data_layout_.getABITypeAlign( l_array_type->element_type.GetLLVMType() );

			const auto size_l= llvm::Constant::getIntegerValue( fundamental_llvm_types_.u32_, llvm::APInt( 32, data_layout_.getTypeAllocSize( l_array_type->llvm_type ) ) );
			const auto size_r= llvm::Constant::getIntegerValue( fundamental_llvm_types_.u32_, llvm::APInt( 32, data_layout_.getTypeAllocSize( r_array_type->llvm_type ) ) );

			// Copy l_var bytes into destination.
			function_context.llvm_ir_builder.CreateMemCpy(
				result->llvm_value, llvm::MaybeAlign(alignment),
				l_var->llvm_value , llvm::MaybeAlign(alignment),
				size_l );

			// Copy r_var bytes into destination.
			const auto dst_r=
				function_context.llvm_ir_builder.CreateInBoundsGEP(
					l_array_type->llvm_type,
					result->llvm_value,
					llvm::Constant::getIntegerValue( fundamental_llvm_types_.u32_, llvm::APInt( 32, 1 ) ) );

			function_context.llvm_ir_builder.CreateMemCpy(
				dst_r, llvm::MaybeAlign(alignment),
				r_var->llvm_value, llvm::MaybeAlign(alignment),
				size_r );
		}
	}

	if( l_var->constexpr_value != nullptr && r_var->constexpr_value != nullptr )
	{
		if( const auto l_constant_data_array= llvm::dyn_cast<llvm::ConstantDataArray>( l_var->constexpr_value ) )
		{
			if( const auto r_constant_data_array= llvm::dyn_cast<llvm::ConstantDataArray>( r_var->constexpr_value ) )
			{
				// Fast path - for constant data arrays.
				const llvm::StringRef l_data= l_constant_data_array->getRawDataValues();
				const llvm::StringRef r_data= r_constant_data_array->getRawDataValues();

				llvm::SmallString<128> concat_result;
				concat_result.resize( l_data.size() + r_data.size() );

				U_ASSERT( concat_result.size() == data_layout_.getTypeAllocSize( result_array_type.llvm_type ) );

				std::memcpy( concat_result.data(), l_data.data(), l_data.size() );
				std::memcpy( concat_result.data() + l_data.size(), r_data.data(), r_data.size() );

				result->constexpr_value=
					llvm::ConstantDataArray::getRaw( concat_result, result_array_type.element_count, result_array_type.element_type.GetLLVMType() );
			}
		}

		if( result->constexpr_value == nullptr )
		{
			// Generic path - process concatenation symbol by symbol.
			llvm::SmallVector<llvm::Constant*, 64> constants;
			constants.resize( size_t(result_array_type.element_count) );

			for( uint64_t i= 0; i < l_array_type->element_count; ++i )
				constants[size_t(i)]= l_var->constexpr_value->getAggregateElement(uint32_t(i));

			for( uint64_t i= 0; i < r_array_type->element_count; ++i )
				constants[size_t(i + l_array_type->element_count)]= r_var->constexpr_value->getAggregateElement(uint32_t(i));

			result->constexpr_value= llvm::ConstantArray::get( result_array_type.llvm_type, constants );
		}
	}

	function_context.variables_state.AddNode( result );
	RegisterTemporaryVariable( function_context, result );
	return result;
}

std::optional<Value> CodeBuilder::TryCallOverloadedUnaryOperator(
	const VariablePtr& variable,
	const OverloadedOperator op,
	const SrcLoc& src_loc,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	if( variable->type.GetClassType() == nullptr )
		return std::nullopt;

	FunctionType::Param args[1];
	args[0].type= variable->type;
	args[0].value_type= variable->value_type;
	const FunctionVariable* const overloaded_operator= GetOverloadedOperator( args, op, names_scope, src_loc );

	if( overloaded_operator == nullptr )
		return std::nullopt;

	overloaded_operator->referenced= true;

	if( !( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
		function_context.has_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

	const std::pair<VariablePtr, llvm::Value*> fetch_result=
		TryFetchVirtualFunction( variable, *overloaded_operator, function_context, names_scope.GetErrors(), src_loc );

	return
		DoCallFunction(
			fetch_result.second,
			overloaded_operator->type,
			src_loc,
			fetch_result.first,
			{},
			false,
			names_scope,
			function_context );
}

std::optional<Value> CodeBuilder::TryCallOverloadedPostfixOperator(
	const VariablePtr& variable,
	const llvm::ArrayRef<Synt::Expression> synt_args,
	const OverloadedOperator op,
	const SrcLoc& src_loc,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	llvm::SmallVector<FunctionType::Param, 16> actual_args;
	actual_args.reserve( 1 + synt_args.size() );

	{
		const bool prev_is_functionless_context= function_context.is_functionless_context;
		function_context.is_functionless_context= true;
		const auto state= SaveFunctionContextState( function_context );
		{
			const StackVariablesStorage dummy_stack_variables_storage( function_context );
			actual_args.push_back( GetArgExtendedType( *variable ) );
			for( const Synt::Expression& arg_expression : synt_args )
				actual_args.push_back( PreEvaluateArg( arg_expression, names_scope, function_context ) );
		}

		RestoreFunctionContextState( function_context, state );
		function_context.is_functionless_context= prev_is_functionless_context;
	}

	const FunctionVariable* const function= GetOverloadedOperator( actual_args, op, names_scope, src_loc );
	if(function == nullptr )
		return std::nullopt;

	if( !( function->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || function->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
		function_context.has_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

	function->referenced= true;

	llvm::SmallVector<const Synt::Expression*, 16> synt_args_ptrs;
	synt_args_ptrs.reserve( synt_args.size() );
	for( const Synt::Expression& arg : synt_args )
		synt_args_ptrs.push_back( &arg );

	const auto fetch_result= TryFetchVirtualFunction( variable, *function, function_context, names_scope.GetErrors(), src_loc );

	return DoCallFunction(
		fetch_result.second,
		function->type,
		src_loc,
		fetch_result.first,
		synt_args_ptrs,
		false,
		names_scope,
		function_context,
		function->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete );
}

Value CodeBuilder::BuildBinaryOperator(
	const Variable& l_var,
	const Variable& r_var,
	const BinaryOperatorType binary_operator,
	const SrcLoc& src_loc,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	using BinaryOperatorType= BinaryOperatorType;

	const Type& l_type= l_var.type;
	const Type& r_type= r_var.type;

	if( ( l_type.GetRawPointerType() != nullptr || r_type.GetRawPointerType() != nullptr ) &&
		( binary_operator == BinaryOperatorType::Add || binary_operator == BinaryOperatorType::Sub ) )
		return BuildBinaryArithmeticOperatorForRawPointers( l_var, r_var, binary_operator, src_loc, names_scope, function_context );

	const FundamentalType* const l_fundamental_type= l_type.GetFundamentalType();
	const FundamentalType* const r_fundamental_type= r_type.GetFundamentalType();

	llvm::Value* const l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
	llvm::Value* const r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );

	const VariableMutPtr result= Variable::Create(
		invalid_type_, // Set later.
		ValueType::Value,
		Variable::Location::LLVMRegister,
		std::string( BinaryOperatorToString(binary_operator) ) );

	switch( binary_operator )
	{
	case BinaryOperatorType::Add:
	case BinaryOperatorType::Sub:
	case BinaryOperatorType::Mul:
	case BinaryOperatorType::Div:
	case BinaryOperatorType::Rem:

		if( r_var.type != l_var.type )
		{
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names_scope.GetErrors(), src_loc, r_var.type, l_var.type,  BinaryOperatorToString( binary_operator ) );
			return ErrorValue();
		}
		if( l_fundamental_type == nullptr )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
			return ErrorValue();
		}
		else
		{
			if( GetFundamentalTypeSize( l_fundamental_type->fundamental_type ) < 4u )
			{
				// Operation supported only for 32 and 64bit operands
				REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}
			const bool is_float= IsFloatingPoint( l_fundamental_type->fundamental_type );
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || is_float ) )
			{
				// this operations allowed only for integer and floating point operands.
				REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}

			if( l_value_for_op != nullptr && r_value_for_op != nullptr )
			{
				const bool is_signed= IsSignedInteger( l_fundamental_type->fundamental_type );

				switch( binary_operator )
				{
				case BinaryOperatorType::Add:
					if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFAdd( l_value_for_op, r_value_for_op );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateAdd( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperatorType::Sub:
					if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFSub( l_value_for_op, r_value_for_op );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateSub( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperatorType::Div:
					if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFDiv( l_value_for_op, r_value_for_op );
					else if( is_signed )
						result->llvm_value= function_context.llvm_ir_builder.CreateSDiv( l_value_for_op, r_value_for_op );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateUDiv( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperatorType::Mul:
					if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFMul( l_value_for_op, r_value_for_op );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateMul( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperatorType::Rem:
					if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFRem( l_value_for_op, r_value_for_op );
					else if( is_signed )
						result->llvm_value= function_context.llvm_ir_builder.CreateSRem( l_value_for_op, r_value_for_op );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateURem( l_value_for_op, r_value_for_op );
					break;
				default: U_ASSERT( false ); break;
				};
			}

			result->type= r_var.type;
		}
		break;

	case BinaryOperatorType::Equal:
	case BinaryOperatorType::NotEqual:
		{
			if( r_var.type != l_var.type )
			{
				REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names_scope.GetErrors(), src_loc, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
				return ErrorValue();
			}
			if( !( l_fundamental_type != nullptr ||
					l_type.GetEnumType() != nullptr ||
					l_type.GetFunctionPointerType() != nullptr ||
					l_type.GetRawPointerType() != nullptr ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}

			if( l_value_for_op != nullptr && r_value_for_op != nullptr )
			{
				const bool is_void= l_fundamental_type != nullptr && l_fundamental_type->fundamental_type == U_FundamentalType::void_;
				const bool is_float= l_fundamental_type != nullptr && IsFloatingPoint( l_fundamental_type->fundamental_type );

				// LLVM constants folder produces wrong compare result for function pointers to "unnamed_addr" functions.
				// Perform manual constant functions compare instead.
				const auto l_function= llvm::dyn_cast<llvm::Function>( l_value_for_op );
				const auto r_function= llvm::dyn_cast<llvm::Function>( r_value_for_op );

				// Use ordered floating point compare operations, which result is false for NaN, except !=. nan != nan must be true.
				switch( binary_operator )
				{
				case BinaryOperatorType::Equal:
					if( is_void )
						result->llvm_value= llvm::ConstantInt::getTrue( llvm_context_ ); // All "void" values are same.
					else if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFCmpOEQ( l_value_for_op, r_value_for_op );
					else if( l_function != nullptr && r_function != nullptr )
						result->llvm_value= llvm::ConstantInt::getBool( llvm_context_, l_function == r_function );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateICmpEQ( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperatorType::NotEqual:
					if( is_void )
						result->llvm_value= llvm::ConstantInt::getFalse( llvm_context_ ); // All "void" values are same.
					else if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFCmpUNE( l_value_for_op, r_value_for_op );
					else if( l_function != nullptr && r_function != nullptr )
						result->llvm_value= llvm::ConstantInt::getBool( llvm_context_, l_function != r_function );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateICmpNE( l_value_for_op, r_value_for_op );
					break;

				default: U_ASSERT( false ); break;
				};
			}

			result->type= bool_type_;
		}
		break;

	case BinaryOperatorType::Less:
	case BinaryOperatorType::LessEqual:
	case BinaryOperatorType::Greater:
	case BinaryOperatorType::GreaterEqual:
		{
			if( r_var.type != l_var.type )
			{
				REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names_scope.GetErrors(), src_loc, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
				return ErrorValue();
			}
			if( !( l_fundamental_type != nullptr || l_type.GetRawPointerType() != nullptr || l_type.GetEnumType() != nullptr ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}

			bool is_float= false, is_signed= false;
			if( l_fundamental_type != nullptr )
			{
				const auto t= l_fundamental_type->fundamental_type;
				is_float= IsFloatingPoint( t );
				is_signed= IsSignedInteger( t );
				if( !( IsInteger(t) || IsChar(t) || is_float ) )
				{
					REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
					return ErrorValue();
				}
			}
			else if( const auto enum_type= l_type.GetEnumType() )
				is_signed= IsSignedInteger( enum_type->underlying_type.fundamental_type );

			if( l_value_for_op != nullptr && r_value_for_op != nullptr )
			{
				switch( binary_operator )
				{
				// Use ordered floating point compare operations, which result is false for NaN.
				case BinaryOperatorType::Less:
					if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFCmpOLT( l_value_for_op, r_value_for_op );
					else if( is_signed )
						result->llvm_value= function_context.llvm_ir_builder.CreateICmpSLT( l_value_for_op, r_value_for_op );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateICmpULT( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperatorType::LessEqual:
					if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFCmpOLE( l_value_for_op, r_value_for_op );
					else if( is_signed )
						result->llvm_value= function_context.llvm_ir_builder.CreateICmpSLE( l_value_for_op, r_value_for_op );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateICmpULE( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperatorType::Greater:
					if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFCmpOGT( l_value_for_op, r_value_for_op );
					else if( is_signed )
						result->llvm_value= function_context.llvm_ir_builder.CreateICmpSGT( l_value_for_op, r_value_for_op );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateICmpUGT( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperatorType::GreaterEqual:
					if( is_float )
						result->llvm_value= function_context.llvm_ir_builder.CreateFCmpOGE( l_value_for_op, r_value_for_op );
					else if( is_signed )
						result->llvm_value= function_context.llvm_ir_builder.CreateICmpSGE( l_value_for_op, r_value_for_op );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateICmpUGE( l_value_for_op, r_value_for_op );
					break;

				default: U_ASSERT( false ); break;
				};
			}

			result->type= bool_type_;
		}
		break;

	case BinaryOperatorType::CompareOrder:
		{
			if( r_var.type != l_var.type )
			{
				REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names_scope.GetErrors(), src_loc, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
				return ErrorValue();
			}
			if( !( l_fundamental_type != nullptr || l_type.GetRawPointerType() != nullptr || l_type.GetEnumType() != nullptr ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}

			bool is_float= false, is_signed= false;
			if( l_fundamental_type != nullptr )
			{
				const auto t= l_fundamental_type->fundamental_type;
				is_float= IsFloatingPoint( t );
				is_signed= IsSignedInteger( t );
				if( !( IsInteger(t) || IsChar(t) || is_float ) )
				{
					REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
					return ErrorValue();
				}
			}
			else if( const auto enum_type= l_type.GetEnumType() )
				is_signed= IsSignedInteger( enum_type->underlying_type.fundamental_type );

			const auto result_fundamental_type= U_FundamentalType::i32_;
			const auto result_llvm_type= GetFundamentalLLVMType( result_fundamental_type );

			if( l_value_for_op != nullptr && r_value_for_op != nullptr )
			{
				llvm::Value* less= nullptr;
				llvm::Value* greater= nullptr;

				if( is_float )
					less= function_context.llvm_ir_builder.CreateFCmpOLT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					less= function_context.llvm_ir_builder.CreateICmpSLT( l_value_for_op, r_value_for_op );
				else
					less= function_context.llvm_ir_builder.CreateICmpULT( l_value_for_op, r_value_for_op );

				if( is_float )
					greater= function_context.llvm_ir_builder.CreateFCmpOGT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					greater= function_context.llvm_ir_builder.CreateICmpSGT( l_value_for_op, r_value_for_op );
				else
					greater= function_context.llvm_ir_builder.CreateICmpUGT( l_value_for_op, r_value_for_op );

				const auto zero= llvm::ConstantInt::get( result_llvm_type, uint64_t(0), true );
				const auto plus_one= llvm::ConstantInt::get( result_llvm_type, uint64_t(1), true );
				const auto minus_one= llvm::ConstantInt::get( result_llvm_type, uint64_t(int64_t(-1)), true );

				result->llvm_value=
					function_context.llvm_ir_builder.CreateSelect(
						less,
						minus_one,
						function_context.llvm_ir_builder.CreateSelect( greater, plus_one, zero ) );
			}

			result->type= FundamentalType( result_fundamental_type, result_llvm_type );
		}
		break;

	case BinaryOperatorType::And:
	case BinaryOperatorType::Or:
	case BinaryOperatorType::Xor:

		if( r_var.type != l_var.type )
		{
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names_scope.GetErrors(), src_loc, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
			return ErrorValue();
		}
		if( l_fundamental_type == nullptr )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
			return ErrorValue();
		}
		else
		{
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || l_fundamental_type->fundamental_type == U_FundamentalType::bool_ ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}

			if( l_value_for_op != nullptr && r_value_for_op != nullptr )
			{
				switch( binary_operator )
				{
				case BinaryOperatorType::And:
					result->llvm_value= function_context.llvm_ir_builder.CreateAnd( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperatorType::Or:
					result->llvm_value= function_context.llvm_ir_builder.CreateOr( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperatorType::Xor:
					result->llvm_value= function_context.llvm_ir_builder.CreateXor( l_value_for_op, r_value_for_op );
					break;

				default: U_ASSERT( false ); break;
				};
			}

			result->type= l_type;
		}
		break;

	case BinaryOperatorType::ShiftLeft :
	case BinaryOperatorType::ShiftRight:
		{
			if( l_fundamental_type == nullptr || !IsInteger( l_fundamental_type->fundamental_type ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}
			if( r_fundamental_type == nullptr || !IsUnsignedInteger( r_fundamental_type->fundamental_type ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, r_type );
				return ErrorValue();
			}

			if( l_value_for_op != nullptr && r_value_for_op != nullptr )
			{
				const uint64_t l_type_size= GetFundamentalTypeSize( l_fundamental_type->fundamental_type );
				const uint64_t r_type_size= GetFundamentalTypeSize( r_fundamental_type->fundamental_type );

				llvm::Value* r_value_converted= r_value_for_op;

				// Convert value of shift to type of shifted value. LLVM Reuqired this.
				if( r_type_size > l_type_size )
					r_value_converted= function_context.llvm_ir_builder.CreateTrunc( r_value_converted, l_var.type.GetLLVMType() );
				else if( r_type_size < l_type_size )
					r_value_converted= function_context.llvm_ir_builder.CreateZExt( r_value_converted, l_var.type.GetLLVMType() );

				// Cut upper bits of shift value to avoid undefined behaviour.
				r_value_converted =
					function_context.llvm_ir_builder.CreateAnd(
						r_value_converted,
						llvm::ConstantInt::get( l_var.type.GetLLVMType(), l_type_size * 8 - 1 ) );

				if( binary_operator == BinaryOperatorType::ShiftLeft )
					result->llvm_value= function_context.llvm_ir_builder.CreateShl( l_value_for_op, r_value_converted );
				else if( binary_operator == BinaryOperatorType::ShiftRight )
				{
					if( IsSignedInteger( l_fundamental_type->fundamental_type ) )
						result->llvm_value= function_context.llvm_ir_builder.CreateAShr( l_value_for_op, r_value_converted );
					else
						result->llvm_value= function_context.llvm_ir_builder.CreateLShr( l_value_for_op, r_value_converted );
				}
				else U_ASSERT(false);
			}

			result->type= l_type;
		}
		break;

	case BinaryOperatorType::LazyLogicalAnd:
	case BinaryOperatorType::LazyLogicalOr:
		U_ASSERT(false);
		break;
	};

	// Produce constexpr value only for constexpr arguments.
	if( l_var.constexpr_value != nullptr && r_var.constexpr_value != nullptr && result->llvm_value != nullptr )
		result->constexpr_value= llvm::dyn_cast<llvm::Constant>(result->llvm_value);

	if( result->constexpr_value != nullptr )
	{
		// Undef value can occurs in integer division by zero or something like it.
		// But, if inputs are undef, this means, that they are template-dependent and this is not error case.
		if( llvm::dyn_cast<llvm::UndefValue >(result->constexpr_value) != nullptr )
		{
			REPORT_ERROR( ConstantExpressionResultIsUndefined, names_scope.GetErrors(), src_loc );
			result->constexpr_value= nullptr;
		}
	}

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildBinaryArithmeticOperatorForRawPointers(
	const Variable& l_var,
	const Variable& r_var,
	BinaryOperatorType binary_operator,
	const SrcLoc& src_loc,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	U_ASSERT( l_var.type.GetRawPointerType() != nullptr || r_var.type.GetRawPointerType() != nullptr );

	// Pointer arithmetic considered to be unsafe, since overflow is undefined behavior.
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( RawPointerArithmeticOutsideUnsafeBlock, names_scope.GetErrors(), src_loc );

	llvm::Value* const l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
	llvm::Value* const r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );

	const VariableMutPtr result= Variable::Create(
		invalid_type_, // Set later.
		ValueType::Value,
		Variable::Location::LLVMRegister,
		std::string( BinaryOperatorToString(binary_operator) ) );

	if( binary_operator == BinaryOperatorType::Add )
	{
		const uint64_t ptr_size= fundamental_llvm_types_.size_type_->getIntegerBitWidth() / 8;
		U_FundamentalType int_type= U_FundamentalType::InvalidType;

		llvm::Value* ptr_value= nullptr;
		llvm::Value* index_value= nullptr;

		if( const auto l_fundamental_type= l_var.type.GetFundamentalType() )
		{
			int_type= l_fundamental_type->fundamental_type;
			index_value= l_value_for_op;

			U_ASSERT( r_var.type.GetRawPointerType() != nullptr );
			result->type= r_var.type;
			ptr_value= r_value_for_op;
		}
		else if( const auto r_fundamental_type= r_var.type.GetFundamentalType() )
		{
			int_type= r_fundamental_type->fundamental_type;
			index_value= r_value_for_op;

			U_ASSERT( l_var.type.GetRawPointerType() != nullptr );
			result->type= l_var.type;
			ptr_value= l_value_for_op;
		}
		else
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, l_var.type );
			return ErrorValue();
		}

		const uint64_t int_size= GetFundamentalTypeSize( int_type );

		const Type& element_type= result->type.GetRawPointerType()->element_type;
		if( !EnsureTypeComplete( element_type ) )
		{
			// Complete types required for pointer arithmetic.
			REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), src_loc, element_type );
			return ErrorValue();
		}
		if( !IsInteger( int_type ) || int_size > ptr_size )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, GetFundamentalTypeName( int_type ) );
			return ErrorValue();
		}

		if( !function_context.is_functionless_context )
		{
			if( int_size < ptr_size )
			{
				if( IsSignedInteger( int_type ) )
					index_value= function_context.llvm_ir_builder.CreateSExt( index_value, fundamental_llvm_types_.size_type_ );
				else
					index_value= function_context.llvm_ir_builder.CreateZExt( index_value, fundamental_llvm_types_.size_type_ );
			}
			result->llvm_value= function_context.llvm_ir_builder.CreateInBoundsGEP( element_type.GetLLVMType(), ptr_value, index_value );
		}
	}
	else if( binary_operator == BinaryOperatorType::Sub )
	{
		const auto ptr_type= l_var.type.GetRawPointerType();
		if( ptr_type == nullptr )
		{
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names_scope.GetErrors(), src_loc, l_var.type, r_var.type, BinaryOperatorToString( binary_operator ) );
			return ErrorValue();
		}

		if( !EnsureTypeComplete( ptr_type->element_type ) )
		{
			// Complete types required for pointer arithmetic.
			REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), src_loc, ptr_type->element_type );
			return ErrorValue();
		}

		if( const auto r_ptr_type= r_var.type.GetRawPointerType() )
		{
			// Pointer difference.
			if( *r_ptr_type != *ptr_type )
			{
				REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names_scope.GetErrors(), src_loc, l_var.type, r_var.type, BinaryOperatorToString( binary_operator ) );
				return ErrorValue();
			}

			// Result is special "ssize_type" - like "size_type", but signed.
			// Its size is equal to pointer size.
			llvm::Type* const diff_llvm_type= fundamental_llvm_types_.ssize_type_;
			result->type= ssize_type_;

			const auto element_size= data_layout_.getTypeAllocSize( ptr_type->element_type.GetLLVMType() );
			if( element_size == 0 )
			{
				REPORT_ERROR( DifferenceBetweenRawPointersWithZeroElementSize, names_scope.GetErrors(), src_loc, l_var.type );
				return ErrorValue();
			}

			if( !function_context.is_functionless_context )
			{
				llvm::Value* const l_as_int= function_context.llvm_ir_builder.CreatePtrToInt( l_value_for_op, diff_llvm_type );
				llvm::Value* const r_as_int= function_context.llvm_ir_builder.CreatePtrToInt( r_value_for_op, diff_llvm_type );
				llvm::Value* const diff= function_context.llvm_ir_builder.CreateSub( l_as_int, r_as_int );
				llvm::Value* const element_size_constant= llvm::ConstantInt::get( diff_llvm_type, uint64_t(element_size), false );
				llvm::Value* const diff_divided= function_context.llvm_ir_builder.CreateSDiv( diff, element_size_constant, "", true /* exact */ );
				result->llvm_value= diff_divided;
			}
		}
		else if( const auto r_fundamental_type= r_var.type.GetFundamentalType() )
		{
			// Subtract integer from pointer.

			const uint64_t ptr_size= fundamental_llvm_types_.size_type_->getIntegerBitWidth() / 8;
			const uint64_t int_size= GetFundamentalTypeSize( r_fundamental_type->fundamental_type );

			if( !IsInteger( r_fundamental_type->fundamental_type ) || int_size > ptr_size )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, r_var.type );
				return ErrorValue();
			}

			result->type= l_var.type;

			if( !function_context.is_functionless_context )
			{
				llvm::Value* index_value= r_value_for_op;
				if( int_size < ptr_size )
				{
					if( IsSignedInteger( r_fundamental_type->fundamental_type ) )
						index_value= function_context.llvm_ir_builder.CreateSExt( index_value, fundamental_llvm_types_.size_type_ );
					else
						index_value= function_context.llvm_ir_builder.CreateZExt( index_value, fundamental_llvm_types_.size_type_ );
				}
				llvm::Value* const index_value_negative= function_context.llvm_ir_builder.CreateNeg( index_value );
				result->llvm_value= function_context.llvm_ir_builder.CreateInBoundsGEP( ptr_type->element_type.GetLLVMType(), l_value_for_op, index_value_negative );
			}
		}
		else
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), src_loc, r_var.type );
			return ErrorValue();
		}
	}
	else{ U_ASSERT(false); }

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::BuildLazyBinaryOperator(
	const Synt::Expression& l_expression,
	const Synt::Expression& r_expression,
	const Synt::BinaryOperator& binary_operator,
	const SrcLoc& src_loc,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	// TODO - maybe create separate variables stack frame for right expression evaluation and call destructors?
	const VariablePtr l_var= BuildExpressionCodeEnsureVariable( l_expression, names_scope, function_context );

	if( l_var->type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), binary_operator.src_loc, bool_type_, l_var->type );
		return ErrorValue();
	}

	llvm::Value* const l_var_in_register= CreateMoveToLLVMRegisterInstruction( *l_var, function_context );

	llvm::BasicBlock* const l_part_block= function_context.llvm_ir_builder.GetInsertBlock();
	llvm::BasicBlock* r_part_block= nullptr;
	llvm::BasicBlock* block_after_operator= nullptr;

	if( !function_context.is_functionless_context )
	{
		r_part_block= llvm::BasicBlock::Create( llvm_context_ );
		block_after_operator= llvm::BasicBlock::Create( llvm_context_ );

		if( binary_operator.operator_type == BinaryOperatorType::LazyLogicalAnd )
			function_context.llvm_ir_builder.CreateCondBr( l_var_in_register, r_part_block, block_after_operator );
		else if( binary_operator.operator_type == BinaryOperatorType::LazyLogicalOr )
			function_context.llvm_ir_builder.CreateCondBr( l_var_in_register, block_after_operator, r_part_block );
		else U_ASSERT(false);

		r_part_block->insertInto( function_context.function );
		function_context.llvm_ir_builder.SetInsertPoint( r_part_block );
	}

	ReferencesGraph variables_state_before_r_branch= function_context.variables_state;

	llvm::Value* r_var_in_register= nullptr;
	llvm::Constant* r_var_constepxr_value= nullptr;
	{
		// Right part of lazy operator is conditinal. So, we must destroy its temporaries only in this condition.
		// We doesn`t needs longer lifetime of expression temporaries, because we use only bool result.
		const StackVariablesStorage r_var_temp_variables_storage( function_context );

		const VariablePtr r_var= BuildExpressionCodeEnsureVariable( r_expression, names_scope, function_context );
		if( r_var->type != bool_type_ )
		{
			REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), binary_operator.src_loc, bool_type_, r_var->type );
			r_var_in_register= llvm::UndefValue::get( fundamental_llvm_types_.bool_ );
		}
		else
		{
			r_var_constepxr_value= r_var->constexpr_value;
			r_var_in_register= CreateMoveToLLVMRegisterInstruction( *r_var, function_context );
		}

		// Destroy r_var temporaries in this branch.
		CallDestructors( r_var_temp_variables_storage, names_scope, function_context, src_loc );
	}
	function_context.variables_state= MergeVariablesStateAfterIf( { variables_state_before_r_branch, function_context.variables_state }, names_scope.GetErrors(), src_loc );

	const VariableMutPtr result=
		Variable::Create(
			bool_type_,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			std::string( BinaryOperatorToString(binary_operator.operator_type) ) );

	if( !function_context.is_functionless_context )
	{
		llvm::BasicBlock* const r_part_end_block= function_context.llvm_ir_builder.GetInsertBlock();

		U_ASSERT( block_after_operator != nullptr );
		function_context.llvm_ir_builder.CreateBr( block_after_operator );
		block_after_operator->insertInto( function_context.function );
		function_context.llvm_ir_builder.SetInsertPoint( block_after_operator );

		llvm::PHINode* const phi= function_context.llvm_ir_builder.CreatePHI( fundamental_llvm_types_.bool_, 2u );
		phi->addIncoming( l_var_in_register, l_part_block );
		phi->addIncoming( r_var_in_register, r_part_end_block );
		result->llvm_value= phi;
	}

	// Evaluate constexpr value.
	// TODO - remove all blocks code in case of constexpr?
	if( l_var->constexpr_value != nullptr && r_var_constepxr_value != nullptr )
	{
		if( binary_operator.operator_type == BinaryOperatorType::LazyLogicalAnd )
			result->constexpr_value= llvm::ConstantExpr::getAnd( l_var->constexpr_value, r_var_constepxr_value );
		else if( binary_operator.operator_type == BinaryOperatorType::LazyLogicalOr )
			result->constexpr_value= llvm::ConstantExpr::getOr ( l_var->constexpr_value, r_var_constepxr_value );
		else
			U_ASSERT(false);
	}

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return result;
}

Value CodeBuilder::DoReferenceCast(
	const SrcLoc& src_loc,
	const Synt::TypeName& type_name,
	const Synt::Expression& expression,
	bool enable_unsafe,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	const Type type= PrepareType( type_name, names_scope, function_context );
	if( type == invalid_type_ )
		return ErrorValue();

	// Complete types required for both safe and unsafe casting.
	// This needs, becasue we must emit same code for places where types yet not complete, and where they are complete.
	if( !EnsureTypeComplete( type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), src_loc, type );
		return ErrorValue();
	}

	const VariablePtr var= BuildExpressionCodeEnsureVariable( expression, names_scope, function_context );

	if( !EnsureTypeComplete( var->type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), src_loc, var->type );
		return ErrorValue();
	}

	const bool types_are_compatible= ReferenceIsConvertible( var->type, type, names_scope.GetErrors(), src_loc );

	const VariableMutPtr result=
		Variable::Create(
			type,
			var->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut, // "ValueType" here converts into ConstReference
			Variable::Location::Pointer,
			"cast</" + type.ToString() + "/>(" + var->name + ")" );
	function_context.variables_state.AddNode( result );
	function_context.variables_state.TryAddLink( var, result, names_scope.GetErrors(), src_loc );

	if( types_are_compatible )
		function_context.variables_state.TryAddInnerLinks( var, result, names_scope.GetErrors(), src_loc );

	llvm::Value* src_value= var->llvm_value;
	if( var->location == Variable::Location::LLVMRegister )
	{
		if( !function_context.is_functionless_context )
		{
			src_value= function_context.alloca_ir_builder.CreateAlloca( var->type.GetLLVMType() );
			CreateTypedStore( function_context, var->type, var->llvm_value, src_value );
		}
	}

	if( type == var->type )
		result->llvm_value= src_value;
	else
	{
		if( types_are_compatible )
		{
			if( !function_context.is_functionless_context )
				result->llvm_value= CreateReferenceCast( src_value, var->type, type, function_context );
		}
		else
		{
			if( !function_context.is_functionless_context )
				result->llvm_value= function_context.llvm_ir_builder.CreatePointerCast( src_value, type.GetLLVMType()->getPointerTo() );
			if( !enable_unsafe )
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, type, var->type );
		}
	}

	RegisterTemporaryVariable( function_context, result );

	return result;
}

Value CodeBuilder::CallFunctionValue(
	const Value& function_value,
	const llvm::ArrayRef<Synt::Expression> synt_args,
	const SrcLoc& call_src_loc,
	const std::optional<SrcLoc>& function_value_src_loc,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(function_value);

	if( const Type* const type= function_value.GetTypeName() )
		return BuildTempVariableConstruction( *type, synt_args, call_src_loc, names_scope, function_context );

	VariablePtr this_;
	OverloadedFunctionsSetConstPtr functions_set= function_value.GetFunctionsSet();

	if( functions_set != nullptr )
	{}
	else if( const ThisOverloadedMethodsSet* const this_overloaded_methods_set=
		function_value.GetThisOverloadedMethodsSet() )
	{
		functions_set= this_overloaded_methods_set->overloaded_methods_set;
		this_= this_overloaded_methods_set->this_;
	}
	else if( const VariablePtr callable_variable= function_value.GetVariable() )
	{
		if( const FunctionPointerType* const function_pointer= callable_variable->type.GetFunctionPointerType() )
		{
			function_context.has_non_constexpr_operations_inside= true; // Calling function, using pointer, is not constexpr. We can not garantee, that called function is constexpr.

			// Call function pointer directly.
			if( function_pointer->function_type.params.size() != synt_args.size() )
			{
				REPORT_ERROR( InvalidFunctionArgumentCount, names_scope.GetErrors(), call_src_loc, synt_args.size(), function_pointer->function_type.params.size() );
				return ErrorValue();
			}

			llvm::SmallVector<const Synt::Expression*, 16> args;
			args.reserve( synt_args.size() );
			for( const Synt::Expression& arg : synt_args )
				args.push_back( &arg );

			llvm::Value* const func_itself= CreateMoveToLLVMRegisterInstruction( *callable_variable, function_context );

			return
				DoCallFunction(
					func_itself, function_pointer->function_type, call_src_loc,
					nullptr, args, false,
					names_scope, function_context );
		}

		// Try to call overloaded () operator.
		// DO NOT fill "this" here and continue this function because we should process callable object as non-this.

		if( auto res= TryCallOverloadedPostfixOperator( callable_variable, synt_args, OverloadedOperator::Call, call_src_loc, names_scope, function_context ) )
			return std::move(*res);
	}

	if( functions_set == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names_scope.GetErrors(), call_src_loc, function_value.GetKindName() );
		return ErrorValue();
	}

	const FunctionVariable* function_ptr= nullptr;

	// Make preevaluation af arguments for selection of overloaded function.
	{
		llvm::SmallVector<FunctionType::Param, 16> actual_args;
		actual_args.reserve( (this_ == nullptr ? 0u : 1u) + synt_args.size() );

		{
			const bool prev_is_functionless_context= function_context.is_functionless_context;
			function_context.is_functionless_context= true;
			const auto state= SaveFunctionContextState( function_context );
			{
				const StackVariablesStorage dummy_stack_variables_storage( function_context );

				if( this_ != nullptr )
					actual_args.push_back( GetArgExtendedType( *this_ ) );

				for( const Synt::Expression& arg_expression : synt_args )
					actual_args.push_back( PreEvaluateArg( arg_expression, names_scope, function_context ) );
			}

			RestoreFunctionContextState( function_context, state );
			function_context.is_functionless_context= prev_is_functionless_context;
		}

		function_ptr=
			GetOverloadedFunction( *functions_set, actual_args, this_ != nullptr, names_scope.GetErrors(), call_src_loc );
	}

	// SPRACHE_TODO - try get function with "this" parameter in signature and without it.
	// We must support static functions call using "this".
	if( function_ptr == nullptr )
	{
		if( function_value_src_loc != std::nullopt && !functions_set->functions.empty() )
		{
			// Collect definition point for some function if selection is failed.
			// This may help in language server - to go to some definition in order to know exact signature and fix error point.
			CollectFunctionDefinition( functions_set->functions.front(), *function_value_src_loc );
		}

		return ErrorValue();
	}

	const FunctionVariable& function= *function_ptr;
	const FunctionType& function_type= function.type;

	function.referenced= true;

	if( function_value_src_loc != std::nullopt )
	{
		// Collect definition point for specific selected function.
		CollectFunctionDefinition( function, *function_value_src_loc );
	}

	if( this_ != nullptr && !function.is_this_call )
	{
		// Static function call via "this".
		// Just dump first "this" arg.
		this_= nullptr;
	}

	if( function_ptr->is_deleted )
		REPORT_ERROR( AccessingDeletedMethod, names_scope.GetErrors(), call_src_loc );

	if( !( function_ptr->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || function_ptr->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
		function_context.has_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

	llvm::SmallVector<const Synt::Expression*, 16> synt_args_ptrs;
	synt_args_ptrs.reserve( synt_args.size() );
	for( const Synt::Expression& arg : synt_args )
		synt_args_ptrs.push_back( &arg );

	llvm::Value* llvm_function_ptr= EnsureLLVMFunctionCreated( function );
	if( this_ != nullptr )
	{
		auto fetch_result= TryFetchVirtualFunction( this_, function, function_context, names_scope.GetErrors(), call_src_loc );
		llvm_function_ptr= fetch_result.second;
		this_= fetch_result.first;
	}

	return
		DoCallFunction(
			llvm_function_ptr, function_type,
			call_src_loc,
			this_,
			synt_args_ptrs, false,
			names_scope, function_context,
			function.constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete );
}

Value CodeBuilder::DoCallFunction(
	llvm::Value* const function,
	const FunctionType& function_type,
	const SrcLoc& call_src_loc,
	const VariablePtr& this_, // optional
	const llvm::ArrayRef<const Synt::Expression*> args,
	const bool evaluate_args_in_reverse_order,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const bool func_is_constexpr )
{
	return DoCallFunction(
		function,
		function_type,
		call_src_loc,
		this_ == nullptr ? llvm::ArrayRef<VariablePtr>() : llvm::ArrayRef<VariablePtr>( this_ ),
		args,
		evaluate_args_in_reverse_order,
		names_scope,
		function_context,
		func_is_constexpr );
}

Value CodeBuilder::DoCallFunction(
	llvm::Value* function,
	const FunctionType& function_type,
	const SrcLoc& call_src_loc,
	const llvm::ArrayRef<VariablePtr> preevaluated_args,
	const llvm::ArrayRef<const Synt::Expression*> args,
	const bool evaluate_args_in_reverse_order,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const bool func_is_constexpr )
{
	if( function_type.unsafe && !function_context.is_in_unsafe_block )
		REPORT_ERROR( UnsafeFunctionCallOutsideUnsafeBlock, names_scope.GetErrors(), call_src_loc );

	const size_t arg_count= preevaluated_args.size() + args.size();
	U_ASSERT( arg_count == function_type.params.size() );

	llvm::SmallVector<llvm::Value*, 16> llvm_args;
	llvm::SmallVector<llvm::Constant*, 16> constant_llvm_args;
	llvm_args.resize( arg_count, nullptr );

	// TODO - use vector of pairs instead.
	llvm::SmallVector< VariablePtr, 16 > args_nodes;
	args_nodes.resize( arg_count, nullptr );

	llvm::SmallVector< std::vector<VariablePtr>, 16 > second_order_reference_nodes;

	llvm::SmallVector<llvm::Value*, 16> value_args_for_lifetime_end_call;

	for( size_t i= 0u; i < arg_count; ++i )
	{
		const size_t arg_number= evaluate_args_in_reverse_order ? arg_count - i - 1u : i;

		const FunctionType::Param& param= function_type.params[arg_number];

		VariablePtr expr;
		SrcLoc src_loc;
		if( arg_number < preevaluated_args.size() )
		{
			expr= preevaluated_args[arg_number];
			src_loc= call_src_loc;
		}
		else
		{
			expr= BuildExpressionCodeEnsureVariable( *args[ arg_number - preevaluated_args.size() ], names_scope, function_context );
			src_loc= Synt::GetSrcLoc( *args[ arg_number - preevaluated_args.size() ] );
		}

		if( param.value_type != ValueType::Value )
		{
			if( !ReferenceIsConvertible( expr->type, param.type, names_scope.GetErrors(), call_src_loc ) &&
				!HasConversionConstructor( FunctionType::Param{ expr->type, expr->value_type }, param.type, names_scope.GetErrors(), src_loc ) )
			{
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, param.type, expr->type );
				continue;
			}

			if( param.value_type == ValueType::ReferenceMut )
			{
				if( expr->value_type == ValueType::Value )
				{
					REPORT_ERROR( ExpectedReferenceValue, names_scope.GetErrors(), src_loc );
					continue;
				}
				if( expr->value_type == ValueType::ReferenceImut )
				{
					REPORT_ERROR( BindingConstReferenceToNonconstReference, names_scope.GetErrors(), src_loc );
					continue;
				}

				llvm_args[arg_number]= CreateReferenceCast( expr->llvm_value, expr->type, param.type, function_context );
			}
			else
			{
				if( expr->constexpr_value != nullptr )
					constant_llvm_args.push_back( expr->constexpr_value );

				if( expr->value_type == ValueType::Value && expr->location == Variable::Location::LLVMRegister )
				{
					if( !function_context.is_functionless_context )
					{
						// Bind value to const reference.
						llvm::Value* const temp_storage= function_context.alloca_ir_builder.CreateAlloca( expr->type.GetLLVMType() );
						CreateTypedStore( function_context, expr->type, expr->llvm_value, temp_storage );
						llvm_args[arg_number]= temp_storage;
						// Do not call here lifetime.start since there is no way to call lifetime.end for this value, because this allocation logically linked with some temp variable and can extend it's lifetime.
					}
				}
				else
					llvm_args[arg_number]= expr->llvm_value;

				if( expr->type != param.type )
				{
					if( expr->type.ReferenceIsConvertibleTo( param.type ) )
						llvm_args[arg_number]= CreateReferenceCast( llvm_args[arg_number], expr->type, param.type, function_context );
					else
					{
						const auto conversion_constructor=
							GetConversionConstructor(
								FunctionType::Param{ expr->type, expr->value_type },
								param.type,
								names_scope.GetErrors(),
								src_loc );

						U_ASSERT( conversion_constructor != nullptr );
						expr= ConvertVariable( expr, param.type, *conversion_constructor, names_scope, function_context, src_loc );
						llvm_args[arg_number]= expr->llvm_value;
					}
				}
			}

			// Lock references.
			const VariableMutPtr arg_node=
				Variable::Create(
				param.type,
				param.value_type,
				Variable::Location::Pointer,
				"reference_arg " + std::to_string(i),
				llvm_args[arg_number] );
			function_context.variables_state.AddNode( arg_node );
			function_context.variables_state.TryAddLink( expr, arg_node, names_scope.GetErrors(), src_loc );
			function_context.variables_state.TryAddInnerLinks( expr, arg_node, names_scope.GetErrors(), src_loc );

			// Register node for destruction in case of return in further args evaluation.
			arg_node->preserve_temporary= true;
			RegisterTemporaryVariable( function_context, arg_node );
			args_nodes[arg_number]= arg_node;
		}
		else
		{
			const VariableMutPtr arg_node=
				Variable::Create(
					param.type,
					ValueType::Value,
					Variable::Location::Pointer, // Set later
					"value_arg_" + std::to_string(i) );

			function_context.variables_state.AddNode( arg_node );

			if( !ReferenceIsConvertible( expr->type, param.type, names_scope.GetErrors(), call_src_loc ) &&
				!HasConversionConstructor( FunctionType::Param{ expr->type, expr->value_type }, param.type, names_scope.GetErrors(), src_loc ) )
			{
				REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), src_loc, param.type, expr->type );
				continue;
			}

			if( expr->type != param.type )
			{
				if( expr->type.ReferenceIsConvertibleTo( param.type ) ){}
				else
				{
					const auto conversion_constructor=
						GetConversionConstructor(
							FunctionType::Param{ expr->type, expr->value_type },
							param.type,
							names_scope.GetErrors(),
							src_loc );

					U_ASSERT( conversion_constructor != nullptr );
					expr= ConvertVariable( expr, param.type, *conversion_constructor, names_scope, function_context, src_loc );
				}
			}

			if( param.type.GetFundamentalType() != nullptr ||
				param.type.GetEnumType() != nullptr ||
				param.type.GetRawPointerType() != nullptr ||
				param.type.GetFunctionPointerType() != nullptr )
			{
				llvm_args[arg_number]= CreateMoveToLLVMRegisterInstruction( *expr, function_context );

				if( expr->constexpr_value != nullptr )
					constant_llvm_args.push_back( expr->constexpr_value );

				arg_node->llvm_value= llvm_args[arg_number];
				arg_node->location= Variable::Location::LLVMRegister;
			}
			else if( param.type.GetClassType() != nullptr || param.type.GetArrayType() != nullptr || param.type.GetTupleType() != nullptr )
			{
				// Lock inner references.
				// Do it only if arg type can contain any reference inside.
				// Do it before potential moving.
				EnsureTypeComplete( param.type ); // arg type for value arg must be already complete.
				function_context.variables_state.TryAddInnerLinks( expr, arg_node, names_scope.GetErrors(), src_loc );

				llvm::Type* const single_scalar_type= GetSingleScalarType( param.type.GetLLVMType() );

				if( expr->constexpr_value != nullptr )
				{
					if( single_scalar_type == nullptr )
						constant_llvm_args.push_back( expr->constexpr_value );
					else
						constant_llvm_args.push_back( UnwrapRawScalarConstant( expr->constexpr_value ) );
				}

				if( expr->value_type == ValueType::Value && expr->type == param.type )
				{
					// Do not call copy constructors - just move.
					function_context.variables_state.MoveNode( expr );

					if( !function_context.is_functionless_context )
					{
						if( single_scalar_type == nullptr )
							llvm_args[arg_number]= expr->llvm_value;
						else
							llvm_args[arg_number]= function_context.llvm_ir_builder.CreateLoad( single_scalar_type, expr->llvm_value );

						// Save address into temporary container to call lifetime.end after call.
						value_args_for_lifetime_end_call.push_back( expr->llvm_value );
						arg_node->llvm_value= expr->llvm_value;
					}
				}
				else
				{
					if( !param.type.IsCopyConstructible() )
					{
						// Can not call function with value parameter, because for value parameter needs copy, but parameter type is not copyable.
						REPORT_ERROR( CopyConstructValueOfNoncopyableType, names_scope.GetErrors(), src_loc, param.type );
						continue;
					}
					// Allow value params of abstract types (it is useful in templates) but disallow call of such functions.
					if( param.type.IsAbstract() )
					{
						REPORT_ERROR( ConstructingAbstractClassOrInterface, names_scope.GetErrors(), src_loc, param.type );
						continue;
					}

					if( !function_context.is_functionless_context )
					{
						// Create copy of class or tuple value. Call copy constructor.
						llvm::Value* const arg_copy= function_context.alloca_ir_builder.CreateAlloca( param.type.GetLLVMType() );

						// Create lifetime.start instruction for value arg.
						CreateLifetimeStart( function_context, arg_copy );

						BuildCopyConstructorPart(
							arg_copy,
							CreateReferenceCast( expr->llvm_value, expr->type, param.type, function_context ),
							param.type,
							function_context );

						if( single_scalar_type == nullptr )
						{
							// Pass by hidden reference.
							llvm_args[arg_number]= arg_copy;
						}
						else
						{
							// If this is a single scalar type - just load value.
							llvm_args[arg_number]= function_context.llvm_ir_builder.CreateLoad( single_scalar_type, arg_copy );
						}

						// Save address into temporary container to call lifetime.end after call.
						value_args_for_lifetime_end_call.push_back( arg_copy );
						arg_node->llvm_value= arg_copy;
					}
				}

				arg_node->location= Variable::Location::Pointer;
			}
			else U_ASSERT( false );

			// Register node for destruction in case of return in further args evaluation.
			arg_node->preserve_temporary= true;
			RegisterTemporaryVariable( function_context, arg_node );
			args_nodes[arg_number]= arg_node;
		}

		// Create second order lock nodes (for both value and reference args).
		const size_t reference_tag_count= param.type.ReferenceTagCount();
		for(size_t i= 0; i < reference_tag_count; ++i )
		{
			const SecondOrderInnerReferenceKind second_order_inner_reference_kind= param.type.GetSecondOrderInnerReferenceKind(i);
			if( second_order_inner_reference_kind != SecondOrderInnerReferenceKind::None )
			{
				const VariableMutPtr second_order_reference_node=
					Variable::Create(
					void_type_,
					second_order_inner_reference_kind == SecondOrderInnerReferenceKind::Imut ? ValueType::ReferenceImut : ValueType::ReferenceMut,
					Variable::Location::Pointer,
					"second order inner reference " + std::to_string(i) + " of arg " + std::to_string(arg_number) );
				second_order_reference_node->preserve_temporary= true;

				second_order_reference_nodes.resize( arg_count );
				second_order_reference_nodes[arg_number].resize( reference_tag_count );
				second_order_reference_nodes[arg_number][i]= second_order_reference_node;

				function_context.variables_state.AddNode( second_order_reference_node );
				for( const VariablePtr& accessible_non_inner_node :
					function_context.variables_state.GetAllAccessibleNonInnerNodes( args_nodes[arg_number]->inner_reference_nodes[i] ) )
				{
					// Usually we should have here only one inner reference node.
					// But it may be more if a member of a composite value is referenced.
					// In such case it's necessary to create links for all inner reference nodes, even if sometimes it can create false-positive reference protection errors.
					for( const VariablePtr& node : accessible_non_inner_node->inner_reference_nodes )
						function_context.variables_state.TryAddLink(
							node, second_order_reference_node, names_scope.GetErrors(), src_loc );
				}

				RegisterTemporaryVariable( function_context, second_order_reference_node );
			}
		}

		// Destroy unused temporary variables after each argument evaluation.
		DestroyUnusedTemporaryVariables( function_context, names_scope.GetErrors(), call_src_loc );
	} // for args

	if( !EnsureTypeComplete( function_type.return_type ) )
		REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), call_src_loc, function_type.return_type );

	const bool return_value_is_composite= function_type.ReturnsCompositeValue();
	const bool return_value_is_sret= FunctionTypeIsSRet( function_type );

	const VariableMutPtr result= Variable::Create(
		function_type.return_type,
		function_type.return_value_type,
		( function_type.return_value_type == ValueType::Value && !return_value_is_composite ) ? Variable::Location::LLVMRegister : Variable::Location::Pointer,
		"fn_result " + function_type.return_type.ToString() );

	function_context.variables_state.AddNode( result );

	if( return_value_is_composite )
	{
		if( !function_context.is_functionless_context )
		{
			result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( function_type.return_type.GetLLVMType() );
			CreateLifetimeStart( function_context, result->llvm_value );
		}

		if( return_value_is_sret )
		{
			llvm_args.insert( llvm_args.begin(), result->llvm_value );
			constant_llvm_args.insert( constant_llvm_args.begin(), nullptr );
		}
	}

	// Currently, we can not pass back referenes from constexpr functions evaluator.
	const auto function_as_real_function= function == nullptr ? nullptr : llvm::dyn_cast<llvm::Function>(function);
	if( func_is_constexpr &&
		function_as_real_function != nullptr &&
		constant_llvm_args.size() == function_as_real_function->arg_size() &&
		function_type.return_value_type == ValueType::Value && function_type.return_type.ReferenceTagCount() == 0u )
	{
		const Interpreter::ResultConstexpr evaluation_result=
			constexpr_function_evaluator_.EvaluateConstexpr( function_as_real_function, constant_llvm_args );

		for( const std::string& error_text : evaluation_result.errors )
		{
			CodeBuilderError error;
			error.code= CodeBuilderErrorCode::ConstexprFunctionEvaluationError;
			error.src_loc= call_src_loc;
			error.text= error_text;
			names_scope.GetErrors().push_back( std::move(error) );
		}
		if( evaluation_result.errors.empty() && evaluation_result.result_constant != nullptr )
		{
			if( function_type.return_value_type == ValueType::Value && function_type.return_type == void_type_ )
				result->llvm_value= result->constexpr_value= llvm::Constant::getNullValue( fundamental_llvm_types_.void_ );
			else if( return_value_is_composite )
			{
				if( return_value_is_sret )
				{
					if( !function_context.is_functionless_context )
						MoveConstantToMemory( result->type, result->llvm_value, evaluation_result.result_constant, function_context );
					result->constexpr_value= evaluation_result.result_constant;
				}
				else
				{
					if( !function_context.is_functionless_context )
						function_context.llvm_ir_builder.CreateStore( evaluation_result.result_constant, result->llvm_value );
					result->constexpr_value= WrapRawScalarConstant( evaluation_result.result_constant, function_type.return_type.GetLLVMType() );
				}
			}
			else
				result->llvm_value= result->constexpr_value= evaluation_result.result_constant;
		}
		else
			return ErrorValue(); // Should copy evaluation error later.
	}
	else if( function_context.is_functionless_context )
	{}
	else if( std::find( llvm_args.begin(), llvm_args.end(), nullptr ) == llvm_args.end() )
	{
		const auto really_function= llvm::dyn_cast<llvm::Function>(function);

		llvm::FunctionType* const llvm_function_type=
			really_function != nullptr ? really_function->getFunctionType() : GetLLVMFunctionType( function_type );

		llvm::CallInst* const call_instruction= function_context.llvm_ir_builder.CreateCall( llvm_function_type, function, llvm_args );
		call_instruction->setCallingConv( function_type.calling_convention );

		// In calls via function pointer set "nonnull" attribute for functions returning references.
		// It is needed only for pointer calls, since regular functions already have "nonnull" attribute on return value.
		if( really_function == nullptr && function_type.return_value_type != ValueType::Value )
			call_instruction->addRetAttr( llvm::Attribute::NonNull );

		if( function_type.return_value_type == ValueType::Value && function_type.return_type == void_type_ )
			result->llvm_value= llvm::UndefValue::get( fundamental_llvm_types_.void_ );
		else if( return_value_is_composite )
		{
			if( !return_value_is_sret )
				function_context.llvm_ir_builder.CreateStore( call_instruction, result->llvm_value );
		}
		else
			result->llvm_value= call_instruction;
	}
	else
	{
		// Fill dummy for error cases.
		if( return_value_is_composite )
		{}
		else if( function_type.return_value_type == ValueType::Value )
			result->llvm_value= llvm::UndefValue::get( function_type.return_type.GetLLVMType() );
		else
			result->llvm_value= llvm::UndefValue::get( function_type.return_type.GetLLVMType()->getPointerTo() );
	}

	// Call "lifetime.end" just right after call for value args.
	// It is fine because there is no way to return reference to value arg (reference protection does not allow this).
	// Do this only after call even for passed by single scalar values,
	// because we still need to have an alive address during furter args evaluation in order to call properly destructor in case of return or await.
	for( llvm::Value* const value_arg_var : value_args_for_lifetime_end_call )
		CreateLifetimeEnd( function_context, value_arg_var );

	// Prepare result references.
	if( function_type.return_value_type != ValueType::Value )
	{
		for( const FunctionType::ParamReference& arg_reference : function_type.return_references )
		{
			// Setup also second order references.
			// Do this specially since we have for now no special notation to specify returning of second order references.
			if( arg_reference.second != FunctionType::c_param_reference_number &&
				arg_reference.first < second_order_reference_nodes.size() )
			{
				const auto& arg_second_order_reference_nodes= second_order_reference_nodes[ arg_reference.first ];
				if( arg_reference.second < arg_second_order_reference_nodes.size() )
				{
					const VariablePtr& node= arg_second_order_reference_nodes[ arg_reference.second ];
					if( node != nullptr )
					{
						// Perform linking for all result inner references, we have no way to pick only some of them.
						for( const VariablePtr& inner_reference_node : result->inner_reference_nodes )
							function_context.variables_state.TryAddLink( node, inner_reference_node, names_scope.GetErrors(), call_src_loc );
					}
				}
			}

			if( arg_reference.first < args_nodes.size() )
			{
				const auto& arg_node= args_nodes[arg_reference.first];
				if( arg_node == nullptr )
					continue;

				if( arg_reference.second == FunctionType::c_param_reference_number )
					function_context.variables_state.TryAddLink( arg_node, result, names_scope.GetErrors(), call_src_loc );
				else if( arg_reference.second < arg_node->inner_reference_nodes.size() )
					function_context.variables_state.TryAddLink( arg_node->inner_reference_nodes[ arg_reference.second ], result, names_scope.GetErrors(), call_src_loc );
			}
		}
	}
	// Create links for result inner references.
	for( size_t tag_n= 0; tag_n < std::min( result->inner_reference_nodes.size(), function_type.return_inner_references.size() ); ++tag_n )
	{
		auto& dst_node= result->inner_reference_nodes[tag_n];
		for( const FunctionType::ParamReference& arg_reference : function_type.return_inner_references[tag_n] )
		{
			if( arg_reference.first < args_nodes.size() )
			{
				const auto& arg_node= args_nodes[arg_reference.first];
				if( arg_node == nullptr )
					continue;

				if( arg_reference.second == FunctionType::c_param_reference_number )
					function_context.variables_state.TryAddLink( arg_node, dst_node, names_scope.GetErrors(), call_src_loc );
				else if( arg_reference.second < arg_node->inner_reference_nodes.size() )
					function_context.variables_state.TryAddLink( arg_node->inner_reference_nodes[ arg_reference.second ], dst_node, names_scope.GetErrors(), call_src_loc );
			}
		}
	}

	// Setup references after call.
	for( const FunctionType::ReferencePollution& referene_pollution : function_type.references_pollution )
	{
		const size_t dst_arg= referene_pollution.dst.first;
		const size_t src_arg= referene_pollution.src.first;
		if( dst_arg >= function_type.params.size() || src_arg >= function_type.params.size() )
			continue;

		if( function_type.params[ dst_arg ].type.ReferenceTagCount() == 0 )
			continue;

		const VariablePtr& src_arg_node= args_nodes[ src_arg ];
		const VariablePtr& dst_arg_node= args_nodes[ dst_arg ];

		VariablePtr src_node;
		if( referene_pollution.src.second == FunctionType::c_param_reference_number )
			src_node= src_arg_node;
		else if( referene_pollution.src.second < src_arg_node->inner_reference_nodes.size() )
			src_node= src_arg_node->inner_reference_nodes[ referene_pollution.src.second ];

		VariablePtr dst_node;
		if( referene_pollution.dst.second == FunctionType::c_param_reference_number )
			dst_node= dst_arg_node;
		else if( referene_pollution.dst.second < dst_arg_node->inner_reference_nodes.size() )
			dst_node= dst_arg_node->inner_reference_nodes[ referene_pollution.dst.second ];

		if( src_node != nullptr && dst_node != nullptr )
			function_context.variables_state.TryAddLinkToAllAccessibleVariableNodesInnerReferences( src_node, dst_node, names_scope.GetErrors(), call_src_loc );
	}

	// Move arg nodes.
	// It is safe for variable-args, since the callee is responsible for its destruction.
	for( const VariablePtr& node : args_nodes )
	{
		if( node != nullptr )
			function_context.variables_state.MoveNode( node );
	}
	args_nodes.clear();

	for( const auto& nodes : second_order_reference_nodes )
	{
		for( const VariablePtr& node : nodes )
		{
			if( node != nullptr )
				function_context.variables_state.MoveNode( node );
		}
	}

	second_order_reference_nodes.clear();

	DestroyUnusedTemporaryVariables( function_context, names_scope.GetErrors(), call_src_loc );
	RegisterTemporaryVariable( function_context, result );

	return result;
}

VariablePtr CodeBuilder::BuildTempVariableConstruction(
	const Type& type,
	const llvm::ArrayRef<Synt::Expression> synt_args,
	const SrcLoc& src_loc,
	NamesScope& names_scope,
	FunctionContext& function_context )
{
	if( !EnsureTypeComplete( type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), src_loc, type );
		return nullptr;
	}
	else if( type.IsAbstract() )
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names_scope.GetErrors(), src_loc, type );

	const VariableMutPtr variable=
		Variable::Create(
			type,
			ValueType::Value,
			Variable::Location::Pointer,
			"temp " + type.ToString() );
	function_context.variables_state.AddNode( variable );

	if( !function_context.is_functionless_context )
	{
		variable->llvm_value= function_context.alloca_ir_builder.CreateAlloca( type.GetLLVMType() );
		CreateLifetimeStart( function_context, variable->llvm_value );
	}

	{
		const VariablePtr variable_for_initialization=
			Variable::Create(
				type,
				ValueType::ReferenceMut,
				Variable::Location::Pointer,
				variable->name,
				variable->llvm_value );
		function_context.variables_state.AddNode( variable_for_initialization );
		function_context.variables_state.AddLink( variable, variable_for_initialization );
		function_context.variables_state.TryAddInnerLinks( variable, variable_for_initialization, names_scope.GetErrors(), src_loc );

		variable->constexpr_value= ApplyConstructorInitializer( variable_for_initialization, synt_args, src_loc, names_scope, function_context );

		function_context.variables_state.RemoveNode( variable_for_initialization );
	}

	RegisterTemporaryVariable( function_context, variable );
	return variable;
}

VariablePtr CodeBuilder::ConvertVariable(
	const VariablePtr variable,
	const Type& dst_type,
	const FunctionVariable& conversion_constructor,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const SrcLoc& src_loc )
{
	if( !EnsureTypeComplete( dst_type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names_scope.GetErrors(), src_loc, dst_type );
		return nullptr;
	}

	conversion_constructor.referenced= true;

	const VariableMutPtr result=
		Variable::Create(
			dst_type,
			ValueType::Value,
			Variable::Location::Pointer,
			"temp " + dst_type.ToString() );

	function_context.variables_state.AddNode( result );

	if( !function_context.is_functionless_context )
	{
		result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( dst_type.GetLLVMType() );
		CreateLifetimeStart( function_context, result->llvm_value );
	}

	{
		// Create temp variables frame to prevent destruction of "src".
		const StackVariablesStorage temp_variables_storage( function_context );

		const VariablePtr result_for_initialization=
			Variable::Create(
				dst_type,
				ValueType::ReferenceMut,
				Variable::Location::Pointer,
				result->name,
				result->llvm_value );
		function_context.variables_state.AddNode( result_for_initialization );
		function_context.variables_state.AddLink( result, result_for_initialization );
		function_context.variables_state.TryAddInnerLinks( result, result_for_initialization, names_scope.GetErrors(), src_loc );

		DoCallFunction(
			EnsureLLVMFunctionCreated( conversion_constructor ),
			conversion_constructor.type,
			src_loc,
			{ result_for_initialization, variable },
			{},
			false,
			names_scope,
			function_context,
			false );

		CallDestructors( temp_variables_storage, names_scope, function_context, src_loc );

		function_context.variables_state.RemoveNode( result_for_initialization );
	}

	RegisterTemporaryVariable( function_context, result );
	return result;
}

bool CodeBuilder::EvaluateBoolConstantExpression( NamesScope& names_scope, FunctionContext& function_context, const Synt::Expression& expression )
{
	const VariablePtr v= BuildExpressionCodeEnsureVariable( expression, names_scope, function_context );
	if( v->type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch, names_scope.GetErrors(), Synt::GetSrcLoc( expression ), bool_type_, v->type );
		return false;
	}
	if( v->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names_scope.GetErrors(), Synt::GetSrcLoc( expression ) );
		return false;
	}

	// Do not need to destroy variables here, because this function is normally called only in constexr context.

	return v->constexpr_value->isAllOnesValue();
}

FunctionType::Param CodeBuilder::PreEvaluateArg( const Synt::Expression& expression, NamesScope& names, FunctionContext& function_context )
{
	if( function_context.args_preevaluation_cache.count(&expression) == 0 )
	{
		const VariablePtr v= BuildExpressionCodeEnsureVariable( expression, names, function_context );
		function_context.args_preevaluation_cache.emplace( &expression, GetArgExtendedType(*v) );
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), Synt::GetSrcLoc( expression ) );

	}
	return function_context.args_preevaluation_cache[&expression];
}

FunctionType::Param CodeBuilder::GetArgExtendedType( const Variable& variable )
{
	FunctionType::Param arg_type_extended;
	arg_type_extended.type= variable.type;
	arg_type_extended.value_type= variable.value_type;
	return arg_type_extended;
}

} // namespace U
