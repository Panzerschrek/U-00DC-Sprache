#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/MD5.h>
#include <llvm/Support/ConvertUTF.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

#define CHECK_RETURN_ERROR_VALUE(value) if( value.GetErrorValue() != nullptr ) { return value; }

namespace U
{

Value CodeBuilder::BuildExpressionCodeAndDestroyTemporaries(
	const Synt::Expression& expression,
	NamesScope& names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expression.
	const StackVariablesStorage temp_variables_storage( function_context );
	const Value result= BuildExpressionCode( expression, names, function_context );
	CallDestructors( temp_variables_storage, names, function_context, Synt::GetExpressionSrcLoc( expression ) );

	return result;
}

Variable CodeBuilder::BuildExpressionCodeEnsureVariable(
	const Synt::Expression& expression,
	NamesScope& names,
	FunctionContext& function_context )
{
	Value result= BuildExpressionCode( expression, names, function_context );

	Variable* const result_variable= result.GetVariable();
	if( result_variable == nullptr )
	{
		if( result.GetErrorValue() == nullptr )
			REPORT_ERROR( ExpectedVariable, names.GetErrors(), Synt::GetExpressionSrcLoc( expression ), result.GetKindName() );

		Variable dummy_result;
		dummy_result.type= invalid_type_;
		dummy_result.llvm_value= llvm::UndefValue::get( invalid_type_.GetLLVMType()->getPointerTo() );
		return dummy_result;
	}
	return std::move( *result_variable );
}

Value CodeBuilder::BuildExpressionCode(
	const Synt::Expression& expression,
	NamesScope& names,
	FunctionContext& function_context )
{
	return
		std::visit(
			[&]( const auto& t )
			{
				return BuildExpressionCodeImpl( names, function_context, t );
			},
			expression );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope&,
	FunctionContext&,
	const Synt::EmptyVariant& )
{
	U_ASSERT(false);
	return Value();
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::CallOperator& call_operator )
{
	const Value function_value= BuildExpressionCode( *call_operator.expression_, names, function_context );
	CHECK_RETURN_ERROR_VALUE(function_value);

	return CallFunction( function_value, call_operator.arguments_, call_operator.src_loc_, names, function_context );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::IndexationOperator& indexation_operator )
{
	const Variable variable= BuildExpressionCodeEnsureVariable( *indexation_operator.expression_, names, function_context );

	if( variable.type.GetClassType() != nullptr ) // If this is class - try call overloaded [] operator.
	{
		if( auto res=
				TryCallOverloadedPostfixOperator(
					variable,
					llvm::ArrayRef<Synt::Expression>(*indexation_operator.index_),
					OverloadedOperator::Indexing,
					indexation_operator.src_loc_,
					names,
					function_context ) )
			return std::move(*res);

		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), indexation_operator.src_loc_, variable.type );
		return ErrorValue();
	}

	// Lock variable. We must prevent modification of this variable in index calcualtion.
	// You SHOULD register variable in case if you call "TakeNode".
	ReferencesGraphNodeHolder variable_lock(
		function_context,
		variable.value_type == ValueType::ReferenceMut ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut,
		"[]" );
	if( variable.node != nullptr && !function_context.variables_state.TryAddLink( variable.node, variable_lock.Node() ) )
		REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), indexation_operator.src_loc_, variable.node->name );

	const Variable index= BuildExpressionCodeEnsureVariable( *indexation_operator.index_, names, function_context );

	if( const ArrayType* const array_type= variable.type.GetArrayType() )
	{
		const FundamentalType* const index_fundamental_type= index.type.GetFundamentalType();
		if( !( index_fundamental_type != nullptr && (
			( index.constexpr_value != nullptr && IsInteger( index_fundamental_type->fundamental_type ) ) ||
			( index.constexpr_value == nullptr && IsUnsignedInteger( index_fundamental_type->fundamental_type ) ) ) ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), indexation_operator.src_loc_, "any unsigned integer", index.type );
			return ErrorValue();
		}

		if( variable.location != Variable::Location::Pointer )
		{
			// TODO - Strange variable location.
			return ErrorValue();
		}

		// If index is constant and not undefined statically check index.
		if( index.constexpr_value != nullptr )
		{
			const llvm::APInt index_value= index.constexpr_value->getUniqueInteger();
			if( IsSignedInteger(index_fundamental_type->fundamental_type) )
			{
				if( index_value.getLimitedValue() >= array_type->size || index_value.isNegative() )
					REPORT_ERROR( ArrayIndexOutOfBounds, names.GetErrors(), indexation_operator.src_loc_, index_value.getSExtValue(), array_type->size );
			}
			else
			{
				if( index_value.getLimitedValue() >= array_type->size )
					REPORT_ERROR( ArrayIndexOutOfBounds, names.GetErrors(), indexation_operator.src_loc_, index_value.getLimitedValue(), array_type->size );
			}
		}

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= GetZeroGEPIndex();
		index_list[1]= CreateMoveToLLVMRegisterInstruction( index, function_context );

		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), indexation_operator.src_loc_ ); // Destroy temporaries of index expression.

		Variable result;
		result.location= Variable::Location::Pointer;
		result.value_type= variable.value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut;
		result.node= variable_lock.TakeNode();
		result.type= array_type->type;

		if( variable.constexpr_value != nullptr && index.constexpr_value != nullptr )
			result.constexpr_value= variable.constexpr_value->getAggregateElement( index.constexpr_value );

		// If index is not constant - check bounds.
		if( index.constexpr_value == nullptr )
		{
			llvm::Value* index_value= index_list[1];
			const uint64_t index_size= index_fundamental_type->GetSize();
			const uint64_t size_type_size= size_type_.GetFundamentalType()->GetSize();
			if( index_size > size_type_size )
				index_value= function_context.llvm_ir_builder.CreateTrunc( index_value, size_type_.GetLLVMType() );
			else if( index_size < size_type_size )
				index_value= function_context.llvm_ir_builder.CreateZExt( index_value, size_type_.GetLLVMType() );

			llvm::Value* const condition=
				function_context.llvm_ir_builder.CreateICmpUGE( // if( index >= array_size ) {halt;}
					index_value,
					llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), array_type->size ) ) );

			llvm::BasicBlock* const halt_block= llvm::BasicBlock::Create( llvm_context_ );
			llvm::BasicBlock* const block_after_if= llvm::BasicBlock::Create( llvm_context_ );
			function_context.llvm_ir_builder.CreateCondBr( condition, halt_block, block_after_if );

			function_context.function->getBasicBlockList().push_back( halt_block );
			function_context.llvm_ir_builder.SetInsertPoint( halt_block );
			function_context.llvm_ir_builder.CreateCall( halt_func_ );
			function_context.llvm_ir_builder.CreateUnreachable(); // terminate block.

			function_context.function->getBasicBlockList().push_back( block_after_if );
			function_context.llvm_ir_builder.SetInsertPoint( block_after_if );
		}

		result.llvm_value= function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, index_list );

		RegisterTemporaryVariable( function_context, result );
		return Value( std::move(result), indexation_operator.src_loc_ );
	}
	else if( const TupleType* const tuple_type= variable.type.GetTupleType() )
	{
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), indexation_operator.src_loc_ ); // Destroy temporaries of index expression.

		const FundamentalType* const index_fundamental_type= index.type.GetFundamentalType();
		if( index_fundamental_type == nullptr || !IsInteger( index_fundamental_type->fundamental_type ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), indexation_operator.src_loc_, "any integer", index.type );
			return ErrorValue();
		}

		if( variable.location != Variable::Location::Pointer )
		{
			// TODO - Strange variable location.
			return ErrorValue();
		}

		// For tuple indexing only constexpr indeces are valid.
		if( index.constexpr_value == nullptr )
		{
			REPORT_ERROR( ExpectedConstantExpression, names.GetErrors(), indexation_operator.src_loc_ );
			return ErrorValue();
		}
		const llvm::APInt index_value_raw= index.constexpr_value->getUniqueInteger();
		const uint64_t index_value= index_value_raw.getLimitedValue();
		if( IsSignedInteger(index_fundamental_type->fundamental_type) )
		{
			if( index_value >= static_cast<uint64_t>(tuple_type->elements.size()) || index_value_raw.isNegative() )
			{
				REPORT_ERROR( TupleIndexOutOfBounds, names.GetErrors(), indexation_operator.src_loc_, index_value_raw.getSExtValue(), tuple_type->elements.size() );
				return ErrorValue();
			}
		}
		else
		{
			if( index_value >= static_cast<uint64_t>(tuple_type->elements.size()) )
			{
				REPORT_ERROR( TupleIndexOutOfBounds, names.GetErrors(), indexation_operator.src_loc_, index_value, tuple_type->elements.size() );
				return ErrorValue();
			}
		}

		Variable result;
		result.location= Variable::Location::Pointer;
		result.value_type= variable.value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut;
		result.node= variable_lock.TakeNode();
		result.type= tuple_type->elements[static_cast<size_t>(index_value)];
		result.llvm_value= function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex(index_value) } );

		if( variable.constexpr_value != nullptr )
			result.constexpr_value= variable.constexpr_value->getAggregateElement( static_cast<unsigned int>(index_value) );

		RegisterTemporaryVariable( function_context, result );
		return Value( std::move(result), indexation_operator.src_loc_ );
	}
	else
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), indexation_operator.src_loc_, variable.type );
		return ErrorValue();
	}
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::MemberAccessOperator& member_access_operator )
{
	const Variable variable= BuildExpressionCodeEnsureVariable( *member_access_operator.expression_, names, function_context );

	Class* const class_type= variable.type.GetClassType();
	if( class_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), member_access_operator.src_loc_, variable.type );
		return ErrorValue();
	}

	if( !EnsureTypeComplete( variable.type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), member_access_operator.src_loc_, variable.type );
		return ErrorValue();
	}

	const Value* const class_member= class_type->members->GetThisScopeValue( member_access_operator.member_name_ );
	if( class_member == nullptr )
	{
		REPORT_ERROR( NameNotFound, names.GetErrors(), member_access_operator.src_loc_, member_access_operator.member_name_ );
		return ErrorValue();
	}

	if( !function_context.is_in_unsafe_block &&
		( member_access_operator.member_name_ == Keywords::constructor_ || member_access_operator.member_name_ == Keywords::destructor_ ) )
		REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names.GetErrors(), member_access_operator.src_loc_,  member_access_operator.member_name_ );

	if( names.GetAccessFor( variable.type.GetClassType() ) < class_type->GetMemberVisibility( member_access_operator.member_name_ ) )
		REPORT_ERROR( AccessingNonpublicClassMember, names.GetErrors(), member_access_operator.src_loc_, member_access_operator.member_name_, class_type->members->GetThisNamespaceName() );

	if( const OverloadedFunctionsSet* functions_set= class_member->GetFunctionsSet() )
	{
		if( member_access_operator.template_parameters != std::nullopt )
		{
			if( functions_set->template_functions.empty() )
				REPORT_ERROR( ValueIsNotTemplate, names.GetErrors(), member_access_operator.src_loc_ );
			else
			{
				const Value* const inserted_value=
					ParametrizeFunctionTemplate(
						member_access_operator.src_loc_,
						*functions_set,
						*member_access_operator.template_parameters,
						names,
						function_context );
				if( inserted_value == nullptr )
					return ErrorValue();

				functions_set= inserted_value->GetFunctionsSet();
			}
		}
		ThisOverloadedMethodsSet this_overloaded_methods_set;
		this_overloaded_methods_set.this_= variable;
		this_overloaded_methods_set.GetOverloadedFunctionsSet()= *functions_set;
		return std::move(this_overloaded_methods_set);
	}

	if( member_access_operator.template_parameters != std::nullopt )
		REPORT_ERROR( ValueIsNotTemplate, names.GetErrors(), member_access_operator.src_loc_ );

	const ClassField* const field= class_member->GetClassField();
	if( field == nullptr )
	{
		REPORT_ERROR( NotImplemented, names.GetErrors(), member_access_operator.src_loc_, "class members, except fields or methods" );
		return ErrorValue();
	}

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= GetZeroGEPIndex();

	llvm::Value* actual_field_class_ptr= nullptr;
	if( field->class_ == variable.type.GetClassType() )
		actual_field_class_ptr= variable.llvm_value;
	else
	{
		// For parent field we needs make several GEP isntructions.
		ClassPtr actual_field_class= variable.type.GetClassType();
		actual_field_class_ptr= variable.llvm_value;
		while( actual_field_class != field->class_ )
		{
			index_list[1]= GetFieldGEPIndex( 0u /* base class is allways first field */ );
			actual_field_class_ptr= function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );

			actual_field_class= actual_field_class->base_class;
			U_ASSERT(actual_field_class != nullptr );
		}
	}

	index_list[1]= GetFieldGEPIndex( field->index );
	llvm::Value* const gep_result= function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );

	Variable result;
	result.location= Variable::Location::Pointer;
	result.type= field->type;

	if( field->is_reference )
	{
		result.value_type= field->is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;

		if( variable.constexpr_value != nullptr )
		{
			if( EnsureTypeComplete( field->type ) )
			{
				// Constexpr references field should be "GlobalVariable"
				const auto var= llvm::dyn_cast<llvm::GlobalVariable>( variable.constexpr_value->getAggregateElement( static_cast<unsigned int>( field->index ) ));
				result.llvm_value= var;
				result.constexpr_value= var->getInitializer();
			}
			else
				return ErrorValue(); // Actual error will be reported in another place.
		}
		else
		{
			// Reference is never null, so, mark result of reference field load with "nonnull" metadata.
			const auto load_res= function_context.llvm_ir_builder.CreateLoad( gep_result );
			load_res->setMetadata( llvm::LLVMContext::MD_nonnull, llvm::MDNode::get( llvm_context_, llvm::None ) );
			result.llvm_value= load_res;
		}

		if( variable.node != nullptr )
		{
			const auto inner_nodes= function_context.variables_state.GetAccessibleVariableNodesInnerReferences( variable.node );
			if( inner_nodes.size() == 1 )
			{
				// For cases with single inner node just return it. This hels preventing some cases of false "ReferenceProtectionError".
				result.node= *inner_nodes.begin();
			}
			else
			{
				result.node= function_context.variables_state.AddNode( field->is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut, variable.node->name + "." + member_access_operator.member_name_ );
				RegisterTemporaryVariable( function_context, result );
				for( const ReferencesGraphNodePtr& inner_reference : inner_nodes )
				{
					if( !function_context.variables_state.TryAddLink( inner_reference, result.node ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), member_access_operator.src_loc_, inner_reference->name );
				}
			}
		}
	}
	else
	{
		result.value_type= ( variable.value_type == ValueType::ReferenceMut && field->is_mutable ) ? ValueType::ReferenceMut : ValueType::ReferenceImut;

		result.llvm_value= gep_result;
		if( variable.constexpr_value != nullptr )
			result.constexpr_value= variable.constexpr_value->getAggregateElement( static_cast<unsigned int>( field->index ) );

		result.node= variable.node;
	}

	return Value( std::move(result), member_access_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::UnaryMinus& unary_minus )
{
	const Variable variable= BuildExpressionCodeEnsureVariable( *unary_minus.expression_, names, function_context );

	if( auto res= TryCallOverloadedUnaryOperator( variable, OverloadedOperator::Sub, unary_minus.src_loc_, names, function_context ) )
		return std::move(*res);

	const FundamentalType* const fundamental_type= variable.type.GetFundamentalType();
	if( fundamental_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), unary_minus.src_loc_, variable.type );
		return ErrorValue();
	}

	const bool is_float= IsFloatingPoint( fundamental_type->fundamental_type );
	if( !( IsInteger( fundamental_type->fundamental_type ) || is_float ) )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), unary_minus.src_loc_, variable.type );
		return ErrorValue();
	}
	// TODO - maybe not support unary minus for 8 and 16 bot integer types?

	Variable result;
	result.type= variable.type;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;

	llvm::Value* const value_for_neg= CreateMoveToLLVMRegisterInstruction( variable, function_context );
	if( is_float )
		result.llvm_value= function_context.llvm_ir_builder.CreateFNeg( value_for_neg );
	else
		result.llvm_value= function_context.llvm_ir_builder.CreateNeg( value_for_neg );

	result.constexpr_value= llvm::dyn_cast<llvm::Constant>(result.llvm_value);

	result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, OverloadedOperatorToString(OverloadedOperator::Sub) );
	RegisterTemporaryVariable( function_context, result );
	return Value( std::move(result), unary_minus.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::UnaryPlus& unary_plus )
{
	// TODO - maybe check type of expression here?
	return BuildExpressionCode( *unary_plus.expression_, names, function_context );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::LogicalNot& logical_not )
{
	const Variable variable= BuildExpressionCodeEnsureVariable( *logical_not.expression_, names, function_context );

	if( auto res= TryCallOverloadedUnaryOperator( variable, OverloadedOperator::LogicalNot, logical_not.src_loc_, names, function_context ) )
		return std::move(*res);

	if( variable.type != bool_type_ )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), logical_not.src_loc_, variable.type );
		return ErrorValue();
	}

	Variable result;
	result.type= variable.type;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;
	result.llvm_value= function_context.llvm_ir_builder.CreateNot( CreateMoveToLLVMRegisterInstruction( variable, function_context ) );
	result.constexpr_value= llvm::dyn_cast<llvm::Constant>(result.llvm_value);
	result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, OverloadedOperatorToString(OverloadedOperator::LogicalNot) );
	RegisterTemporaryVariable( function_context, result );
	return Value( std::move(result), logical_not.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::BitwiseNot& bitwise_not )
{
	const Variable variable= BuildExpressionCodeEnsureVariable( *bitwise_not.expression_, names, function_context );

	if( auto res= TryCallOverloadedUnaryOperator( variable, OverloadedOperator::BitwiseNot, bitwise_not.src_loc_, names, function_context ) )
		return std::move(*res);

	const FundamentalType* const fundamental_type= variable.type.GetFundamentalType();
	if( fundamental_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), bitwise_not.src_loc_, variable.type );
		return ErrorValue();
	}
	if( !IsInteger( fundamental_type->fundamental_type ) )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), bitwise_not.src_loc_, variable.type  );
		return ErrorValue();
	}

	Variable result;
	result.type= variable.type;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;
	result.llvm_value= function_context.llvm_ir_builder.CreateNot( CreateMoveToLLVMRegisterInstruction( variable, function_context ) );
	result.constexpr_value= llvm::dyn_cast<llvm::Constant>(result.llvm_value);
	result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, OverloadedOperatorToString(OverloadedOperator::BitwiseNot) );
	RegisterTemporaryVariable( function_context, result );
	return Value( std::move(result), bitwise_not.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::BinaryOperator& binary_operator	)
{
	if( binary_operator.operator_type_ == BinaryOperatorType::LazyLogicalAnd ||
		binary_operator.operator_type_ == BinaryOperatorType::LazyLogicalOr )
	{
		return
			BuildLazyBinaryOperator(
				*binary_operator.left_,
				*binary_operator.right_,
				binary_operator,
				binary_operator.src_loc_,
				names,
				function_context );
	}

	std::optional<Value> overloaded_operator_call_try=
		TryCallOverloadedBinaryOperator(
			GetOverloadedOperatorForBinaryOperator( binary_operator.operator_type_ ),
			*binary_operator.left_, *binary_operator.right_,
			false,
			binary_operator.src_loc_,
			names,
			function_context );
	if( overloaded_operator_call_try != std::nullopt )
		return std::move(*overloaded_operator_call_try);

	Variable l_var=
		BuildExpressionCodeEnsureVariable(
			*binary_operator.left_,
			names,
			function_context );

	if( l_var.type.GetFundamentalType() != nullptr ||
		l_var.type.GetEnumType() != nullptr ||
		l_var.type.GetRawPointerType() != nullptr ||
		l_var.type.GetFunctionPointerType() != nullptr )
	{
		// Save l_var in register, because build-in binary operators require value-parameters.
		if( l_var.location == Variable::Location::Pointer )
		{
			l_var.llvm_value= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			l_var.location= Variable::Location::LLVMRegister;
		}
		l_var.value_type= ValueType::Value;
	}
	DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), binary_operator.src_loc_ );

	const Variable r_var=
		BuildExpressionCodeEnsureVariable(
			*binary_operator.right_,
			names,
			function_context );

	return BuildBinaryOperator( l_var, r_var, binary_operator.operator_type_, binary_operator.src_loc_, names, function_context );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::ComplexName& named_operand )
{
	if( std::get_if<std::string>( &named_operand.start_value ) != nullptr && named_operand.tail == nullptr )
	{
		const std::string& start_name= std::get<std::string>( named_operand.start_value );
		if( start_name == Keywords::this_ )
		{
			if( function_context.this_ == nullptr || function_context.whole_this_is_unavailable )
			{
				REPORT_ERROR( ThisUnavailable, names.GetErrors(), named_operand.src_loc_ );
				return ErrorValue();
			}
			return Value( *function_context.this_, named_operand.src_loc_ );
		}
		else if( start_name == Keywords::base_ )
		{
			if( function_context.this_ == nullptr )
			{
				REPORT_ERROR( BaseUnavailable, names.GetErrors(), named_operand.src_loc_ );
				return ErrorValue();
			}
			const Class& class_= *function_context.this_->type.GetClassType();
			if( class_.base_class == nullptr )
			{
				REPORT_ERROR( BaseUnavailable, names.GetErrors(), named_operand.src_loc_ );
				return ErrorValue();
			}
			if( function_context.whole_this_is_unavailable && ( !function_context.base_initialized || class_.base_class->kind == Class::Kind::Abstract ) )
			{
				REPORT_ERROR( FieldIsNotInitializedYet, names.GetErrors(), named_operand.src_loc_, Keyword( Keywords::base_ ) );
				return ErrorValue();
			}

			Variable base= *function_context.this_;
			base.type= class_.base_class;
			base.llvm_value= CreateReferenceCast( function_context.this_->llvm_value, function_context.this_->type, base.type, function_context );
			return Value( std::move(base), named_operand.src_loc_ );
		}
	}

	const Value value_entry= ResolveValue( names, function_context, named_operand );

	if( const ClassField* const field= value_entry.GetClassField() )
	{
		if( function_context.this_ == nullptr )
		{
			REPORT_ERROR( ClassFieldAccessInStaticMethod, names.GetErrors(), named_operand.src_loc_, field->syntax_element->name );
			return ErrorValue();
		}

		const ClassPtr class_= field->class_;
		U_ASSERT( class_ != nullptr && "Class is dead? WTF?" );

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= GetZeroGEPIndex();


		llvm::Value* actual_field_class_ptr= nullptr;
		if( class_ == function_context.this_->type.GetClassType() )
			actual_field_class_ptr= function_context.this_->llvm_value;
		else
		{
			// For parent field we needs make several GEP isntructions.
			ClassPtr actual_field_class= function_context.this_->type.GetClassType();
			actual_field_class_ptr= function_context.this_->llvm_value;
			while( actual_field_class != class_ )
			{
				if( actual_field_class->base_class == nullptr )
				{
					REPORT_ERROR( AccessOfNonThisClassField, names.GetErrors(), named_operand.src_loc_, field->syntax_element->name );
					return ErrorValue();
				}

				index_list[1]= GetFieldGEPIndex( 0u /* base class is allways first field */ );
				actual_field_class_ptr= function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );
				actual_field_class= actual_field_class->base_class;
			}
		}

		if( function_context.whole_this_is_unavailable &&
			function_context.uninitialized_this_fields.find( field->syntax_element->name ) != function_context.uninitialized_this_fields.end() )
		{
			REPORT_ERROR( FieldIsNotInitializedYet, names.GetErrors(), named_operand.src_loc_, field->syntax_element->name );
			return ErrorValue();
		}
		if( function_context.whole_this_is_unavailable &&
			class_ != function_context.this_->type.GetClassType() &&
			!function_context.base_initialized )
		{
			REPORT_ERROR( FieldIsNotInitializedYet, names.GetErrors(), named_operand.src_loc_, Keyword( Keywords::base_ ) );
			return ErrorValue();
		}

		Variable field_variable;
		field_variable.type= field->type;
		field_variable.location= Variable::Location::Pointer;
		field_variable.value_type= ( function_context.this_->value_type == ValueType::ReferenceMut && field->is_mutable ) ? ValueType::ReferenceMut : ValueType::ReferenceImut;
		field_variable.node= function_context.this_->node;

		index_list[1]= GetFieldGEPIndex( field->index );
		field_variable.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );

		if( field->is_reference )
		{
			field_variable.value_type= field->is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;
			field_variable.llvm_value= function_context.llvm_ir_builder.CreateLoad( field_variable.llvm_value );

			if( function_context.this_->node != nullptr )
			{
				const auto inner_nodes= function_context.variables_state.GetAccessibleVariableNodesInnerReferences( function_context.this_->node );
				if( inner_nodes.size() == 1 )
				{
					// For cases with single inner node just return it. This hels preventing some cases of false "ReferenceProtectionError".
					field_variable.node= *inner_nodes.begin();
				}
				else
				{
					field_variable.node= function_context.variables_state.AddNode( field->is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut, "this." + field->syntax_element->name );
					RegisterTemporaryVariable( function_context, field_variable );
					for( const ReferencesGraphNodePtr& node : inner_nodes )
					{
						if( !function_context.variables_state.TryAddLink( node, field_variable.node ) )
							REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), named_operand.src_loc_, node->name );
					}
				}
			}
		}

		return Value( std::move(field_variable), named_operand.src_loc_ );
	}
	else if( const OverloadedFunctionsSet* const overloaded_functions_set= value_entry.GetFunctionsSet() )
	{
		if( function_context.this_ != nullptr )
		{
			// Trying add "this" to functions set, but only if whole "this" is available.
			if( ( function_context.this_->type.GetClassType() == overloaded_functions_set->base_class ||
				  function_context.this_->type.GetClassType()->HaveAncestor( overloaded_functions_set->base_class ) ) &&
				!function_context.whole_this_is_unavailable )
			{
				ThisOverloadedMethodsSet this_overloaded_methods_set;
				this_overloaded_methods_set.this_= *function_context.this_;
				this_overloaded_methods_set.GetOverloadedFunctionsSet()= *overloaded_functions_set;
				return std::move(this_overloaded_methods_set);
			}
		}
	}
	else if( const Variable* const variable= value_entry.GetVariable() )
	{
		if( variable->node != nullptr && function_context.variables_state.NodeMoved( variable->node ) )
			REPORT_ERROR( AccessingMovedVariable, names.GetErrors(), named_operand.src_loc_, variable->node->name );

		// Forbid mutable global variables access outside unsafe block.
		// Detect global variable by checking dynamic type of variable's LLVM value.
		// TODO - what if variable is constant GEP result with global variable base?
		if( variable->value_type == ValueType::ReferenceMut &&
			llvm::dyn_cast<llvm::GlobalVariable>( variable->llvm_value ) != nullptr &&
			!function_context.is_in_unsafe_block )
			REPORT_ERROR( GlobalMutableVariableAccessOutsideUnsafeBlock, names.GetErrors(), named_operand.src_loc_ );
	}

	return value_entry;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::TernaryOperator& ternary_operator )
{
	const Variable condition= BuildExpressionCodeEnsureVariable( *ternary_operator.condition, names, function_context );
	if( condition.type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), ternary_operator.src_loc_, bool_type_, condition.type );
		return ErrorValue();
	}

	const auto condition_in_register= CreateMoveToLLVMRegisterInstruction( condition, function_context );

	// Preevaluate branches for selection of type and value type for operator result.
	Type branches_types[2u];
	ValueType branches_value_types[2u];
	{
		for( size_t i= 0u; i < 2u; ++i )
		{
			const auto state= SaveInstructionsState( function_context );
			{
				const StackVariablesStorage dummy_stack_variables_storage( function_context );
				const Variable var= BuildExpressionCodeEnsureVariable( i == 0u ? *ternary_operator.true_branch : *ternary_operator.false_branch, names, function_context );
				branches_types[i]= var.type;
				branches_value_types[i]= var.value_type;
				DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), ternary_operator.src_loc_ );
			}
			RestoreInstructionsState( function_context, state );
		}
	}

	if( branches_types[0] != branches_types[1] )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), ternary_operator.src_loc_, branches_types[0], branches_types[1] );
		return ErrorValue();
	}

	Variable result;
	result.type= branches_types[0];
	result.location= Variable::Location::Pointer;
	ReferencesGraphNode::Kind node_kind;
	if( branches_value_types[0] == ValueType::Value || branches_value_types[1] == ValueType::Value )
	{
		result.value_type= ValueType::Value;
		node_kind= ReferencesGraphNode::Kind::Variable;
		if( !EnsureTypeComplete( result.type ) )
		{
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), ternary_operator.src_loc_, result.type );
			return ErrorValue();
		}
		result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( result.type.GetLLVMType() );
		result.llvm_value->setName( "select_result" );

		CreateLifetimeStart( result, function_context );
	}
	else if( branches_value_types[0] == ValueType::ReferenceImut || branches_value_types[1] == ValueType::ReferenceImut )
	{
		result.value_type= ValueType::ReferenceImut;
		node_kind= ReferencesGraphNode::Kind::ReferenceImut;
	}
	else
	{
		result.value_type= ValueType::ReferenceMut;
		node_kind= ReferencesGraphNode::Kind::ReferenceMut;
	}
	// Do not forget to remove node in case of error-return!!!
	result.node= function_context.variables_state.AddNode( node_kind, Keyword( Keywords::select_ ) );

	llvm::BasicBlock* const result_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const branches_basic_blocks[2]{ llvm::BasicBlock::Create( llvm_context_ ), llvm::BasicBlock::Create( llvm_context_ ) };

	function_context.llvm_ir_builder.CreateCondBr( condition_in_register, branches_basic_blocks[0], branches_basic_blocks[1] );

	llvm::Value* branches_reference_values[2] { nullptr, nullptr };
	llvm::Constant* branches_constexpr_values[2] { nullptr, nullptr };
	llvm::BasicBlock* branches_end_basic_blocks[2]{ nullptr, nullptr };
	ReferencesGraph variables_state_before= function_context.variables_state;
	std::vector<ReferencesGraph> branches_variables_state(2u);
	for( size_t i= 0u; i < 2u; ++i )
	{
		function_context.variables_state= variables_state_before;
		{
			const StackVariablesStorage branch_temp_variables_storage( function_context );

			function_context.function->getBasicBlockList().push_back( branches_basic_blocks[i] );
			function_context.llvm_ir_builder.SetInsertPoint( branches_basic_blocks[i] );
			const Variable branch_result= BuildExpressionCodeEnsureVariable( i == 0u ? *ternary_operator.true_branch : *ternary_operator.false_branch, names, function_context );

			branches_constexpr_values[i]= branch_result.constexpr_value;
			if( result.value_type == ValueType::Value )
			{
				// Move or create copy.
				if( result.type == void_type_ ){}
				else if(
					result.type.GetFundamentalType() != nullptr ||
					result.type.GetEnumType() != nullptr ||
					result.type.GetRawPointerType() != nullptr ||
					result.type.GetFunctionPointerType() != nullptr )
					function_context.llvm_ir_builder.CreateStore( CreateMoveToLLVMRegisterInstruction( branch_result, function_context ), result.llvm_value );
				else if(
					result.type.GetClassType() != nullptr ||
					result.type.GetTupleType() != nullptr ||
					result.type.GetArrayType() != nullptr )
				{
					SetupReferencesInCopyOrMove( function_context, result, branch_result, names.GetErrors(), ternary_operator.src_loc_ );

					if( branch_result.value_type == ValueType::Value )
					{
						// Move.
						if( branch_result.node != nullptr )
							function_context.variables_state.MoveNode( branch_result.node );
						CopyBytes( branch_result.llvm_value, result.llvm_value, result.type, function_context );
					}
					else
					{
						// Copy.
						if( !result.type.IsCopyConstructible() )
						{
							REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), ternary_operator.src_loc_, result.type );
							return ErrorValue();
						}

						BuildCopyConstructorPart( result.llvm_value, branch_result.llvm_value, result.type, function_context );
					}
				}
				else
				{
					REPORT_ERROR( NotImplemented, names.GetErrors(), ternary_operator.src_loc_, "move such kind of types" );
					return ErrorValue();
				}
			}
			else
			{
				branches_reference_values[i]= branch_result.llvm_value;

				if( branch_result.node != nullptr && !function_context.variables_state.TryAddLink( branch_result.node, result.node ) )
					REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), ternary_operator.src_loc_, branch_result.node->name );
			}

			CallDestructors( branch_temp_variables_storage, names, function_context, ternary_operator.src_loc_ );
			function_context.llvm_ir_builder.CreateBr( result_block );
		}
		branches_end_basic_blocks[i]= function_context.llvm_ir_builder.GetInsertBlock();
		branches_variables_state[i]= function_context.variables_state;
	}
	function_context.function->getBasicBlockList().push_back( result_block );
	function_context.llvm_ir_builder.SetInsertPoint( result_block );

	function_context.variables_state= MergeVariablesStateAfterIf( branches_variables_state, names.GetErrors(), ternary_operator.src_loc_ );

	if( result.value_type != ValueType::Value )
	{
		llvm::PHINode* const phi= function_context.llvm_ir_builder.CreatePHI( result.type.GetLLVMType()->getPointerTo(), 2u );
		phi->addIncoming( branches_reference_values[0], branches_end_basic_blocks[0] );
		phi->addIncoming( branches_reference_values[1], branches_end_basic_blocks[1] );
		result.llvm_value= phi;
	}

	if( condition.constexpr_value != nullptr )
		result.constexpr_value= condition.constexpr_value->getUniqueInteger().getLimitedValue() != 0u ? branches_constexpr_values[0] : branches_constexpr_values[1];

	RegisterTemporaryVariable( function_context, result );
	return Value( result, ternary_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator )
{
	const Variable v= BuildExpressionCodeEnsureVariable( *reference_to_raw_pointer_operator.expression, names, function_context );
	if( v.value_type == ValueType::Value )
	{
		REPORT_ERROR( ValueIsNotReference, names.GetErrors(), reference_to_raw_pointer_operator.src_loc_ );
		return ErrorValue();
	}
	if( v.value_type == ValueType::ReferenceImut )
	{
		// Disable immutable reference to pointer conversion, because pointer dereference produces mutable value.
		REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), reference_to_raw_pointer_operator.src_loc_ );
		return ErrorValue();
	}

	U_ASSERT( v.location == Variable::Location::Pointer );

	// Reference to pointer conversion can break functional purity, so, disable such conversions in constexpr functions.
	function_context.have_non_constexpr_operations_inside= true;

	RawPointerType raw_pointer_type;
	raw_pointer_type.type= v.type;
	raw_pointer_type.llvm_type= llvm::PointerType::get( v.type.GetLLVMType(), 0u );

	Variable res;
	res.type= std::move(raw_pointer_type);
	res.llvm_value= v.llvm_value;
	res.value_type= ValueType::Value;
	res.location= Variable::Location::LLVMRegister;
	res.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, "ptr" );
	RegisterTemporaryVariable( function_context, res );

	return Value( std::move(res), reference_to_raw_pointer_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( RawPointerToReferenceConversionOutsideUnsafeBlock, names.GetErrors(), raw_pointer_to_reference_operator.src_loc_ );

	const Variable v= BuildExpressionCodeEnsureVariable( *raw_pointer_to_reference_operator.expression, names, function_context );
	const RawPointerType* const raw_pointer_type= v.type.GetRawPointerType();

	if( raw_pointer_type == nullptr )
	{
		REPORT_ERROR( ValueIsNotPointer, names.GetErrors(), raw_pointer_to_reference_operator.src_loc_, v.type );
		return ErrorValue();
	}

	Variable res;
	res.type= raw_pointer_type->type;
	res.llvm_value= CreateMoveToLLVMRegisterInstruction( v, function_context );
	res.value_type= ValueType::ReferenceMut;
	res.location= Variable::Location::Pointer;
	res.node= nullptr; // There is no reference graph node associated with raw pointer.

	return Value( std::move(res), raw_pointer_to_reference_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::NumericConstant& numeric_constant )
{
	U_FundamentalType type= U_FundamentalType::InvalidType;
	const std::string type_suffix= numeric_constant.type_suffix.data();

	if( type_suffix.empty() )
		type= numeric_constant.has_fractional_point ? U_FundamentalType::f64 : U_FundamentalType::i32;
	else if( type_suffix == "u" )
		type= U_FundamentalType::u32;
	// Suffix for size_type
	else if( type_suffix == "s" )
		type= size_type_.GetFundamentalType()->fundamental_type;
	// Simple "f" suffix for 32bit floats.
	else if( type_suffix == "f" )
		type= U_FundamentalType::f32;
	// Short suffixes for chars
	else if( type_suffix ==  "c8" )
		type= U_FundamentalType::char8 ;
	else if( type_suffix == "c16" )
		type= U_FundamentalType::char16;
	else if( type_suffix == "c32" )
		type= U_FundamentalType::char32;
	else
		type=GetFundamentalTypeByName( type_suffix );

	if( type == U_FundamentalType::InvalidType )
	{
		REPORT_ERROR( UnknownNumericConstantType, names.GetErrors(), numeric_constant.src_loc_, numeric_constant.type_suffix.data() );
		return ErrorValue();
	}
	llvm::Type* const llvm_type= GetFundamentalLLVMType( type );

	Variable result;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;
	result.type= FundamentalType( type, llvm_type );

	if( IsInteger( type ) || IsChar( type ) )
		result.constexpr_value=
			llvm::Constant::getIntegerValue( llvm_type, llvm::APInt( llvm_type->getIntegerBitWidth(), numeric_constant.value_int ) );
	else if( IsFloatingPoint( type ) )
		result.constexpr_value=
			llvm::ConstantFP::get( llvm_type, numeric_constant.value_double );
	else
		U_ASSERT(false);

	result.llvm_value= result.constexpr_value;

	result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, "numeric constant " + std::to_string(numeric_constant.value_double) );
	RegisterTemporaryVariable( function_context, result );
	return Value( std::move(result), numeric_constant.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::BooleanConstant& boolean_constant )
{
	U_UNUSED(names);
	
	Variable result;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;
	result.type= bool_type_;

	result.llvm_value= result.constexpr_value=
		llvm::Constant::getIntegerValue(
			fundamental_llvm_types_.bool_ ,
			llvm::APInt( 1u, uint64_t(boolean_constant.value_) ) );

	result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, Keyword( boolean_constant.value_ ? Keywords::true_ : Keywords::false_ ) );
	RegisterTemporaryVariable( function_context, result );
	return Value ( std::move(result), boolean_constant.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::StringLiteral& string_literal )
{
	U_UNUSED( function_context );

	U_FundamentalType char_type= U_FundamentalType::InvalidType;
	uint64_t array_size= ~0u; // ~0 - means non-array
	llvm::Constant* initializer= nullptr;

	const std::string type_suffix= string_literal.type_suffix_.data();

	if( type_suffix.empty() || type_suffix == "u8" )
	{
		char_type= U_FundamentalType::char8;
		array_size= string_literal.value_.size();
		initializer= llvm::ConstantDataArray::getString( llvm_context_, string_literal.value_, false /* not null terminated */ );
	}
	else if( type_suffix == "u16" )
	{
		llvm::SmallVector<llvm::UTF16, 32> str;
		llvm::convertUTF8ToUTF16String( string_literal.value_, str );

		char_type= U_FundamentalType::char16;
		array_size= str.size();
		initializer= llvm::ConstantDataArray::get( llvm_context_, str );
	}
	else if( type_suffix == "u32" )
	{
		std::vector<uint32_t> str;
		for( auto it = string_literal.value_.data(), it_end= it + string_literal.value_.size(); it < it_end; )
			str.push_back( ReadNextUTF8Char( it, it_end ) );

		char_type= U_FundamentalType::char32;
		array_size= str.size();
		initializer= llvm::ConstantDataArray::get( llvm_context_, str );
	}
	// If string literal have char suffix, process it as single char literal.
	else if( type_suffix ==  "c8" || type_suffix == GetFundamentalTypeName( U_FundamentalType::char8  ) )
	{
		if( string_literal.value_.size() == 1u )
		{
			char_type= U_FundamentalType::char8 ;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char8 , uint64_t(string_literal.value_[0]), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names.GetErrors(), string_literal.src_loc_, string_literal.value_ );
	}
	else if( type_suffix == "c16" || type_suffix == GetFundamentalTypeName( U_FundamentalType::char16 ) )
	{
		const char* it_start= string_literal.value_.data();
		const char* const it_end= it_start + string_literal.value_.size();
		const sprache_char c= ReadNextUTF8Char( it_start, it_end );
		if( it_start == it_end && c <= 65535u )
		{
			char_type= U_FundamentalType::char16;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char16, uint64_t(c), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names.GetErrors(), string_literal.src_loc_, string_literal.value_ );
	}
	else if( type_suffix == "c32" || type_suffix== GetFundamentalTypeName( U_FundamentalType::char32 ) )
	{
		const char* it_start= string_literal.value_.data();
		const char* const it_end= it_start + string_literal.value_.size() ;
		const sprache_char c= ReadNextUTF8Char( it_start, it_end );
		if( it_start == it_end )
		{
			char_type= U_FundamentalType::char32;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char32, uint64_t(c), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names.GetErrors(), string_literal.src_loc_, string_literal.value_ );
	}
	else
		REPORT_ERROR( UnknownStringLiteralSuffix, names.GetErrors(), string_literal.src_loc_, type_suffix );

	if( initializer == nullptr )
		return ErrorValue();

	Variable result;
	result.constexpr_value= initializer;
	if( array_size == ~0u )
	{
		result.type= FundamentalType( char_type, GetFundamentalLLVMType( char_type ) );

		result.value_type= ValueType::Value;
		result.location= Variable::Location::LLVMRegister;
		result.llvm_value= initializer;
	}
	else
	{
		ArrayType array_type;
		array_type.type= FundamentalType( char_type, GetFundamentalLLVMType( char_type ) );
		array_type.size= array_size;
		array_type.llvm_type= llvm::ArrayType::get( GetFundamentalLLVMType( char_type ), array_size );
		result.type= std::move(array_type);

		result.value_type= ValueType::ReferenceImut;
		result.location= Variable::Location::Pointer;

		// Use md5 for string literal names.
		llvm::MD5 md5;
		if( const auto constant_data_array = llvm::dyn_cast<llvm::ConstantDataArray>(initializer) )
			md5.update( constant_data_array->getRawDataValues() );
		else if( const auto all_zeros = llvm::dyn_cast<llvm::ConstantAggregateZero>(initializer) )
			md5.update( std::string( size_t(all_zeros->getNumElements() * FundamentalType( char_type ).GetSize()), '\0' ) );
		md5.update( llvm::ArrayRef<uint8_t>( reinterpret_cast<const uint8_t*>(&char_type), sizeof(U_FundamentalType) ) ); // Add type to hash for distinction of zero-sized strings with different types.
		llvm::MD5::MD5Result md5_result;
		md5.final(md5_result);
		const std::string literal_name= ( "_string_literal_" + md5_result.digest() ).str();

		result.llvm_value= CreateGlobalConstantVariable( result.type, literal_name, result.constexpr_value );
	}

	return Value( std::move(result), string_literal.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::MoveOperator& move_operator	)
{
	Synt::ComplexName complex_name(move_operator.src_loc_);
	complex_name.start_value= move_operator.var_name_;

	const Value resolved_value= ResolveValue( names, function_context, complex_name );
	const Variable* const variable_for_move= resolved_value.GetVariable();
	if( variable_for_move == nullptr ||
		variable_for_move->node == nullptr ||
		variable_for_move->node->kind != ReferencesGraphNode::Kind::Variable )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), move_operator.src_loc_, resolved_value.GetKindName() );
		return ErrorValue();
	}
	const ReferencesGraphNodePtr& node= variable_for_move->node;

	bool found_in_variables= false;
	for( const auto& stack_frame : function_context.stack_variables_stack )
	for( const Variable& arg_node : stack_frame->variables_ )
	{
		if( arg_node.node == node )
		{
			found_in_variables= true;
			goto end_variable_search;
		}
	}
	end_variable_search:
	if( !found_in_variables )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), move_operator.src_loc_, resolved_value.GetKindName() );
		return ErrorValue();
	}

	// TODO - maybe allow moving for immutable variables?
	if( variable_for_move->value_type != ValueType::ReferenceMut )
	{
		REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), move_operator.src_loc_ );
		return ErrorValue();
	}
	if( function_context.variables_state.NodeMoved( node ) )
	{
		REPORT_ERROR( AccessingMovedVariable, names.GetErrors(), move_operator.src_loc_, node->name );
		return ErrorValue();
	}

	// If this is mutable variable - it is stack variable or value argument.
	// This can not be temp variable, global variable, or inner argument variable.

	if( function_context.variables_state.HaveOutgoingLinks( node ) )
	{
		REPORT_ERROR( MovedVariableHaveReferences, names.GetErrors(), move_operator.src_loc_, node->name );
		return ErrorValue();
	}

	Variable content= *variable_for_move;
	content.value_type= ValueType::Value;
	content.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, "_moved_" + node->name );

	// We must save inner references of moved variable.
	// TODO - maybe reset inner node of moved variable?
	if( const auto move_variable_inner_node= function_context.variables_state.GetNodeInnerReference( node ) )
	{
		const auto inner_node= function_context.variables_state.CreateNodeInnerReference( content.node, move_variable_inner_node->kind );
		function_context.variables_state.AddLink( move_variable_inner_node, inner_node );
	}
	function_context.variables_state.MoveNode( node );

	RegisterTemporaryVariable( function_context, content );
	return Value( std::move(content), move_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::TakeOperator& take_operator	)
{
	Variable expression_result= BuildExpressionCodeEnsureVariable( *take_operator.expression_, names, function_context );
	if( expression_result.value_type == ValueType::Value ) // If it is value - just pass it.
		return Value( std::move(expression_result), take_operator.src_loc_ );

	if( expression_result.value_type != ValueType::ReferenceMut )
	{
		REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), take_operator.src_loc_ );
		return ErrorValue();
	}

	if( function_context.variables_state.HaveOutgoingLinks( expression_result.node ) )
	{
		REPORT_ERROR( MovedVariableHaveReferences, names.GetErrors(), take_operator.src_loc_, expression_result.node->name );
		return ErrorValue();
	}

	// Allocate variable for result.
	Variable result;
	result.location= Variable::Location::Pointer;
	result.type= expression_result.type;
	result.value_type= ValueType::Value;
	result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( result.type.GetLLVMType() );
	result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, "_moved_" + expression_result.node->name );
	result.llvm_value->setName( result.node->name );

	CreateLifetimeStart( result, function_context );

	SetupReferencesInCopyOrMove( function_context, result, expression_result, names.GetErrors(), take_operator.src_loc_ );

	// Copy content to new variable.
	CopyBytes( expression_result.llvm_value, result.llvm_value, result.type, function_context );

	// Construct empty value in old place.
	ApplyEmptyInitializer( expression_result.node->name, take_operator.src_loc_, expression_result, names, function_context );

	RegisterTemporaryVariable( function_context, result );
	return Value( std::move(result), take_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::CastMut& cast_mut )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( MutableReferenceCastOutsideUnsafeBlock, names.GetErrors(), cast_mut.src_loc_ );

	const Variable var= BuildExpressionCodeEnsureVariable( *cast_mut.expression_, names, function_context );

	Variable result= var;
	result.constexpr_value= nullptr; // Reset constexprness for mutable reference.
	result.value_type= ValueType::ReferenceMut;

	if( var.location == Variable::Location::LLVMRegister )
	{
		result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( var.type.GetLLVMType() );
		if( var.type != void_type_ )
			function_context.llvm_ir_builder.CreateStore( var.llvm_value, result.llvm_value );
	}

	return Value( std::move(result), cast_mut.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::CastImut& cast_imut	)
{
	const Variable var= BuildExpressionCodeEnsureVariable( *cast_imut.expression_, names, function_context );

	Variable result= var;
	result.value_type= ValueType::ReferenceImut;

	if( var.location == Variable::Location::LLVMRegister )
	{
		result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( var.type.GetLLVMType() );
		if( var.type != void_type_ )
			function_context.llvm_ir_builder.CreateStore( var.llvm_value, result.llvm_value );
	}

	return Value( std::move(result), cast_imut.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::CastRef& cast_ref )
{
	return DoReferenceCast( cast_ref.src_loc_, *cast_ref.type_, *cast_ref.expression_, false, names, function_context );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::CastRefUnsafe& cast_ref_unsafe )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( UnsafeReferenceCastOutsideUnsafeBlock, names.GetErrors(), cast_ref_unsafe.src_loc_ );

	return DoReferenceCast( cast_ref_unsafe.src_loc_, *cast_ref_unsafe.type_, *cast_ref_unsafe.expression_, true, names, function_context );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::TypeInfo& typeinfo )
{
	const Type type= PrepareType( *typeinfo.type_, names, function_context );
	if( type == invalid_type_ )
		return ErrorValue();

	if( !EnsureTypeComplete( type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), typeinfo.src_loc_, type );
		return ErrorValue();
	}

	NamesScope& root_namespace= *names.GetRoot();
	BuildTypeInfo( type, *names.GetRoot() );

	Variable& var= typeinfo_cache_[type];
	BuildFullTypeinfo( type, var, root_namespace );

	return Value( var, typeinfo.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::ArrayTypeName& type_name )
{
	return Value( PrepareTypeImpl( names, function_context, type_name ), type_name.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::FunctionTypePtr& type_name )
{
	return Value( PrepareTypeImpl( names, function_context, type_name ), type_name->src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::TupleType& type_name )
{
	return Value( PrepareTypeImpl( names, function_context, type_name ), type_name.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::RawPointerType& type_name )
{
	return Value( PrepareTypeImpl( names, function_context, type_name ), type_name.src_loc_ );
}


std::optional<Value> CodeBuilder::TryCallOverloadedBinaryOperator(
	const OverloadedOperator op,
	const Synt::Expression&  left_expr,
	const Synt::Expression& right_expr,
	const bool evaluate_args_in_reverse_order,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	// Know args types.
	ArgsVector<FunctionType::Param> args;
	const auto state= SaveInstructionsState( function_context );
	{
		const StackVariablesStorage dummy_stack_variables_storage( function_context );
		for( const Synt::Expression* const in_arg : { &left_expr, &right_expr } )
			args.push_back( PreEvaluateArg( *in_arg, names, function_context ) );
	}
	RestoreInstructionsState( function_context, state );

	// Apply here move-assignment.
	if( op == OverloadedOperator::Assign &&
		args.front().is_mutable && args.front().is_reference  &&
		!args.back().is_reference &&
		args.front().type == args.back().type &&
		( args.front().type.GetClassType() != nullptr || args.front().type.GetArrayType() != nullptr || args.front().type.GetTupleType() != nullptr ) )
	{
		// Move here, instead of calling copy-assignment operator. Before moving we must also call destructor for destination.
		const Variable r_var_real= *BuildExpressionCode( right_expr, names, function_context ).GetVariable();

		std::optional<ReferencesGraphNodeHolder> r_var_lock;
		if( r_var_real.node != nullptr )
		{
			r_var_lock.emplace(
				function_context,
				ReferencesGraphNode::Kind::ReferenceImut,
				r_var_real.node->name + " lock" );
			if( !function_context.variables_state.TryAddLink( r_var_real.node, r_var_lock->Node() ) )
				REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), src_loc, r_var_real.node->name );
		}

		const Variable l_var_real= *BuildExpressionCode(  left_expr, names, function_context ).GetVariable();

		SetupReferencesInCopyOrMove( function_context, l_var_real, r_var_real, names.GetErrors(), src_loc );

		if( l_var_real.type.HaveDestructor() )
			CallDestructor( l_var_real.llvm_value, l_var_real.type, function_context, names.GetErrors(), src_loc );

		if( r_var_real.node != nullptr )
			function_context.variables_state.MoveNode( r_var_real.node );

		CopyBytes( r_var_real.llvm_value, l_var_real.llvm_value, l_var_real.type, function_context );
		CreateLifetimeEnd( r_var_real, function_context );

		Variable move_result;
		move_result.type= void_type_;
		return Value( std::move(move_result), src_loc );
	}
	else if( args.front().type == args.back().type && ( args.front().type.GetArrayType() != nullptr || args.front().type.GetTupleType() != nullptr ) )
		return CallBinaryOperatorForArrayOrTuple( op, left_expr, right_expr, src_loc, names, function_context );

	if( const auto overloaded_operator= GetOverloadedOperator( args, op, names, src_loc ) )
	{
		if( overloaded_operator->is_deleted )
			REPORT_ERROR( AccessingDeletedMethod, names.GetErrors(), src_loc );
		if( !( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
			function_context.have_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

		if( overloaded_operator->virtual_table_index != ~0u )
		{
			// We can not fetch virtual function here, because "this" may be evaluated as second operand for some binary operators.
			REPORT_ERROR( NotImplemented, names.GetErrors(), src_loc, "calling virtual binary operators" );
		}

		return
			DoCallFunction(
				overloaded_operator->llvm_function,
				*overloaded_operator->type.GetFunctionType(),
				src_loc,
				nullptr,
				{ &left_expr, &right_expr },
				evaluate_args_in_reverse_order,
				names,
				function_context );
	}

	return std::nullopt;
}

Value CodeBuilder::CallBinaryOperatorForArrayOrTuple(
	OverloadedOperator op,
	const Synt::Expression&  left_expr,
	const Synt::Expression& right_expr,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( op == OverloadedOperator::Assign )
	{
		const Variable r_var= BuildExpressionCodeEnsureVariable( right_expr, names, function_context );
		std::optional<ReferencesGraphNodeHolder> r_var_lock;
		if( r_var.node != nullptr )
		{
			r_var_lock.emplace(
				function_context,
				ReferencesGraphNode::Kind::ReferenceImut,
				r_var.node->name + " lock" );
			if( !function_context.variables_state.TryAddLink( r_var.node, r_var_lock->Node() ) )
				REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), src_loc, r_var.node->name );
		}

		const Variable l_var= BuildExpressionCodeEnsureVariable(  left_expr, names, function_context );
		if( l_var.node != nullptr && function_context.variables_state.HaveOutgoingLinks( l_var.node ) )
			REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), src_loc, l_var.node->name );

		U_ASSERT( l_var.type == r_var.type ); // Checked before.
		if( !l_var.type.IsCopyAssignable() )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_var.type );
			return ErrorValue();
		}
		if( l_var.value_type != ValueType::ReferenceMut )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), src_loc );
			return ErrorValue();
		}

		SetupReferencesInCopyOrMove( function_context, l_var, r_var, names.GetErrors(), src_loc );

		BuildCopyAssignmentOperatorPart(
			l_var.llvm_value, r_var.llvm_value,
			l_var.type,
			function_context );

		Variable result;
		result.type= void_type_;
		return Value( std::move(result), src_loc );
	}
	else
	{
		const Variable l_var= BuildExpressionCodeEnsureVariable( left_expr, names, function_context );
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_var.type );
		return ErrorValue();
	}
}

std::optional<Value> CodeBuilder::TryCallOverloadedUnaryOperator(
	const Variable& variable,
	const OverloadedOperator op,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( variable.type.GetClassType() == nullptr )
		return std::nullopt;

	ArgsVector<FunctionType::Param> args;
	args.emplace_back();
	args.back().type= variable.type;
	args.back().is_mutable= variable.value_type == ValueType::ReferenceMut;
	args.back().is_reference= variable.value_type != ValueType::Value;

	const FunctionVariable* const overloaded_operator= GetOverloadedOperator( args, op, names, src_loc );

	if( overloaded_operator == nullptr )
		return std::nullopt;

	if( !( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
		function_context.have_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

	const std::pair<Variable, llvm::Value*> fetch_result=
		TryFetchVirtualFunction( variable, *overloaded_operator, function_context, names.GetErrors(), src_loc );

	return
		DoCallFunction(
			fetch_result.second,
			*overloaded_operator->type.GetFunctionType(),
			src_loc,
			&fetch_result.first,
			{},
			false,
			names,
			function_context );
}

std::optional<Value> CodeBuilder::TryCallOverloadedPostfixOperator(
	const Variable& variable,
	const llvm::ArrayRef<Synt::Expression>& synt_args,
	const OverloadedOperator op,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	ArgsVector<FunctionType::Param> actual_args;
	actual_args.reserve( 1 + synt_args.size() );

	const auto state= SaveInstructionsState( function_context );
	{
		const StackVariablesStorage dummy_stack_variables_storage( function_context );

		actual_args.push_back( GetArgExtendedType( variable ) );
		for( const Synt::Expression& arg_expression : synt_args )
			actual_args.push_back( PreEvaluateArg( arg_expression, names, function_context ) );
	}
	RestoreInstructionsState( function_context, state );

	const FunctionVariable* const function= GetOverloadedOperator( actual_args, op, names, src_loc );
	if(function == nullptr )
		return std::nullopt;

	if( !( function->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || function->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
		function_context.have_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

	ArgsVector<const Synt::Expression*> synt_args_ptrs;
	synt_args_ptrs.reserve( synt_args.size() );
	for( const Synt::Expression& arg : synt_args )
		synt_args_ptrs.push_back( &arg );

	const auto fetch_result= TryFetchVirtualFunction( variable, *function, function_context, names.GetErrors(), src_loc );

	return DoCallFunction(
		fetch_result.second,
		*function->type.GetFunctionType(),
		src_loc,
		fetch_result.first,
		synt_args_ptrs,
		false,
		names,
		function_context,
		function->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete );
}

Value CodeBuilder::BuildBinaryOperator(
	const Variable& l_var,
	const Variable& r_var,
	const BinaryOperatorType binary_operator,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	using BinaryOperatorType= BinaryOperatorType;

	const Type& l_type= l_var.type;
	const Type& r_type= r_var.type;

	if( ( l_type.GetRawPointerType() != nullptr || r_type.GetRawPointerType() != nullptr ) &&
		( binary_operator == BinaryOperatorType::Add || binary_operator == BinaryOperatorType::Sub ) )
		return BuildBinaryArithmeticOperatorForRawPointers( l_var, r_var, binary_operator, src_loc, names, function_context );

	const FundamentalType* const l_fundamental_type= l_type.GetFundamentalType();
	const FundamentalType* const r_fundamental_type= r_type.GetFundamentalType();

	llvm::Value* const l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
	llvm::Value* const r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );

	Variable result;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;

	switch( binary_operator )
	{
	case BinaryOperatorType::Add:
	case BinaryOperatorType::Sub:
	case BinaryOperatorType::Mul:
	case BinaryOperatorType::Div:
	case BinaryOperatorType::Rem:

		if( r_var.type != l_var.type )
		{
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), src_loc, r_var.type, l_var.type,  BinaryOperatorToString( binary_operator ) );
			return ErrorValue();
		}
		if( l_fundamental_type == nullptr )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
			return ErrorValue();
		}
		else
		{
			if( l_fundamental_type->GetSize() < 4u )
			{
				// Operation supported only for 32 and 64bit operands
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}
			const bool is_float= IsFloatingPoint( l_fundamental_type->fundamental_type );
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || is_float ) )
			{
				// this operations allowed only for integer and floating point operands.
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}

			const bool is_signed= IsSignedInteger( l_fundamental_type->fundamental_type );

			switch( binary_operator )
			{
			case BinaryOperatorType::Add:
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFAdd( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateAdd( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Sub:
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFSub( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateSub( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Div:
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFDiv( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result.llvm_value= function_context.llvm_ir_builder.CreateSDiv( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateUDiv( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Mul:
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFMul( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateMul( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Rem:
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFRem( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result.llvm_value= function_context.llvm_ir_builder.CreateSRem( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateURem( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.type= r_var.type;
		}
		break;

	case BinaryOperatorType::Equal:
	case BinaryOperatorType::NotEqual:
		{
			if( r_var.type != l_var.type )
			{
				REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), src_loc, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
				return ErrorValue();
			}
			if( !( l_fundamental_type != nullptr ||
					l_type.GetEnumType() != nullptr ||
					l_type.GetFunctionPointerType() != nullptr ||
					l_type.GetRawPointerType() != nullptr ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}

			const bool is_void= l_fundamental_type != nullptr && l_fundamental_type->fundamental_type == U_FundamentalType::Void;
			const bool is_float= l_fundamental_type != nullptr && IsFloatingPoint( l_fundamental_type->fundamental_type );

			// Use ordered floating point compare operations, which result is false for NaN, except !=. nan != nan must be true.
			switch( binary_operator )
			{
			case BinaryOperatorType::Equal:
				if( is_void )
					result.llvm_value= llvm::ConstantInt::getTrue( llvm_context_ ); // All "void" values are same.
				else if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFCmpOEQ( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateICmpEQ( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::NotEqual:
				if( is_void )
					result.llvm_value= llvm::ConstantInt::getFalse( llvm_context_ ); // All "void" values are same.
				else if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFCmpUNE( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateICmpNE( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.type= bool_type_;
		}
		break;

	case BinaryOperatorType::Less:
	case BinaryOperatorType::LessEqual:
	case BinaryOperatorType::Greater:
	case BinaryOperatorType::GreaterEqual:
		{
			if( r_var.type != l_var.type )
			{
				REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), src_loc, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
				return ErrorValue();
			}
			// TODO - maybe allow order compare for enums?
			if( !( l_fundamental_type != nullptr || l_type.GetRawPointerType() != nullptr ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
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
					REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
					return ErrorValue();
				}
			}

			switch( binary_operator )
			{
			// Use ordered floating point compare operations, which result is false for NaN.
			case BinaryOperatorType::Less:
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFCmpOLT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result.llvm_value= function_context.llvm_ir_builder.CreateICmpSLT( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateICmpULT( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::LessEqual:
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFCmpOLE( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result.llvm_value= function_context.llvm_ir_builder.CreateICmpSLE( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateICmpULE( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Greater:
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFCmpOGT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result.llvm_value= function_context.llvm_ir_builder.CreateICmpSGT( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateICmpUGT( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::GreaterEqual:
				if( is_float )
					result.llvm_value= function_context.llvm_ir_builder.CreateFCmpOGE( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result.llvm_value= function_context.llvm_ir_builder.CreateICmpSGE( l_value_for_op, r_value_for_op );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateICmpUGE( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.type= bool_type_;
		}
		break;

	case BinaryOperatorType::And:
	case BinaryOperatorType::Or:
	case BinaryOperatorType::Xor:

		if( r_var.type != l_var.type )
		{
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), src_loc, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
			return ErrorValue();
		}
		if( l_fundamental_type == nullptr )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
			return ErrorValue();
		}
		else
		{
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || l_fundamental_type->fundamental_type == U_FundamentalType::Bool ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}

			switch( binary_operator )
			{
			case BinaryOperatorType::And:
				result.llvm_value= function_context.llvm_ir_builder.CreateAnd( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Or:
				result.llvm_value= function_context.llvm_ir_builder.CreateOr( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Xor:
				result.llvm_value= function_context.llvm_ir_builder.CreateXor( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.type= l_type;
		}
		break;

	case BinaryOperatorType::ShiftLeft :
	case BinaryOperatorType::ShiftRight:
		{
			if( l_fundamental_type == nullptr || !IsInteger( l_fundamental_type->fundamental_type ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}
			if( r_fundamental_type == nullptr || !IsUnsignedInteger( r_fundamental_type->fundamental_type ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, r_type );
				return ErrorValue();
			}
			const uint64_t l_type_size= l_fundamental_type->GetSize();
			const uint64_t r_type_size= r_fundamental_type->GetSize();

			llvm::Value* r_value_converted= r_value_for_op;

			// Convert value of shift to type of shifted value. LLVM Reuqired this.
			if( r_type_size > l_type_size )
				r_value_converted= function_context.llvm_ir_builder.CreateTrunc( r_value_converted, l_var.type.GetLLVMType() );
			else if( r_type_size < l_type_size )
				r_value_converted= function_context.llvm_ir_builder.CreateZExt( r_value_converted, l_var.type.GetLLVMType() );

			if( binary_operator == BinaryOperatorType::ShiftLeft )
				result.llvm_value= function_context.llvm_ir_builder.CreateShl( l_value_for_op, r_value_converted );
			else if( binary_operator == BinaryOperatorType::ShiftRight )
			{
				if( IsSignedInteger( l_fundamental_type->fundamental_type ) )
					result.llvm_value= function_context.llvm_ir_builder.CreateAShr( l_value_for_op, r_value_converted );
				else
					result.llvm_value= function_context.llvm_ir_builder.CreateLShr( l_value_for_op, r_value_converted );
			}
			else U_ASSERT(false);

			result.type= l_type;
		}
		break;

	case BinaryOperatorType::LazyLogicalAnd:
	case BinaryOperatorType::LazyLogicalOr:
		U_ASSERT(false);
		break;
	};

	// Produce constexpr value only for constexpr arguments.
	if( l_var.constexpr_value != nullptr && r_var.constexpr_value != nullptr )
		result.constexpr_value= llvm::dyn_cast<llvm::Constant>(result.llvm_value);

	if( result.constexpr_value != nullptr )
	{
		// Undef value can occurs in integer division by zero or something like it.
		// But, if inputs are undef, this means, that they are template-dependent and this is not error case.
		if( llvm::dyn_cast<llvm::UndefValue >(result.constexpr_value) != nullptr )
		{
			REPORT_ERROR( ConstantExpressionResultIsUndefined, names.GetErrors(), src_loc );
			result.constexpr_value= nullptr;
		}
	}

	result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, BinaryOperatorToString(binary_operator) );
	RegisterTemporaryVariable( function_context, result );
	return Value( std::move(result), src_loc );
}

Value CodeBuilder::BuildBinaryArithmeticOperatorForRawPointers(
	const Variable& l_var,
	const Variable& r_var,
	BinaryOperatorType binary_operator,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( l_var.type.GetRawPointerType() != nullptr || r_var.type.GetRawPointerType() != nullptr );

	llvm::Value* const l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
	llvm::Value* const r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );

	Variable result;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;

	if( binary_operator == BinaryOperatorType::Add )
	{
		const uint64_t ptr_size= fundamental_llvm_types_.int_ptr->getIntegerBitWidth() / 8;
		uint64_t int_size= 0u;
		U_FundamentalType int_type= U_FundamentalType::InvalidType;

		llvm::Value* ptr_value= nullptr;
		llvm::Value* index_value= nullptr;

		if( const auto l_fundamental_type= l_var.type.GetFundamentalType() )
		{
			int_size= l_fundamental_type->GetSize();
			int_type= l_fundamental_type->fundamental_type;
			index_value= l_value_for_op;

			U_ASSERT( r_var.type.GetRawPointerType() != nullptr );
			result.type= r_var.type;
			ptr_value= r_value_for_op;
		}
		else if( const auto r_fundamental_type= r_var.type.GetFundamentalType() )
		{
			int_size= r_fundamental_type->GetSize();
			int_type= r_fundamental_type->fundamental_type;
			index_value= r_value_for_op;

			U_ASSERT( l_var.type.GetRawPointerType() != nullptr );
			result.type= l_var.type;
			ptr_value= l_value_for_op;
		}
		else
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_var.type );
			return ErrorValue();
		}

		const Type& element_type= result.type.GetRawPointerType()->type;
		if( !EnsureTypeComplete( element_type ) )
		{
			// Complete types required for pointer arithmetic.
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, element_type );
			return ErrorValue();
		}
		if( !IsInteger( int_type ) || int_size > ptr_size )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, GetFundamentalTypeName( int_type ) );
			return ErrorValue();
		}

		if( int_size < ptr_size )
		{
			if( IsSignedInteger( int_type ) )
				index_value= function_context.llvm_ir_builder.CreateSExt( index_value, fundamental_llvm_types_.int_ptr );
			else
				index_value= function_context.llvm_ir_builder.CreateZExt( index_value, fundamental_llvm_types_.int_ptr );
		}
		result.llvm_value= function_context.llvm_ir_builder.CreateGEP( ptr_value, index_value );
	}
	else if( binary_operator == BinaryOperatorType::Sub )
	{
		const auto ptr_type= l_var.type.GetRawPointerType();
		if( ptr_type == nullptr )
		{
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), src_loc, l_var.type, r_var.type, BinaryOperatorToString( binary_operator ) );
			return ErrorValue();
		}

		if( !EnsureTypeComplete( ptr_type->type ) )
		{
			// Complete types required for pointer arithmetic.
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, ptr_type->type );
			return ErrorValue();
		}

		if( const auto r_ptr_type= r_var.type.GetRawPointerType() )
		{
			// Pointer difference.
			if( *r_ptr_type != *ptr_type )
			{
				REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), src_loc, l_var.type, r_var.type, BinaryOperatorToString( binary_operator ) );
				return ErrorValue();
			}

			const U_FundamentalType diff_type= fundamental_llvm_types_.int_ptr->getIntegerBitWidth() == 32u ? U_FundamentalType::i32 : U_FundamentalType::i64;
			llvm::Type* const diff_llvm_type= GetFundamentalLLVMType( diff_type );

			result.type= FundamentalType( diff_type, diff_llvm_type );

			const auto element_size= data_layout_.getTypeAllocSize( ptr_type->type.GetLLVMType() );
			if( element_size == 0 )
			{
				REPORT_ERROR( DifferenceBetweenRawPointersWithZeroElementSize, names.GetErrors(), src_loc, l_var.type );
				return ErrorValue();
			}

			llvm::Value* const l_as_int= function_context.llvm_ir_builder.CreatePtrToInt( l_value_for_op, diff_llvm_type );
			llvm::Value* const r_as_int= function_context.llvm_ir_builder.CreatePtrToInt( r_value_for_op, diff_llvm_type );
			llvm::Value* const diff= function_context.llvm_ir_builder.CreateSub( l_as_int, r_as_int );
			llvm::Value* const element_size_constant= llvm::ConstantInt::get( diff_llvm_type, uint64_t(element_size), false );
			llvm::Value* const diff_divided= function_context.llvm_ir_builder.CreateSDiv( diff, element_size_constant, "", true /* exact */ );
			result.llvm_value= diff_divided;
		}
		else if( const auto r_fundamental_type= r_var.type.GetFundamentalType() )
		{
			// Subtract integer from pointer.

			const uint64_t ptr_size= fundamental_llvm_types_.int_ptr->getIntegerBitWidth() / 8;
			const uint64_t int_size= r_fundamental_type->GetSize();

			if( !IsInteger( r_fundamental_type->fundamental_type ) || int_size > ptr_size )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, r_var.type );
				return ErrorValue();
			}

			result.type= l_var.type;

			llvm::Value* index_value= r_value_for_op;
			if( int_size < ptr_size )
			{
				if( IsSignedInteger( r_fundamental_type->fundamental_type ) )
					index_value= function_context.llvm_ir_builder.CreateSExt( index_value, fundamental_llvm_types_.int_ptr );
				else
					index_value= function_context.llvm_ir_builder.CreateZExt( index_value, fundamental_llvm_types_.int_ptr );
			}
			llvm::Value* const index_value_negative= function_context.llvm_ir_builder.CreateNeg( index_value );
			result.llvm_value= function_context.llvm_ir_builder.CreateGEP( l_value_for_op, index_value_negative );
		}
		else
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, r_var.type );
			return ErrorValue();
		}
	}
	else{ U_ASSERT(false); }

	result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, BinaryOperatorToString(binary_operator) );
	RegisterTemporaryVariable( function_context, result );
	return Value( std::move(result), src_loc );
}

Value CodeBuilder::BuildLazyBinaryOperator(
	const Synt::Expression& l_expression,
	const Synt::Expression& r_expression,
	const Synt::BinaryOperator& binary_operator,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	// TODO - maybe create separate variables stack frame for right expression evaluation and call destructors?
	const Variable l_var= BuildExpressionCodeEnsureVariable( l_expression, names, function_context );

	if( l_var.type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), binary_operator.src_loc_, bool_type_, l_var.type );
		return ErrorValue();
	}

	llvm::BasicBlock* const l_part_block= function_context.llvm_ir_builder.GetInsertBlock();
	llvm::BasicBlock* const r_part_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const block_after_operator= llvm::BasicBlock::Create( llvm_context_ );

	llvm::Value* const l_var_in_register= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
	if( binary_operator.operator_type_ == BinaryOperatorType::LazyLogicalAnd )
		function_context.llvm_ir_builder.CreateCondBr( l_var_in_register, r_part_block, block_after_operator );
	else if( binary_operator.operator_type_ == BinaryOperatorType::LazyLogicalOr )
		function_context.llvm_ir_builder.CreateCondBr( l_var_in_register, block_after_operator, r_part_block );
	else U_ASSERT(false);

	function_context.function->getBasicBlockList().push_back( r_part_block );
	function_context.llvm_ir_builder.SetInsertPoint( r_part_block );

	ReferencesGraph variables_state_before_r_branch= function_context.variables_state;

	llvm::Value* r_var_in_register= nullptr;
	llvm::Constant* r_var_constepxr_value= nullptr;
	{
		// Right part of lazy operator is conditinal. So, we must destroy its temporaries only in this condition.
		// We doesn`t needs longer lifetime of epxression temporaries, because we use only bool result.
		const StackVariablesStorage r_var_temp_variables_storage( function_context );

		const Variable r_var= BuildExpressionCodeEnsureVariable( r_expression, names, function_context );
		if( r_var.type != bool_type_ )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), binary_operator.src_loc_, bool_type_, r_var.type );
			return ErrorValue();
		}
		r_var_constepxr_value= r_var.constexpr_value;
		r_var_in_register= CreateMoveToLLVMRegisterInstruction( r_var, function_context );

		// Destroy r_var temporaries in this branch.
		CallDestructors( r_var_temp_variables_storage, names, function_context, src_loc );
	}
	function_context.variables_state= MergeVariablesStateAfterIf( { variables_state_before_r_branch, function_context.variables_state }, names.GetErrors(), src_loc );

	llvm::BasicBlock* const r_part_end_block= function_context.llvm_ir_builder.GetInsertBlock();

	function_context.llvm_ir_builder.CreateBr( block_after_operator );
	function_context.function->getBasicBlockList().push_back( block_after_operator );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_operator );

	llvm::PHINode* const phi= function_context.llvm_ir_builder.CreatePHI( fundamental_llvm_types_.bool_, 2u );
	phi->addIncoming( l_var_in_register, l_part_block );
	phi->addIncoming( r_var_in_register, r_part_end_block );

	Variable result;
	result.type= bool_type_;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;
	result.llvm_value= phi;

	// Evaluate constexpr value.
	// TODO - remove all blocks code in case of constexpr?
	if( l_var.constexpr_value != nullptr && r_var_constepxr_value != nullptr )
	{
		if( binary_operator.operator_type_ == BinaryOperatorType::LazyLogicalAnd )
			result.constexpr_value= llvm::ConstantExpr::getAnd( l_var.constexpr_value, r_var_constepxr_value );
		else if( binary_operator.operator_type_ == BinaryOperatorType::LazyLogicalOr )
			result.constexpr_value= llvm::ConstantExpr::getOr ( l_var.constexpr_value, r_var_constepxr_value );
		else
			U_ASSERT(false);
	}

	result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, BinaryOperatorToString(binary_operator.operator_type_));
	RegisterTemporaryVariable( function_context, result );
	return Value( std::move(result), src_loc );
}


Value CodeBuilder::DoReferenceCast(
	const SrcLoc& src_loc,
	const Synt::TypeName& type_name,
	const Synt::Expression& expression,
	bool enable_unsafe,
	NamesScope& names,
	FunctionContext& function_context )
{
	const Type type= PrepareType( type_name, names, function_context );
	if( type == invalid_type_ )
		return ErrorValue();

	const Variable var= BuildExpressionCodeEnsureVariable( expression, names, function_context );

	Variable result;
	result.type= type;
	result.value_type= var.value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut; // "ValueType" here converts into ConstReference.
	result.location= Variable::Location::Pointer;
	result.node= var.node;

	llvm::Value* src_value= var.llvm_value;
	if( var.location == Variable::Location::LLVMRegister )
	{
		src_value= function_context.alloca_ir_builder.CreateAlloca( var.type.GetLLVMType() );
		if( var.type != void_type_ )
			function_context.llvm_ir_builder.CreateStore( var.llvm_value, src_value );
	}

	if( type == var.type )
		result.llvm_value= src_value;
	else
	{
		// Complete types required for both safe and unsafe casting, except unsafe void to anything cast.
		// This needs, becasue we must emit same code for places where types yet not complete, and where they are complete.
		if( !EnsureTypeComplete( type ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, type );

		if( !EnsureTypeComplete( var.type ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, var.type );

		if( ReferenceIsConvertible( var.type, type, names.GetErrors(), src_loc ) )
			result.llvm_value= CreateReferenceCast( src_value, var.type, type, function_context );
		else
		{
			result.llvm_value= function_context.llvm_ir_builder.CreatePointerCast( src_value, type.GetLLVMType()->getPointerTo() );
			if( !enable_unsafe )
				REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, type, var.type );
		}
	}

	return Value( std::move(result), src_loc );
}

Value CodeBuilder::CallFunction(
	const Value& function_value,
	const std::vector<Synt::Expression>& synt_args,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(function_value);

	if( const Type* const type= function_value.GetTypeName() )
		return Value( BuildTempVariableConstruction( *type, synt_args, src_loc, names, function_context ), src_loc );

	const Variable* this_= nullptr;
	const OverloadedFunctionsSet* functions_set= function_value.GetFunctionsSet();

	if( functions_set != nullptr )
	{}
	else if( const ThisOverloadedMethodsSet* const this_overloaded_methods_set=
		function_value.GetThisOverloadedMethodsSet() )
	{
		functions_set= &this_overloaded_methods_set->GetOverloadedFunctionsSet();
		this_= &this_overloaded_methods_set->this_;
	}
	else if( const Variable* const callable_variable= function_value.GetVariable() )
	{
		if( const FunctionPointerType* const function_pointer= callable_variable->type.GetFunctionPointerType() )
		{
			function_context.have_non_constexpr_operations_inside= true; // Calling function, using pointer, is not constexpr. We can not garantee, that called function is constexpr.

			// Call function pointer directly.
			if( function_pointer->function.params.size() != synt_args.size() )
			{
				REPORT_ERROR( InvalidFunctionArgumentCount, names.GetErrors(), src_loc, synt_args.size(), function_pointer->function.params.size() );
				return ErrorValue();
			}

			std::vector<const Synt::Expression*> args;
			args.reserve( synt_args.size() );
			for( const Synt::Expression& arg : synt_args )
				args.push_back( &arg );

			llvm::Value* const func_itself= CreateMoveToLLVMRegisterInstruction( *callable_variable, function_context );

			return
				DoCallFunction(
					func_itself, function_pointer->function, src_loc,
					nullptr, args, false,
					names, function_context );
		}

		// Try to call overloaded () operator.
		// DO NOT fill "this" here and continue this function because we should process callable object as non-this.

		if( auto res= TryCallOverloadedPostfixOperator( *callable_variable, synt_args, OverloadedOperator::Call, src_loc, names, function_context ) )
			return std::move(*res);
	}

	if( functions_set == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, function_value.GetKindName() );
		return ErrorValue();
	}

	size_t total_args= (this_ == nullptr ? 0u : 1u) + synt_args.size();

	const FunctionVariable* function_ptr= nullptr;

	// Make preevaluation af arguments for selection of overloaded function.
	{
		ArgsVector<FunctionType::Param> actual_args;
		actual_args.reserve( total_args );

		const auto state= SaveInstructionsState( function_context );
		{
			const StackVariablesStorage dummy_stack_variables_storage( function_context );

			if( this_ != nullptr )
				actual_args.push_back( GetArgExtendedType( *this_ ) );

			for( const Synt::Expression& arg_expression : synt_args )
				actual_args.push_back( PreEvaluateArg( arg_expression, names, function_context ) );
		}
		RestoreInstructionsState( function_context, state );

		function_ptr=
			GetOverloadedFunction( *functions_set, actual_args, this_ != nullptr, names.GetErrors(), src_loc );
	}

	// SPRACHE_TODO - try get function with "this" parameter in signature and without it.
	// We must support static functions call using "this".
	if( function_ptr == nullptr )
		return ErrorValue();
	const FunctionVariable& function= *function_ptr;
	const FunctionType& function_type= *function.type.GetFunctionType();

	if( this_ != nullptr && !function.is_this_call )
	{
		// Static function call via "this".
		// Just dump first "this" arg.
		total_args--;
		this_= nullptr;
	}

	if( function_ptr->is_deleted )
		REPORT_ERROR( AccessingDeletedMethod, names.GetErrors(), src_loc );

	if( !( function_ptr->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || function_ptr->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
		function_context.have_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

	ArgsVector<const Synt::Expression*> synt_args_ptrs;
	synt_args_ptrs.reserve( synt_args.size() );
	for( const Synt::Expression& arg : synt_args )
		synt_args_ptrs.push_back( &arg );

	Variable this_casted;
	llvm::Value* llvm_function_ptr= function.llvm_function;
	if( this_ != nullptr )
	{
		auto fetch_result= TryFetchVirtualFunction( *this_, function, function_context, names.GetErrors(), src_loc );
		this_casted= std::move( fetch_result.first );
		llvm_function_ptr= fetch_result.second;
		this_= &this_casted;
	}

	return
		DoCallFunction(
			llvm_function_ptr, function_type,
			src_loc,
			this_,
			synt_args_ptrs, false,
			names, function_context,
			function.constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete );
}

Value CodeBuilder::DoCallFunction(
	llvm::Value* const function,
	const FunctionType& function_type,
	const SrcLoc& call_src_loc,
	const Variable* const this_, // optional
	const llvm::ArrayRef<const Synt::Expression*>& args,
	const bool evaluate_args_in_reverse_order,
	NamesScope& names,
	FunctionContext& function_context,
	const bool func_is_constexpr )
{
	return DoCallFunction(
		function,
		function_type,
		call_src_loc,
		this_ == nullptr ? llvm::ArrayRef<Variable>() : llvm::ArrayRef<Variable>( *this_ ),
		args,
		evaluate_args_in_reverse_order,
		names,
		function_context,
		func_is_constexpr );
}

Value CodeBuilder::DoCallFunction(
	llvm::Value* function,
	const FunctionType& function_type,
	const SrcLoc& call_src_loc,
	const llvm::ArrayRef<Variable>& preevaluated_args,
	const llvm::ArrayRef<const Synt::Expression*>& args,
	const bool evaluate_args_in_reverse_order,
	NamesScope& names,
	FunctionContext& function_context,
	const bool func_is_constexpr )
{
	if( function_type.unsafe && !function_context.is_in_unsafe_block )
		REPORT_ERROR( UnsafeFunctionCallOutsideUnsafeBlock, names.GetErrors(), call_src_loc );

	const size_t arg_count= preevaluated_args.size() + args.size();
	U_ASSERT( arg_count == function_type.params.size() );

	ArgsVector<llvm::Value*> llvm_args;
	ArgsVector<llvm::Constant*> constant_llvm_args;
	llvm_args.resize( arg_count, nullptr );

	std::vector< ReferencesGraphNodeHolder > args_nodes;
	std::vector< ReferencesGraphNodeHolder > locked_args_inner_references;

	for( size_t i= 0u; i < arg_count; ++i )
	{
		const size_t j= evaluate_args_in_reverse_order ? arg_count - i - 1u : i;

		const FunctionType::Param& param= function_type.params[j];

		Variable expr;
		SrcLoc src_loc;
		if( j < preevaluated_args.size() )
		{
			expr= preevaluated_args[j];
			src_loc= call_src_loc;
		}
		else
		{
			expr= BuildExpressionCodeEnsureVariable( *args[ j - preevaluated_args.size() ], names, function_context );
			src_loc= Synt::GetExpressionSrcLoc( *args[ j - preevaluated_args.size() ] );
		}

		if( expr.constexpr_value != nullptr && !( param.is_reference && param.is_mutable ) )
			constant_llvm_args.push_back( expr.constexpr_value );

		if( param.is_reference )
		{
			if( !ReferenceIsConvertible( expr.type, param.type, names.GetErrors(), call_src_loc ) &&
				GetConversionConstructor( expr.type, param.type, names.GetErrors(), src_loc ) == nullptr )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, param.type, expr.type );
				return ErrorValue();
			}

			if( param.is_mutable )
			{
				if( expr.value_type == ValueType::Value )
				{
					REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), src_loc );
					return ErrorValue();
				}
				if( expr.value_type == ValueType::ReferenceImut )
				{
					REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), src_loc );
					return ErrorValue();
				}

				if( expr.type == param.type )
					llvm_args[j]= expr.llvm_value;
				else
					llvm_args[j]= CreateReferenceCast( expr.llvm_value, expr.type, param.type, function_context );
			}
			else
			{
				if( expr.value_type == ValueType::Value && expr.location == Variable::Location::LLVMRegister )
				{
					// Bind value to const reference.
					// TODO - support nonfundamental values.
					llvm::Value* const temp_storage= function_context.alloca_ir_builder.CreateAlloca( expr.type.GetLLVMType() );
					if( expr.type != void_type_ )
						function_context.llvm_ir_builder.CreateStore( expr.llvm_value, temp_storage );
					llvm_args[j]= temp_storage;
				}
				else
					llvm_args[j]= expr.llvm_value;

				if( expr.type != param.type )
				{
					if( expr.type.ReferenceIsConvertibleTo( param.type ) )
						llvm_args[j]= CreateReferenceCast( llvm_args[j], expr.type, param.type, function_context );
					else
					{
						const auto conversion_constructor= GetConversionConstructor( expr.type, param.type, names.GetErrors(), src_loc );
						U_ASSERT( conversion_constructor != nullptr );
						expr= ConvertVariable( expr, param.type, *conversion_constructor, names, function_context, src_loc );
						llvm_args[j]= expr.llvm_value;
					}
				}
			}

			// Lock references.
			args_nodes.emplace_back(
				function_context,
				param.is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut,
				"reference_arg " + std::to_string(i) );

			if( expr.node != nullptr && !function_context.variables_state.TryAddLink( expr.node, args_nodes.back().Node() ) )
				REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), src_loc, expr.node->name );

			// Lock inner references.
			if( expr.node != nullptr )
			{
				const auto inner_references= function_context.variables_state.GetAccessibleVariableNodesInnerReferences( expr.node );
				if( !inner_references.empty() )
				{
					EnsureTypeComplete( param.type );
					if( param.type.ReferencesTagsCount() > 0 )
					{
						bool is_mutable= false;
						for( const ReferencesGraphNodePtr& inner_reference : inner_references )
							is_mutable= is_mutable || inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

						locked_args_inner_references.emplace_back(
							function_context,
							is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut,
							"inner reference lock " + std::to_string(i) );

						for( const ReferencesGraphNodePtr& inner_reference : inner_references )
						{
							if( !function_context.variables_state.TryAddLink( inner_reference, locked_args_inner_references.back().Node() ) )
								REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), src_loc, inner_reference->name );
						}
					}
				}
			}
		}
		else
		{
			args_nodes.emplace_back( function_context, ReferencesGraphNode::Kind::Variable, "value_arg_" + std::to_string(i) );

			if( !ReferenceIsConvertible( expr.type, param.type, names.GetErrors(), call_src_loc ) &&
				GetConversionConstructor( expr.type, param.type, names.GetErrors(), src_loc ) == nullptr )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, param.type, expr.type );
				return ErrorValue();
			}

			if( expr.type != param.type )
			{
				if( expr.type.ReferenceIsConvertibleTo( param.type ) ){}
				else
				{
					const auto conversion_constructor= GetConversionConstructor( expr.type, param.type, names.GetErrors(), src_loc );
					U_ASSERT( conversion_constructor != nullptr );
					expr= ConvertVariable( expr, param.type, *conversion_constructor, names, function_context, src_loc );
				}
			}

			if( param.type == void_type_ )
				llvm_args[j]= llvm::UndefValue::get( fundamental_llvm_types_.void_ ); // Hack for interpreter - it can not process regular constant values properly.
			else if( param.type.GetFundamentalType() != nullptr ||
				param.type.GetEnumType() != nullptr ||
				param.type.GetRawPointerType() != nullptr ||
				param.type.GetFunctionPointerType() != nullptr )
				llvm_args[j]= CreateMoveToLLVMRegisterInstruction( expr, function_context );
			else if( param.type.GetClassType() != nullptr || param.type.GetArrayType() != nullptr || param.type.GetTupleType() != nullptr )
			{
				// Lock inner references.
				// Do it only if arg type can contain any reference inside.
				// Do it before potential moving.
				EnsureTypeComplete( param.type ); // arg type for value arg must be already complete.
				if( expr.node != nullptr && param.type.ReferencesTagsCount() > 0u )
				{
					const auto inner_references= function_context.variables_state.GetAccessibleVariableNodesInnerReferences( expr.node );
					if( !inner_references.empty() )
					{
						bool is_mutable= false;
						for( const ReferencesGraphNodePtr& inner_reference : inner_references )
							is_mutable= is_mutable || inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

						const auto value_arg_inner_node=
							function_context.variables_state.CreateNodeInnerReference( args_nodes.back().Node(), is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );

						for( const ReferencesGraphNodePtr inner_reference : inner_references )
						{
							if( !function_context.variables_state.TryAddLink( inner_reference, value_arg_inner_node ) )
								REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), src_loc, inner_reference->name );
						}
					}
				}

				if( expr.value_type == ValueType::Value && expr.type == param.type )
				{
					// Do not call copy constructors - just move.
					if( expr.node != nullptr )
						function_context.variables_state.MoveNode( expr.node );
					llvm_args[j]= expr.llvm_value;
				}
				else
				{
					if( !param.type.IsCopyConstructible() )
					{
						// Can not call function with value parameter, because for value parameter needs copy, but parameter type is not copyable.
						// TODO - print more reliable message.
						REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, param.type );
						return ErrorValue();
					}

					// Create copy of class or tuple value. Call copy constructor.
					// TODO - create lifetime start/end?
					llvm::Value* const arg_copy= function_context.alloca_ir_builder.CreateAlloca( param.type.GetLLVMType() );

					llvm_args[j]= arg_copy;
					BuildCopyConstructorPart(
						arg_copy,
						CreateReferenceCast( expr.llvm_value, expr.type, param.type, function_context ),
						param.type,
						function_context );
				}
			}
			else U_ASSERT( false );
		}

		// Destroy unused temporary variables after each argument evaluation.
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), call_src_loc );
	} // for args
	U_ASSERT( args_nodes.size() == arg_count );
	if( evaluate_args_in_reverse_order )
		std::reverse( args_nodes.begin(), args_nodes.end() );

	const bool return_value_is_sret= function_type.IsStructRet();

	Variable result;
	result.type= function_type.return_type;
	if( return_value_is_sret )
	{
		if( !EnsureTypeComplete( function_type.return_type ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), call_src_loc, function_type.return_type );

		result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( function_type.return_type.GetLLVMType() );
		llvm_args.insert( llvm_args.begin(), result.llvm_value );
		constant_llvm_args.insert( constant_llvm_args.begin(), nullptr );

		CreateLifetimeStart( result, function_context );
	}

	llvm::Value* call_result= nullptr;
	llvm::Constant* constant_call_result= nullptr;
	if( std::find( llvm_args.begin(), llvm_args.end(), nullptr ) == llvm_args.end() )
	{
		// Currently, we can not pass back referenes from constexpr functions evaluator.
		if( func_is_constexpr && constant_llvm_args.size() == llvm_args.size() &&
			!function_type.return_value_is_reference && function_type.return_type.ReferencesTagsCount() == 0u )
		{
			const ConstexprFunctionEvaluator::Result evaluation_result=
				constexpr_function_evaluator_.Evaluate( llvm::dyn_cast<llvm::Function>(function), constant_llvm_args );

			for( const std::string& error_text : evaluation_result.errors )
			{
				CodeBuilderError error;
				error.code= CodeBuilderErrorCode::ConstexprFunctionEvaluationError;
				error.src_loc= call_src_loc;
				error.text= error_text;
				names.GetErrors().push_back( std::move(error) );
			}
			if( evaluation_result.errors.empty() && evaluation_result.result_constant != nullptr )
			{
				if( return_value_is_sret ) // We needs here block of memory with result constant struct.
					MoveConstantToMemory( result.llvm_value, evaluation_result.result_constant, function_context );

				if( !function_type.return_value_is_reference && function_type.return_type == void_type_ )
					constant_call_result= llvm::Constant::getNullValue( fundamental_llvm_types_.void_ );
				else
					constant_call_result= evaluation_result.result_constant;
			}
		}
		if( constant_call_result != nullptr )
			call_result= constant_call_result;
		else
		{
			call_result= function_context.llvm_ir_builder.CreateCall( function, llvm_args );
			if( !function_type.return_value_is_reference && function_type.return_type == void_type_ )
				call_result= llvm::UndefValue::get( fundamental_llvm_types_.void_ );
		}
	}
	else
		call_result= llvm::UndefValue::get( llvm::dyn_cast<llvm::FunctionType>(function->getType())->getReturnType() );


	// Clear inner references locks. Do this BEFORE result references management.
	locked_args_inner_references.clear();

	if( !return_value_is_sret )
		result.llvm_value= call_result;
	result.constexpr_value= constant_call_result;

	if( function_type.return_value_is_reference )
	{
		result.location= Variable::Location::Pointer;
		result.value_type= function_type.return_value_is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut;
		result.node= function_context.variables_state.AddNode( function_type.return_value_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut, "fn_result " + result.type.ToString() );
	}
	else
	{
		if( !EnsureTypeComplete( function_type.return_type ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), call_src_loc, function_type.return_type );

		result.location= return_value_is_sret ? Variable::Location::Pointer : Variable::Location::LLVMRegister;
		result.value_type= ValueType::Value;
		result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, "fn_result " + result.type.ToString() );
	}

	// Prepare result references.
	if( function_type.return_value_is_reference )
	{
		for( const FunctionType::ParamReference& arg_reference : function_type.return_references )
		{
			if( arg_reference.second == FunctionType::c_arg_reference_tag_number )
			{
				const auto node= args_nodes[arg_reference.first].Node();
				if( !function_context.variables_state.TryAddLink( node, result.node ) )
					REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), call_src_loc, node->name );
			}
			else
			{
				for( const ReferencesGraphNodePtr& accesible_node : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( args_nodes[arg_reference.first].Node() ) )
				{
					if( !function_context.variables_state.TryAddLink( accesible_node, result.node ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), call_src_loc, accesible_node->name );
				}
			}
		}
	}
	else if( function_type.return_type.ReferencesTagsCount() > 0u )
	{
		bool inner_reference_is_mutable= false;

		// First, know, what kind of reference we needs - mutable or immutable.
		for( const FunctionType::ParamReference& arg_reference : function_type.return_references )
		{
			if( arg_reference.second == FunctionType::c_arg_reference_tag_number )
			{
				const auto node_kind= args_nodes[arg_reference.first].Node()->kind;

				if( node_kind == ReferencesGraphNode::Kind::Variable ||
					node_kind == ReferencesGraphNode::Kind::ReferenceMut )
					inner_reference_is_mutable= true;
				else if( node_kind == ReferencesGraphNode::Kind::ReferenceImut ) {}
				else U_ASSERT( false ); // Unexpected node kind.
			}
			else
			{
				for( const ReferencesGraphNodePtr& accesible_node : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( args_nodes[arg_reference.first].Node() ) )
				{
					if( accesible_node->kind == ReferencesGraphNode::Kind::Variable ||
						accesible_node->kind == ReferencesGraphNode::Kind::ReferenceMut )
						inner_reference_is_mutable= true;
					else if( accesible_node->kind == ReferencesGraphNode::Kind::ReferenceImut ) {}
					else U_ASSERT( false ); // Unexpected node kind.
				}
			}
		}

		// Then, create inner node and link input nodes with it.
		const auto inner_reference_node=
			function_context.variables_state.CreateNodeInnerReference( result.node, inner_reference_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );

		for( const FunctionType::ParamReference& arg_reference : function_type.return_references )
		{
			if( arg_reference.second == FunctionType::c_arg_reference_tag_number )
			{
				const auto node= args_nodes[arg_reference.first].Node();
				if( !function_context.variables_state.TryAddLink( node, inner_reference_node ) )
					REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), call_src_loc, node->name );
			}
			else
				for( const ReferencesGraphNodePtr& accesible_node : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( args_nodes[arg_reference.first].Node() ) )
				{
					if( !function_context.variables_state.TryAddLink( accesible_node, inner_reference_node ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), call_src_loc, accesible_node->name );
				}
		}
	}

	// Setup references after call.
	for( const FunctionType::ReferencePollution& referene_pollution : function_type.references_pollution )
	{
		const size_t dst_arg= referene_pollution.dst.first;
		U_ASSERT( dst_arg < function_type.params.size() );

		// It's possible that reference pollution is set for types without references inside.
		if( function_type.params[ dst_arg ].type.ReferencesTagsCount() == 0 )
			continue;

		bool src_variables_is_mut= false;
		ReferencesGraph::NodesSet src_nodes;
		if( referene_pollution.src.second == FunctionType::c_arg_reference_tag_number )
		{
			// Reference-arg itself
			U_ASSERT( function_type.params[ referene_pollution.src.first ].is_reference );
			src_nodes.emplace( args_nodes[ referene_pollution.src.first ].Node() );

			if( function_type.params[ referene_pollution.src.first ].is_mutable )
				src_variables_is_mut= true;
		}
		else
		{
			// Variables, referenced by inner argument references.
			U_ASSERT( referene_pollution.src.second == 0u );// Currently we support one tag per struct.

			if( function_type.params[ referene_pollution.src.first ].type.ReferencesTagsCount() == 0 )
				continue;

			for( const ReferencesGraphNodePtr& inner_reference : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( args_nodes[ referene_pollution.src.first ].Node() ) )
			{
				src_nodes.insert( inner_reference );
				if( inner_reference->kind != ReferencesGraphNode::Kind::ReferenceImut )
					src_variables_is_mut= true;
			}
		}

		if( function_type.params[ dst_arg ].is_reference && !src_nodes.empty() )
		{
			const bool dst_inner_reference_is_mut= function_type.params[ dst_arg ].type.GetInnerReferenceType() == InnerReferenceType::Mut;
			// Even if reference-pollution is mutable, but if src vars is immutable, link as immutable.
			const bool result_node_is_mut= src_variables_is_mut && dst_inner_reference_is_mut;

			for( const ReferencesGraphNodePtr& dst_node : function_context.variables_state.GetAllAccessibleVariableNodes( args_nodes[ dst_arg ].Node() ) )
			{
				ReferencesGraphNodePtr inner_reference= function_context.variables_state.GetNodeInnerReference( dst_node );
				if( inner_reference == nullptr )
				{
					inner_reference=
						function_context.variables_state.CreateNodeInnerReference(
							dst_node,
							result_node_is_mut ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
				}
				if( ( inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut  && !result_node_is_mut ) ||
					( inner_reference->kind == ReferencesGraphNode::Kind::ReferenceImut &&  result_node_is_mut ))
					REPORT_ERROR( InnerReferenceMutabilityChanging, names.GetErrors(), call_src_loc, inner_reference->name );

				for( const ReferencesGraphNodePtr& src_node : src_nodes )
				{
					if( !function_context.variables_state.TryAddLink( src_node, inner_reference ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), call_src_loc, src_node->name );
				}
			}
		}
		else
		{
			// Does it have sence, write references to value argument?
		}
	}

	args_nodes.clear();
	DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), call_src_loc );
	RegisterTemporaryVariable( function_context, result );

	return Value( std::move(result), call_src_loc );
}

Variable CodeBuilder::BuildTempVariableConstruction(
	const Type& type,
	const std::vector<Synt::Expression>& synt_args,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( !EnsureTypeComplete( type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, type );
		return Variable();
	}
	else if( type.IsAbstract() )
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), src_loc, type );

	Variable variable;
	variable.type= type;
	variable.location= Variable::Location::Pointer;
	variable.value_type= ValueType::ReferenceMut;
	variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( type.GetLLVMType() );
	variable.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, "temp " + type.ToString() );

	CreateLifetimeStart( variable, function_context );

	variable.constexpr_value= ApplyConstructorInitializer( variable, synt_args, src_loc, names, function_context );
	variable.value_type= ValueType::Value; // Make value after construction

	RegisterTemporaryVariable( function_context, variable );
	return variable;
}

Variable CodeBuilder::ConvertVariable(
	const Variable& variable,
	const Type& dst_type,
	const FunctionVariable& conversion_constructor,
	NamesScope& names,
	FunctionContext& function_context,
	const SrcLoc& src_loc )
{
	if( !EnsureTypeComplete( dst_type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, dst_type );
		return Variable();
	}

	Variable result;
	result.type= dst_type;
	result.location= Variable::Location::Pointer;
	result.value_type= ValueType::ReferenceMut;
	result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( dst_type.GetLLVMType() );
	result.node= function_context.variables_state.AddNode( ReferencesGraphNode::Kind::Variable, "temp " + dst_type.ToString() );

	CreateLifetimeStart( result, function_context );

	{
		// Create temp variables frame to prevent destruction of "src".
		const StackVariablesStorage temp_variables_storage( function_context );

		DoCallFunction(
			conversion_constructor.llvm_function,
			*conversion_constructor.type.GetFunctionType(),
			src_loc,
			{ result, variable },
			{},
			false,
			names,
			function_context,
			false );

		CallDestructors( temp_variables_storage, names, function_context, src_loc );
	}

	result.value_type= ValueType::Value; // Make value after construction
	RegisterTemporaryVariable( function_context, result );
	return result;
}

FunctionType::Param CodeBuilder::PreEvaluateArg( const Synt::Expression& expression, NamesScope& names, FunctionContext& function_context )
{
	if( function_context.args_preevaluation_cache.count(&expression) == 0 )
	{
		const Variable v= BuildExpressionCodeEnsureVariable( expression, names, function_context );
		function_context.args_preevaluation_cache.emplace( &expression,  GetArgExtendedType(v) );
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), Synt::GetExpressionSrcLoc( expression ) );

	}
	return function_context.args_preevaluation_cache[&expression];
}

FunctionType::Param CodeBuilder::GetArgExtendedType( const Variable& variable )
{
	FunctionType::Param arg_type_extended;
	arg_type_extended.type= variable.type;
	arg_type_extended.is_reference= variable.value_type != ValueType::Value;
	arg_type_extended.is_mutable= variable.value_type == ValueType::ReferenceMut;
	return arg_type_extended;
}

} // namespace U
