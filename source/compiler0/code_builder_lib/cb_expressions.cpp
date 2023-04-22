#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
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

VariablePtr CodeBuilder::BuildExpressionCodeEnsureVariable(
	const Synt::Expression& expression,
	NamesScope& names,
	FunctionContext& function_context )
{
	Value result= BuildExpressionCode( expression, names, function_context );

	const VariablePtr result_variable= result.GetVariable();
	if( result_variable == nullptr )
	{
		if( result.GetErrorValue() == nullptr )
			REPORT_ERROR( ExpectedVariable, names.GetErrors(), Synt::GetExpressionSrcLoc( expression ), result.GetKindName() );

		const VariablePtr dummy_result=
			std::make_shared<Variable>(
				invalid_type_,
				ValueType::Value,
				Variable::Location::Pointer,
				"error value",
				llvm::UndefValue::get( invalid_type_.GetLLVMType()->getPointerTo() ) );

		function_context.variables_state.AddNode( dummy_result );
		RegisterTemporaryVariable( function_context, dummy_result );

		return dummy_result;
	}
	return result_variable;
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
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( *indexation_operator.expression_, names, function_context );

	if( variable->type.GetClassType() != nullptr ) // If this is class - try call overloaded [] operator.
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

		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), indexation_operator.src_loc_, variable->type );
		return ErrorValue();
	}

	if( const ArrayType* const array_type= variable->type.GetArrayType() )
	{
		// Lock variable. We must prevent modification of this variable in index calcualtion.
		// Do not forget to unregister it in case of error-return!
		const VariablePtr variable_lock=
			std::make_shared<Variable>(
				variable->type,
				variable->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				variable->location,
				variable->name + " lock" );
		function_context.variables_state.AddNode( variable_lock );
		function_context.variables_state.TryAddLink( variable, variable_lock, names.GetErrors(), indexation_operator.src_loc_ );

		const VariablePtr index= BuildExpressionCodeEnsureVariable( *indexation_operator.index_, names, function_context );

		const FundamentalType* const index_fundamental_type= index->type.GetFundamentalType();
		if( !( index_fundamental_type != nullptr && (
			( index->constexpr_value != nullptr && IsInteger( index_fundamental_type->fundamental_type ) ) ||
			( index->constexpr_value == nullptr && IsUnsignedInteger( index_fundamental_type->fundamental_type ) ) ) ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), indexation_operator.src_loc_, "any uint32_teger", index->type );
			function_context.variables_state.RemoveNode( variable_lock );
			return ErrorValue();
		}

		// If index is constant and not undefined statically check index.
		if( index->constexpr_value != nullptr )
		{
			const llvm::APInt index_value= index->constexpr_value->getUniqueInteger();
			if( IsSignedInteger(index_fundamental_type->fundamental_type) )
			{
				if( index_value.getLimitedValue() >= array_type->element_count || index_value.isNegative() )
					REPORT_ERROR( ArrayIndexOutOfBounds, names.GetErrors(), indexation_operator.src_loc_, index_value.getSExtValue(), array_type->element_count );
			}
			else
			{
				if( index_value.getLimitedValue() >= array_type->element_count )
					REPORT_ERROR( ArrayIndexOutOfBounds, names.GetErrors(), indexation_operator.src_loc_, index_value.getLimitedValue(), array_type->element_count );
			}
		}

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_value= CreateMoveToLLVMRegisterInstruction( *index, function_context );

		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), indexation_operator.src_loc_ ); // Destroy temporaries of index expression.

		const VariableMutPtr result=
			std::make_shared<Variable>(
				array_type->element_type,
				variable->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				variable->name + "[]" );

		if( variable->constexpr_value != nullptr && index->constexpr_value != nullptr )
			result->constexpr_value= variable->constexpr_value->getAggregateElement( index->constexpr_value );

		// If index is not constant - check bounds.
		if( index->constexpr_value == nullptr && !function_context.is_functionless_context )
		{
			const uint64_t index_size= index_fundamental_type->GetSize();
			const uint64_t size_type_size= size_type_.GetFundamentalType()->GetSize();
			if( index_size > size_type_size )
				index_value= function_context.llvm_ir_builder.CreateTrunc( index_value, size_type_.GetLLVMType() );
			else if( index_size < size_type_size )
				index_value= function_context.llvm_ir_builder.CreateZExt( index_value, size_type_.GetLLVMType() );

			llvm::Value* const condition=
				function_context.llvm_ir_builder.CreateICmpUGE( // if( index >= array_size ) {halt;}
					index_value,
					llvm::Constant::getIntegerValue( size_type_.GetLLVMType(), llvm::APInt( size_type_.GetLLVMType()->getIntegerBitWidth(), array_type->element_count ) ) );

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

		result->llvm_value= CreateArrayElementGEP( function_context, *variable, index_value );

		function_context.variables_state.AddNode( result );
		function_context.variables_state.TryAddLink( variable_lock, result, names.GetErrors(), indexation_operator.src_loc_ );

		function_context.variables_state.RemoveNode( variable_lock );

		RegisterTemporaryVariable( function_context, result );
		return Value( result, indexation_operator.src_loc_ );
	}
	else if( const TupleType* const tuple_type= variable->type.GetTupleType() )
	{
		VariablePtr index;
		// Create separate variables storage for index calculation.
		// This is needed to prevent destruction of "variable" during index calculation without creating lock node and reference to it.
		// This is needed to properly access multiple mutable child nodes of same tuple variable.
		{
			const StackVariablesStorage temp_variables_storage( function_context );
			index= BuildExpressionCodeEnsureVariable( *indexation_operator.index_, names, function_context );
			CallDestructors( temp_variables_storage, names, function_context, indexation_operator.src_loc_ );
			// It is fine if "index" will be destroyed here. We needed only "constexpr" value of index here.
		}

		const FundamentalType* const index_fundamental_type= index->type.GetFundamentalType();
		if( index_fundamental_type == nullptr || !IsInteger( index_fundamental_type->fundamental_type ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), indexation_operator.src_loc_, "any integer", index->type );
			return ErrorValue();
		}

		if( variable->location != Variable::Location::Pointer )
		{
			// TODO - Strange variable location.
			return ErrorValue();
		}

		// For tuple indexing only constexpr indeces are valid.
		if( index->constexpr_value == nullptr )
		{
			REPORT_ERROR( ExpectedConstantExpression, names.GetErrors(), indexation_operator.src_loc_ );
			return ErrorValue();
		}
		const llvm::APInt index_value_raw= index->constexpr_value->getUniqueInteger();
		const uint64_t index_value= index_value_raw.getLimitedValue();
		if( IsSignedInteger(index_fundamental_type->fundamental_type) )
		{
			if( index_value >= uint64_t(tuple_type->element_types.size()) || index_value_raw.isNegative() )
			{
				REPORT_ERROR( TupleIndexOutOfBounds, names.GetErrors(), indexation_operator.src_loc_, index_value_raw.getSExtValue(), tuple_type->element_types.size() );
				return ErrorValue();
			}
		}
		else
		{
			if( index_value >= uint64_t(tuple_type->element_types.size()) )
			{
				REPORT_ERROR( TupleIndexOutOfBounds, names.GetErrors(), indexation_operator.src_loc_, index_value, tuple_type->element_types.size() );
				return ErrorValue();
			}
		}

		variable->children.resize( tuple_type->llvm_type->getNumElements(), nullptr );
		if( const auto prev_node= variable->children[ index_value ] )
		{
			function_context.variables_state.AddNodeIfNotExists( prev_node );
			return Value( prev_node, indexation_operator.src_loc_ ); // Child node already created.
		}

		// Create variable child node.

		const VariableMutPtr result=
			std::make_shared<Variable>(
				tuple_type->element_types[size_t(index_value)],
				variable->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				variable->name + "[" + std::to_string(index_value) + "]",
				ForceCreateConstantIndexGEP( function_context, tuple_type->llvm_type, variable->llvm_value, uint32_t(index_value) ) );

		if( variable->constexpr_value != nullptr )
			result->constexpr_value= variable->constexpr_value->getAggregateElement( uint32_t(index_value) );

		variable->children[ size_t(index_value) ]= result;
		result->parent= variable;

		function_context.variables_state.AddNode( result );

		return Value( result, indexation_operator.src_loc_ );
	}
	else
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), indexation_operator.src_loc_, variable->type );
		return ErrorValue();
	}
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::MemberAccessOperator& member_access_operator )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( *member_access_operator.expression_, names, function_context );

	Class* const class_type= variable->type.GetClassType();
	if( class_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), member_access_operator.src_loc_, variable->type );
		return ErrorValue();
	}

	if( !EnsureTypeComplete( variable->type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), member_access_operator.src_loc_, variable->type );
		return ErrorValue();
	}

	const auto class_value= ResolveClassValue( class_type, member_access_operator.member_name_ );
	const Value* const class_member= class_value.first;
	if( class_member == nullptr )
	{
		REPORT_ERROR( NameNotFound, names.GetErrors(), member_access_operator.src_loc_, member_access_operator.member_name_ );
		return ErrorValue();
	}

	if( !function_context.is_in_unsafe_block &&
		( member_access_operator.member_name_ == Keywords::constructor_ || member_access_operator.member_name_ == Keywords::destructor_ ) )
		REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names.GetErrors(), member_access_operator.src_loc_,  member_access_operator.member_name_ );

	if( names.GetAccessFor( variable->type.GetClassType() ) < class_value.second )
		REPORT_ERROR( AccessingNonpublicClassMember, names.GetErrors(), member_access_operator.src_loc_, member_access_operator.member_name_, class_type->members->GetThisNamespaceName() );

	if( OverloadedFunctionsSetConstPtr functions_set= class_member->GetFunctionsSet() )
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
		this_overloaded_methods_set.overloaded_methods_set= functions_set;
		return std::move(this_overloaded_methods_set);
	}

	if( member_access_operator.template_parameters != std::nullopt )
		REPORT_ERROR( ValueIsNotTemplate, names.GetErrors(), member_access_operator.src_loc_ );

	if( const ClassField* const field= class_member->GetClassField() )
		return AccessClassField( names, function_context, variable, *field, member_access_operator.member_name_, member_access_operator.src_loc_ );

	REPORT_ERROR( NotImplemented, names.GetErrors(), member_access_operator.src_loc_, "class members, except fields or methods" );
	return ErrorValue();
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::UnaryMinus& unary_minus )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( *unary_minus.expression_, names, function_context );

	if( auto res= TryCallOverloadedUnaryOperator( variable, OverloadedOperator::Sub, unary_minus.src_loc_, names, function_context ) )
		return std::move(*res);

	const FundamentalType* const fundamental_type= variable->type.GetFundamentalType();
	if( fundamental_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), unary_minus.src_loc_, variable->type );
		return ErrorValue();
	}

	const bool is_float= IsFloatingPoint( fundamental_type->fundamental_type );
	if( !( IsInteger( fundamental_type->fundamental_type ) || is_float ) )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), unary_minus.src_loc_, variable->type );
		return ErrorValue();
	}
	// TODO - maybe not support unary minus for 8 and 16 bit integer types?

	const VariableMutPtr result=
		std::make_shared<Variable>(
			variable->type,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			OverloadedOperatorToString(OverloadedOperator::Sub) );

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
	return Value( result, unary_minus.src_loc_ );
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
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( *logical_not.expression_, names, function_context );

	if( auto res= TryCallOverloadedUnaryOperator( variable, OverloadedOperator::LogicalNot, logical_not.src_loc_, names, function_context ) )
		return std::move(*res);

	if( variable->type != bool_type_ )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), logical_not.src_loc_, variable->type );
		return ErrorValue();
	}

	const VariableMutPtr result=
		std::make_shared<Variable>(
			variable->type,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			OverloadedOperatorToString(OverloadedOperator::LogicalNot) );

	if( const auto src_val= CreateMoveToLLVMRegisterInstruction( *variable, function_context ) )
	{
		result->llvm_value= function_context.llvm_ir_builder.CreateNot( src_val );
		result->constexpr_value= llvm::dyn_cast<llvm::Constant>( result->llvm_value );
	}

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return Value( result, logical_not.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::BitwiseNot& bitwise_not )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( *bitwise_not.expression_, names, function_context );

	if( auto res= TryCallOverloadedUnaryOperator( variable, OverloadedOperator::BitwiseNot, bitwise_not.src_loc_, names, function_context ) )
		return std::move(*res);

	const FundamentalType* const fundamental_type= variable->type.GetFundamentalType();
	if( fundamental_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), bitwise_not.src_loc_, variable->type );
		return ErrorValue();
	}
	if( !IsInteger( fundamental_type->fundamental_type ) )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), bitwise_not.src_loc_, variable->type );
		return ErrorValue();
	}

	const VariableMutPtr result=
		std::make_shared<Variable>(
			variable->type,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			OverloadedOperatorToString(OverloadedOperator::BitwiseNot) );

	if( const auto src_val= CreateMoveToLLVMRegisterInstruction( *variable, function_context ) )
	{
		result->llvm_value= function_context.llvm_ir_builder.CreateNot( src_val );
		result->constexpr_value= llvm::dyn_cast<llvm::Constant>(result->llvm_value);
	}

	function_context.variables_state.AddNode( result);

	RegisterTemporaryVariable( function_context, result );
	return Value( result, bitwise_not.src_loc_ );
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

	const auto overloaded_operator= GetOverloadedOperatorForBinaryOperator( binary_operator.operator_type_ );

	std::optional<Value> overloaded_operator_call_try=
		TryCallOverloadedBinaryOperator(
			overloaded_operator,
			*binary_operator.left_, *binary_operator.right_,
			false,
			binary_operator.src_loc_,
			names,
			function_context );
	if( overloaded_operator_call_try != std::nullopt )
	{
		if( const VariablePtr call_variable= overloaded_operator_call_try->GetVariable())
		{
			if( binary_operator.operator_type_ == BinaryOperatorType::NotEqual && call_variable->type == bool_type_ )
			{
				const VariableMutPtr variable=
					std::make_shared<Variable>(
						bool_type_,
						ValueType::Value,
						Variable::Location::LLVMRegister,
						OverloadedOperatorToString( overloaded_operator ) );

				// "!=" is implemented via "==", so, invert result.
				if( const auto call_value_in_register= CreateMoveToLLVMRegisterInstruction( *call_variable, function_context ) )
				{
					variable->llvm_value= function_context.llvm_ir_builder.CreateNot( call_value_in_register );
					variable->constexpr_value= llvm::dyn_cast<llvm::Constant>( variable->llvm_value );
				}

				function_context.variables_state.AddNode( variable );
				RegisterTemporaryVariable( function_context, variable );
				return Value( variable, overloaded_operator_call_try->GetSrcLoc() );

			}
			else if( overloaded_operator == OverloadedOperator::CompareOrder &&
				binary_operator.operator_type_ != BinaryOperatorType::CompareOrder &&
				call_variable->type.GetFundamentalType() != nullptr &&
				IsSignedInteger( call_variable->type.GetFundamentalType()->fundamental_type ) )
			{
				const VariableMutPtr variable=
					std::make_shared<Variable>(
						bool_type_,
						ValueType::Value,
						Variable::Location::LLVMRegister,
						OverloadedOperatorToString( overloaded_operator ) );

				if( const auto call_value_in_register= CreateMoveToLLVMRegisterInstruction( *call_variable, function_context ) )
				{
					const auto zero= llvm::Constant::getNullValue( call_variable->type.GetLLVMType() );
					if( binary_operator.operator_type_ == BinaryOperatorType::Less )
						variable->llvm_value= function_context.llvm_ir_builder.CreateICmpSLT( call_value_in_register, zero );
					else if( binary_operator.operator_type_ == BinaryOperatorType::LessEqual )
						variable->llvm_value= function_context.llvm_ir_builder.CreateICmpSLE( call_value_in_register, zero );
					else if( binary_operator.operator_type_ == BinaryOperatorType::Greater )
						variable->llvm_value= function_context.llvm_ir_builder.CreateICmpSGT( call_value_in_register, zero );
					else if( binary_operator.operator_type_ == BinaryOperatorType::GreaterEqual )
						variable->llvm_value= function_context.llvm_ir_builder.CreateICmpSGE( call_value_in_register, zero );
					else U_ASSERT(false);

					variable->constexpr_value= llvm::dyn_cast<llvm::Constant>( variable->llvm_value );
				}

				function_context.variables_state.AddNode( variable );
				RegisterTemporaryVariable( function_context, variable );
				return Value( variable, overloaded_operator_call_try->GetSrcLoc() );
			}
		}

		return std::move(*overloaded_operator_call_try);
	}

	VariablePtr l_var=
		BuildExpressionCodeEnsureVariable(
			*binary_operator.left_,
			names,
			function_context );

	if( l_var->type.GetFundamentalType() != nullptr ||
		l_var->type.GetEnumType() != nullptr ||
		l_var->type.GetRawPointerType() != nullptr ||
		l_var->type.GetFunctionPointerType() != nullptr )
	{
		// Save l_var in register, because build-in binary operators require value-parameters.
		l_var=
			std::make_shared<Variable>(
				l_var->type,
				ValueType::Value,
				Variable::Location::LLVMRegister,
				l_var->name + " in register",
				l_var->location == Variable::Location::LLVMRegister
					? l_var->llvm_value
					: CreateMoveToLLVMRegisterInstruction( *l_var, function_context ),
				l_var->constexpr_value );

		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), binary_operator.src_loc_ );

		function_context.variables_state.AddNode( l_var );
		RegisterTemporaryVariable( function_context, l_var );
	}
	else
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), binary_operator.src_loc_ );

	const VariablePtr r_var=
		BuildExpressionCodeEnsureVariable(
			*binary_operator.right_,
			names,
			function_context );

	return BuildBinaryOperator( *l_var, *r_var, binary_operator.operator_type_, binary_operator.src_loc_, names, function_context );
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
			return Value( function_context.this_, named_operand.src_loc_ );
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

			// Do not call here "AccessClassBase" method.
			// We can not create child node for "this", because it's still possible to access whole "this" using "base" by calling a virtual method.
			// So, create regular node pointing to "this".
			// TODO - maybe access "base" child node in constructor initializer list since it is not possible to call real virtual method of "this"?
			const VariablePtr base=
				std::make_shared<Variable>(
					class_.base_class,
					function_context.this_->value_type,
					Variable::Location::Pointer,
					Keyword( Keywords::base_ ),
					CreateReferenceCast( function_context.this_->llvm_value, function_context.this_->type, class_.base_class, function_context ) );

			function_context.variables_state.AddNode( base );
			function_context.variables_state.TryAddLink( function_context.this_, base, names.GetErrors(), named_operand.src_loc_ );

			RegisterTemporaryVariable( function_context, base );

			return Value( base, named_operand.src_loc_ );
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

		if( function_context.whole_this_is_unavailable )
		{
			if( class_ == function_context.this_->type.GetClassType() )
			{
				if( field->index < function_context.initialized_this_fields.size() &&
					!function_context.initialized_this_fields[ field->index ] )
				{
					REPORT_ERROR( FieldIsNotInitializedYet, names.GetErrors(), named_operand.src_loc_, field->syntax_element->name );
					return ErrorValue();
				}
			}
			else
			{
				if(!function_context.base_initialized )
				{
					REPORT_ERROR( FieldIsNotInitializedYet, names.GetErrors(), named_operand.src_loc_, Keyword( Keywords::base_ ) );
					return ErrorValue();
				}
			}
		}

		const std::string* field_name= nullptr;
		if( const auto start_string= std::get_if<std::string>( &named_operand.start_value ) )
			field_name= start_string;

		const Synt::ComplexName::Component* last_component= named_operand.tail.get();
		while( last_component != nullptr )
		{
			if( const auto component_name= std::get_if<std::string>( &last_component->name_or_template_paramenters ) )
				field_name= component_name;
			last_component= last_component->next.get();
		}

		return AccessClassField( names, function_context, function_context.this_, *field, field_name == nullptr ? "" : *field_name, named_operand.src_loc_ );
	}
	else if( const OverloadedFunctionsSetConstPtr overloaded_functions_set= value_entry.GetFunctionsSet() )
	{
		if( function_context.this_ != nullptr )
		{
			// Trying add "this" to functions set, but only if whole "this" is available.
			if( ( function_context.this_->type.GetClassType() == overloaded_functions_set->base_class ||
				  function_context.this_->type.GetClassType()->HaveAncestor( overloaded_functions_set->base_class ) ) &&
				!function_context.whole_this_is_unavailable )
			{
				ThisOverloadedMethodsSet this_overloaded_methods_set;
				this_overloaded_methods_set.this_= function_context.this_;
				this_overloaded_methods_set.overloaded_methods_set= overloaded_functions_set;
				return std::move(this_overloaded_methods_set);
			}
		}
	}
	else if( const VariablePtr variable= value_entry.GetVariable() )
	{
		if( function_context.variables_state.NodeMoved( variable ) )
			REPORT_ERROR( AccessingMovedVariable, names.GetErrors(), named_operand.src_loc_, variable->name );

		// Forbid mutable global variables access outside unsafe block.
		// Detect global variable by checking dynamic type of variable's LLVM value.
		// TODO - what if variable is constant GEP result with global variable base?
		if( variable->value_type == ValueType::ReferenceMut &&
			llvm::dyn_cast<llvm::GlobalVariable>( variable->llvm_value ) != nullptr &&
			!function_context.is_in_unsafe_block )
			REPORT_ERROR( GlobalMutableVariableAccessOutsideUnsafeBlock, names.GetErrors(), named_operand.src_loc_ );

		if( IsGlobalVariable(variable) )
		{
			// Add global variable nodes lazily.
			function_context.variables_state.AddNodeIfNotExists( variable );
		}
	}

	return value_entry;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::TernaryOperator& ternary_operator )
{
	const VariablePtr condition= BuildExpressionCodeEnsureVariable( *ternary_operator.condition, names, function_context );
	if( condition->type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), ternary_operator.src_loc_, bool_type_, condition->type );
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
				const VariablePtr var= BuildExpressionCodeEnsureVariable( i == 0u ? *ternary_operator.true_branch : *ternary_operator.false_branch, names, function_context );
				branches_types[i]= var->type;
				branches_value_types[i]= var->value_type;
				DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), ternary_operator.src_loc_ );
			}
			RestoreFunctionContextState( function_context, state );
		}
		function_context.is_functionless_context= prev_is_functionless_context;
	}

	if( branches_types[0] != branches_types[1] )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), ternary_operator.src_loc_, branches_types[0], branches_types[1] );
		return ErrorValue();
	}

	const VariableMutPtr result= std::make_shared<Variable>();
	result->type= branches_types[0];
	result->location= Variable::Location::Pointer;
	result->name= Keyword( Keywords::select_ );
	if( branches_value_types[0] == ValueType::Value || branches_value_types[1] == ValueType::Value )
	{
		result->value_type= ValueType::Value;
		if( !EnsureTypeComplete( result->type ) )
		{
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), ternary_operator.src_loc_, result->type );
			return ErrorValue();
		}

		if( !function_context.is_functionless_context )
		{
			result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( result->type.GetLLVMType() );
			result->llvm_value->setName( "select_result" );

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
				function_context.function->getBasicBlockList().push_back( branches_basic_blocks[i] );
				function_context.llvm_ir_builder.SetInsertPoint( branches_basic_blocks[i] );
			}

			const Synt::Expression& branch_expr= i == 0u ? *ternary_operator.true_branch : *ternary_operator.false_branch;

			const VariablePtr branch_result= BuildExpressionCodeEnsureVariable( branch_expr, names, function_context );

			branches_constexpr_values[i]= branch_result->constexpr_value;
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
					SetupReferencesInCopyOrMove( function_context, result, branch_result, names.GetErrors(), ternary_operator.src_loc_ );

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
							REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), ternary_operator.src_loc_, result->type );
						else if( result->type.IsAbstract() )
							REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), ternary_operator.src_loc_, result->type );
						else if( !function_context.is_functionless_context )
							BuildCopyConstructorPart( result->llvm_value, branch_result->llvm_value, result->type, function_context );
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
				branches_reference_values[i]= branch_result->llvm_value;

				function_context.variables_state.TryAddLink( branch_result, result, names.GetErrors(), ternary_operator.src_loc_ );
			}

			CallDestructors( branch_temp_variables_storage, names, function_context, Synt::GetExpressionSrcLoc( branch_expr ) );

			if( !function_context.is_functionless_context )
				function_context.llvm_ir_builder.CreateBr( result_block );
		}
		branches_end_basic_blocks[i]= function_context.llvm_ir_builder.GetInsertBlock();
		branches_variables_state[i]= function_context.variables_state;
	}

	function_context.variables_state= MergeVariablesStateAfterIf( branches_variables_state, names.GetErrors(), ternary_operator.src_loc_ );

	if( !function_context.is_functionless_context )
	{
		function_context.function->getBasicBlockList().push_back( result_block );
		function_context.llvm_ir_builder.SetInsertPoint( result_block );

		if( result->value_type != ValueType::Value )
		{
			llvm::PHINode* const phi= function_context.llvm_ir_builder.CreatePHI( result->type.GetLLVMType()->getPointerTo(), 2u );
			phi->addIncoming( branches_reference_values[0], branches_end_basic_blocks[0] );
			phi->addIncoming( branches_reference_values[1], branches_end_basic_blocks[1] );
			result->llvm_value= phi;
		}
	}

	if( condition->constexpr_value != nullptr )
		result->constexpr_value= condition->constexpr_value->getUniqueInteger().getLimitedValue() != 0u ? branches_constexpr_values[0] : branches_constexpr_values[1];

	RegisterTemporaryVariable( function_context, result );
	return Value( result, ternary_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::ReferenceToRawPointerOperator& reference_to_raw_pointer_operator )
{
	const VariablePtr v= BuildExpressionCodeEnsureVariable( *reference_to_raw_pointer_operator.expression, names, function_context );
	if( v->value_type == ValueType::Value )
	{
		REPORT_ERROR( ValueIsNotReference, names.GetErrors(), reference_to_raw_pointer_operator.src_loc_ );
		return ErrorValue();
	}
	if( v->value_type == ValueType::ReferenceImut )
	{
		// Disable immutable reference to pointer conversion, because pointer dereference produces mutable value.
		REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), reference_to_raw_pointer_operator.src_loc_ );
		return ErrorValue();
	}

	U_ASSERT( v->location == Variable::Location::Pointer );

	// Reference to pointer conversion can break functional purity, so, disable such conversions in constexpr functions.
	function_context.have_non_constexpr_operations_inside= true;

	RawPointerType raw_pointer_type;
	raw_pointer_type.element_type= v->type;
	raw_pointer_type.llvm_type= llvm::PointerType::get( v->type.GetLLVMType(), 0u );

	const VariablePtr result=
		std::make_shared<Variable>(
			std::move(raw_pointer_type),
			ValueType::Value,
			Variable::Location::LLVMRegister,
			"ptr",
			v->llvm_value );

	function_context.variables_state.AddNode( result );
	RegisterTemporaryVariable( function_context, result );

	return Value( result, reference_to_raw_pointer_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::RawPointerToReferenceOperator& raw_pointer_to_reference_operator )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( RawPointerToReferenceConversionOutsideUnsafeBlock, names.GetErrors(), raw_pointer_to_reference_operator.src_loc_ );

	const VariablePtr v= BuildExpressionCodeEnsureVariable( *raw_pointer_to_reference_operator.expression, names, function_context );
	const RawPointerType* const raw_pointer_type= v->type.GetRawPointerType();

	if( raw_pointer_type == nullptr )
	{
		REPORT_ERROR( ValueIsNotPointer, names.GetErrors(), raw_pointer_to_reference_operator.src_loc_, v->type );
		return ErrorValue();
	}

	// Create mutable reference node without any links. TODO - check if this is correct.
	const VariablePtr result=
		std::make_shared<Variable>(
			raw_pointer_type->element_type,
			ValueType::ReferenceMut,
			Variable::Location::Pointer,
			"$>(" + v->name + ")",
			CreateMoveToLLVMRegisterInstruction( *v, function_context ) );

	function_context.variables_state.AddNode( result );
	RegisterTemporaryVariable( function_context, result );

	return Value( result, raw_pointer_to_reference_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::NumericConstant& numeric_constant )
{
	U_FundamentalType type= U_FundamentalType::InvalidType;
	const std::string type_suffix= numeric_constant.type_suffix.data();

	if( type_suffix.empty() )
		type= numeric_constant.has_fractional_point ? U_FundamentalType::f64_ : U_FundamentalType::i32_;
	else if( type_suffix == "u" )
		type= U_FundamentalType::u32_;
	// Suffix for size_type
	else if( type_suffix == "s" )
		type= size_type_.GetFundamentalType()->fundamental_type;
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
		REPORT_ERROR( UnknownNumericConstantType, names.GetErrors(), numeric_constant.src_loc_, numeric_constant.type_suffix.data() );
		return ErrorValue();
	}
	llvm::Type* const llvm_type= GetFundamentalLLVMType( type );

	const VariableMutPtr result=
		std::make_shared<Variable>(
			FundamentalType( type, llvm_type ),
			ValueType::Value,
			Variable::Location::LLVMRegister,
			"numeric constant " + std::to_string(numeric_constant.value_double) );

	if( IsInteger( type ) || IsChar( type ) )
		result->constexpr_value=
			llvm::Constant::getIntegerValue( llvm_type, llvm::APInt( llvm_type->getIntegerBitWidth(), numeric_constant.value_int ) );
	else if( IsFloatingPoint( type ) )
		result->constexpr_value=
			llvm::ConstantFP::get( llvm_type, numeric_constant.value_double );
	else
		U_ASSERT(false);

	result->llvm_value= result->constexpr_value;

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return Value( result, numeric_constant.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::BooleanConstant& boolean_constant )
{
	U_UNUSED(names);
	
	const VariableMutPtr result=
		std::make_shared<Variable>(
			bool_type_,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			Keyword( boolean_constant.value_ ? Keywords::true_ : Keywords::false_ ) );

	result->llvm_value= result->constexpr_value= llvm::ConstantInt::getBool( llvm_context_, boolean_constant.value_ );

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return Value( result, boolean_constant.src_loc_ );
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
		char_type= U_FundamentalType::char8_;
		array_size= string_literal.value_.size();
		initializer= llvm::ConstantDataArray::getString( llvm_context_, string_literal.value_, false /* not null terminated */ );
	}
	else if( type_suffix == "u16" )
	{
		llvm::SmallVector<llvm::UTF16, 32> str;
		llvm::convertUTF8ToUTF16String( string_literal.value_, str );

		char_type= U_FundamentalType::char16_;
		array_size= str.size();
		initializer= llvm::ConstantDataArray::get( llvm_context_, str );
	}
	else if( type_suffix == "u32" )
	{
		std::vector<uint32_t> str;
		for( auto it = string_literal.value_.data(), it_end= it + string_literal.value_.size(); it < it_end; )
			str.push_back( ReadNextUTF8Char( it, it_end ) );

		char_type= U_FundamentalType::char32_;
		array_size= str.size();
		initializer= llvm::ConstantDataArray::get( llvm_context_, str );
	}
	// If string literal have char suffix, process it as single char literal.
	else if( type_suffix == "c8" || type_suffix == GetFundamentalTypeName( U_FundamentalType::char8_  ) )
	{
		if( string_literal.value_.size() == 1u )
		{
			char_type= U_FundamentalType::char8_ ;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char8_ , uint64_t(string_literal.value_[0]), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names.GetErrors(), string_literal.src_loc_, string_literal.value_ );
	}
	else if( type_suffix == "c16" || type_suffix == GetFundamentalTypeName( U_FundamentalType::char16_ ) )
	{
		const char* it_start= string_literal.value_.data();
		const char* const it_end= it_start + string_literal.value_.size();
		const sprache_char c= ReadNextUTF8Char( it_start, it_end );
		if( it_start == it_end && c <= 65535u )
		{
			char_type= U_FundamentalType::char16_;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char16_, uint64_t(c), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names.GetErrors(), string_literal.src_loc_, string_literal.value_ );
	}
	else if( type_suffix == "c32" || type_suffix== GetFundamentalTypeName( U_FundamentalType::char32_ ) )
	{
		const char* it_start= string_literal.value_.data();
		const char* const it_end= it_start + string_literal.value_.size() ;
		const sprache_char c= ReadNextUTF8Char( it_start, it_end );
		if( it_start == it_end )
		{
			char_type= U_FundamentalType::char32_;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char32_, uint64_t(c), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names.GetErrors(), string_literal.src_loc_, string_literal.value_ );
	}
	else
		REPORT_ERROR( UnknownStringLiteralSuffix, names.GetErrors(), string_literal.src_loc_, type_suffix );

	if( initializer == nullptr )
		return ErrorValue();

	const VariableMutPtr result= std::make_shared<Variable>();
	result->constexpr_value= initializer;
	if( array_size == ~0u )
	{
		result->type= FundamentalType( char_type, GetFundamentalLLVMType( char_type ) );

		result->value_type= ValueType::Value;
		result->location= Variable::Location::LLVMRegister;
		result->llvm_value= initializer;
	}
	else
	{
		ArrayType array_type;
		array_type.element_type= FundamentalType( char_type, GetFundamentalLLVMType( char_type ) );
		array_type.element_count= array_size;
		array_type.llvm_type= llvm::ArrayType::get( GetFundamentalLLVMType( char_type ), array_size );
		result->type= std::move(array_type);

		result->value_type= ValueType::ReferenceImut;
		result->location= Variable::Location::Pointer;

		// Use md5 for string literal names.
		llvm::MD5 md5;
		if( const auto constant_data_array = llvm::dyn_cast<llvm::ConstantDataArray>(initializer) )
			md5.update( constant_data_array->getRawDataValues() );
		else if( llvm::dyn_cast<llvm::ConstantAggregateZero>(initializer) != nullptr )
			md5.update( std::string( size_t(array_size * FundamentalType( char_type ).GetSize()), '\0' ) );
		md5.update( llvm::ArrayRef<uint8_t>( reinterpret_cast<const uint8_t*>(&char_type), sizeof(U_FundamentalType) ) ); // Add type to hash for distinction of zero-sized strings with different types.
		llvm::MD5::MD5Result md5_result;
		md5.final(md5_result);
		const std::string literal_name= ( "_string_literal_" + md5_result.digest() ).str();

		result->llvm_value= CreateGlobalConstantVariable( result->type, literal_name, result->constexpr_value );
	}

	function_context.variables_state.AddNode( result );
	RegisterTemporaryVariable( function_context, result );

	return Value( result, string_literal.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::MoveOperator& move_operator	)
{
	Synt::ComplexName complex_name(move_operator.src_loc_);
	complex_name.start_value= move_operator.var_name_;

	const Value resolved_value= ResolveValue( names, function_context, complex_name );
	const VariablePtr resolved_variable= resolved_value.GetVariable();

	// "resolved_variable" should be mutable reference node pointing to single variable node.

	if( resolved_variable == nullptr || IsGlobalVariable( resolved_variable ) )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), move_operator.src_loc_, resolved_value.GetKindName() );
		return ErrorValue();
	}
	if( resolved_variable->value_type != ValueType::ReferenceMut )
	{
		REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), move_operator.src_loc_ );
		return ErrorValue();
	}

	if( function_context.variables_state.HaveOutgoingLinks( resolved_variable ) )
	{
		REPORT_ERROR( MovedVariableHaveReferences, names.GetErrors(), move_operator.src_loc_, resolved_variable->name );
		return ErrorValue();
	}

	if( function_context.variables_state.NodeMoved( resolved_variable ) )
	{
		REPORT_ERROR( AccessingMovedVariable, names.GetErrors(), move_operator.src_loc_, resolved_variable->name );
		return ErrorValue();
	}

	const auto input_nodes= function_context.variables_state.GetNodeInputLinks( resolved_variable );
	if( input_nodes.size() != 1u )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), move_operator.src_loc_, resolved_value.GetKindName() );
		return ErrorValue();
	}

	const VariablePtr variable_for_move= *input_nodes.begin();
	if( variable_for_move->value_type != ValueType::Value )
	{
		// This is not a variable, but some reference.
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), move_operator.src_loc_, resolved_value.GetKindName() );
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
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), move_operator.src_loc_, resolved_value.GetKindName() );
		return ErrorValue();
	}

	U_ASSERT( !function_context.variables_state.NodeMoved( variable_for_move ) );

	const VariablePtr result=
		std::make_shared<Variable>(
			variable_for_move->type,
			ValueType::Value,
			variable_for_move->location,
			"_moved_" + variable_for_move->name,
			variable_for_move->llvm_value,
			variable_for_move->constexpr_value );
	function_context.variables_state.AddNode( result );

	// We must save inner references of moved variable.
	// TODO - maybe reset inner node of moved variable?
	if( const auto move_variable_inner_node= function_context.variables_state.GetNodeInnerReference( variable_for_move ) )
	{
		const auto inner_node= function_context.variables_state.CreateNodeInnerReference( result, move_variable_inner_node->value_type );
		function_context.variables_state.AddLink( move_variable_inner_node, inner_node );
	}

	// Move both reference node and variable node.
	function_context.variables_state.MoveNode( resolved_variable );
	function_context.variables_state.MoveNode( variable_for_move );

	RegisterTemporaryVariable( function_context, result );
	return Value( result, move_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::TakeOperator& take_operator	)
{
	const VariablePtr expression_result= BuildExpressionCodeEnsureVariable( *take_operator.expression_, names, function_context );
	if( expression_result->value_type == ValueType::Value ) // If it is value - just pass it.
		return Value( expression_result, take_operator.src_loc_ );

	if( expression_result->value_type != ValueType::ReferenceMut )
	{
		REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), take_operator.src_loc_ );
		return ErrorValue();
	}
	if( function_context.variables_state.HaveOutgoingLinks( expression_result ) )
	{
		REPORT_ERROR( MovedVariableHaveReferences, names.GetErrors(), take_operator.src_loc_, expression_result->name );
		return ErrorValue();
	}
	if( expression_result->type.IsAbstract() )
	{
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), take_operator.src_loc_, expression_result->type );
		return ErrorValue();
	}
	if( const auto class_type= expression_result->type.GetClassType() )
	{
		// Do not allow taking values of polymorph non-final classes to avoid calling default constructor of base class in place of derived class.
		// It may break derived class invariants and will overwrite virtual table pointer.
		if( class_type->kind == Class::Kind::Interface || class_type->kind == Class::Kind::Abstract || class_type->kind == Class::Kind::PolymorphNonFinal )
		{
			REPORT_ERROR( TakeForNonFinalPolymorphClass, names.GetErrors(), take_operator.src_loc_, expression_result->type );
			return ErrorValue();
		}
	}

	// Allocate variable for result.
	const VariableMutPtr result=
		std::make_shared<Variable>(
			expression_result->type,
			ValueType::Value,
			Variable::Location::Pointer,
			"_moved_" + expression_result->name );

	// Copy content to new variable.
	function_context.variables_state.AddNode( result );
	SetupReferencesInCopyOrMove( function_context, result, expression_result, names.GetErrors(), take_operator.src_loc_ );

	if( !function_context.is_functionless_context )
	{
		result->llvm_value= function_context.alloca_ir_builder.CreateAlloca( result->type.GetLLVMType() );
		result->llvm_value->setName( result->name );

		CreateLifetimeStart( function_context, result->llvm_value );

		// Copy content to new variable.
		CopyBytes( result->llvm_value, expression_result->llvm_value, result->type, function_context );
	}

	// Construct empty value in old place.
	ApplyEmptyInitializer( expression_result->name, take_operator.src_loc_, expression_result, names, function_context );

	RegisterTemporaryVariable( function_context, result );
	return Value( result, take_operator.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::CastMut& cast_mut )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( MutableReferenceCastOutsideUnsafeBlock, names.GetErrors(), cast_mut.src_loc_ );

	const VariablePtr var= BuildExpressionCodeEnsureVariable( *cast_mut.expression_, names, function_context );

	const VariableMutPtr result=
		std::make_shared<Variable>(
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
	function_context.variables_state.TryAddLink( var, result, names.GetErrors(), cast_mut.src_loc_ );

	RegisterTemporaryVariable( function_context, result );
	return Value( result, cast_mut.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::CastImut& cast_imut	)
{
	const VariablePtr var= BuildExpressionCodeEnsureVariable( *cast_imut.expression_, names, function_context );

	const VariableMutPtr result=
		std::make_shared<Variable>(
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
	function_context.variables_state.TryAddLink( var, result, names.GetErrors(), cast_imut.src_loc_ );

	RegisterTemporaryVariable( function_context, result );
	return Value( result, cast_imut.src_loc_ );
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

	const VariableMutPtr& var_ptr= typeinfo_cache_[type];
	BuildFullTypeinfo( type, var_ptr, root_namespace );

	function_context.variables_state.AddNodeIfNotExists( var_ptr );

	return Value( var_ptr, typeinfo.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::NonSyncExpression& non_sync_expression )
{
	const Type type= PrepareType( *non_sync_expression.type_, names, function_context );
	const bool is_non_sync= GetTypeNonSync( type, names, non_sync_expression.src_loc_ );

	const VariableMutPtr result=
		std::make_shared<Variable>(
			bool_type_,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			Keyword( Keywords::non_sync_ ) );

	result->llvm_value= result->constexpr_value= llvm::ConstantInt::getBool( llvm_context_, is_non_sync );

	function_context.variables_state.AddNode( result);

	RegisterTemporaryVariable( function_context, result );
	return Value( result, non_sync_expression.src_loc_ );
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::SafeExpression& safe_expression )
{
	const bool prev_unsafe= function_context.is_in_unsafe_block;
	function_context.is_in_unsafe_block= false;
	Value result= BuildExpressionCode( *safe_expression.expression_, names, function_context );
	function_context.is_in_unsafe_block= prev_unsafe;
	return result;
}

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::UnsafeExpression& unsafe_expression )
{
	if( function_context.function == global_function_context_->function )
		REPORT_ERROR( UnsafeExpressionInGlobalContext, names.GetErrors(), unsafe_expression.src_loc_ );

	// "unsafe" expression usage should prevent function to be "constexpr".
	function_context.have_non_constexpr_operations_inside= true;

	const bool prev_unsafe= function_context.is_in_unsafe_block;
	function_context.is_in_unsafe_block= true;
	Value result= BuildExpressionCode( *unsafe_expression.expression_, names, function_context );
	function_context.is_in_unsafe_block= prev_unsafe;

	// Avoid passing constexpr values trough unsafe expression.
	// TODO - do we really needs this?
	if( const VariablePtr variable_ptr= result.GetVariable() )
	{
		if( variable_ptr->constexpr_value != nullptr )
		{
			const VariablePtr variable_copy=
				std::make_shared<Variable>(
				variable_ptr->type,
				variable_ptr->value_type,
				variable_ptr->location,
				"unsafe(" + variable_ptr->name + ")",
				variable_ptr->llvm_value,
				nullptr );

			function_context.variables_state.AddNode( variable_copy );

			if( variable_ptr->value_type == ValueType::Value )
			{
				if( const auto variable_inner_node= function_context.variables_state.GetNodeInnerReference( variable_ptr ) )
				{
					const auto inner_node= function_context.variables_state.CreateNodeInnerReference( variable_copy, variable_ptr->value_type );
					function_context.variables_state.AddLink( variable_inner_node, inner_node );
				}
			}
			else
				function_context.variables_state.TryAddLink( variable_ptr, variable_copy, names.GetErrors(), unsafe_expression.src_loc_ );

			function_context.variables_state.MoveNode( variable_ptr );

			RegisterTemporaryVariable( function_context, variable_copy );

			return Value( variable_copy, result.GetSrcLoc() );
		}
	}

	return result;
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

Value CodeBuilder::BuildExpressionCodeImpl(
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::GeneratorTypePtr& type_name )
{
	return Value( PrepareTypeImpl( names, function_context, type_name ), type_name->src_loc_ );
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
		std::make_shared<Variable>(
			variabe_type_class->base_class,
			variable->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut,
			Variable::Location::Pointer,
			Keyword( Keywords::base_ ),
			ForceCreateConstantIndexGEP( function_context, variable->type.GetLLVMType(), variable->llvm_value, c_base_field_index ) );

	variable->children[ c_base_field_index ]= base;
	base->parent= variable;

	function_context.variables_state.AddNode( base );

	return base;
}

Value CodeBuilder::AccessClassField(
	NamesScope& names,
	FunctionContext& function_context,
	const VariablePtr& variable,
	const ClassField& field,
	const std::string& field_name,
	const SrcLoc& src_loc )
{
	const Class* const variabe_type_class= variable->type.GetClassType();
	U_ASSERT( variabe_type_class != nullptr );

	if( field.class_ != variable->type )
	{
		if( variabe_type_class->base_class != nullptr )
		{
			// Recursive try to access field in parent class.
			return
				AccessClassField(
					names,
					function_context,
					AccessClassBase( variable, function_context ),
					field,
					field_name,
					src_loc );
		}

		// No base - this is wrong field for this class.
		REPORT_ERROR( AccessOfNonThisClassField, names.GetErrors(), src_loc, field_name );
		return ErrorValue();
	}

	if( field.is_reference )
	{
		const VariableMutPtr result=
			std::make_shared<Variable>(
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
					if( field.class_->typeinfo_type != std::nullopt && field_name == "type_id" )
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
				// Reference is never null, so, mark result of reference field load with "nonnull" metadata.
				load_res->setMetadata( llvm::LLVMContext::MD_nonnull, llvm::MDNode::get( llvm_context_, llvm::None ) );
				result->llvm_value= load_res;
			}
		}

		function_context.variables_state.AddNode( result );
		for( const VariablePtr& inner_reference : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( variable ) )
			function_context.variables_state.TryAddLink( inner_reference, result, names.GetErrors(), src_loc );

		RegisterTemporaryVariable( function_context, result );
		return Value( result, src_loc );
	}
	else
	{
		variable->children.resize( variabe_type_class->llvm_type->getNumElements(), nullptr );
		if( const auto prev_node= variable->children[ field.index ] )
		{
			function_context.variables_state.AddNodeIfNotExists( prev_node );
			return Value( prev_node, src_loc ); // Child node already created.
		}

		// Create variable child node.

		const VariableMutPtr result=
			std::make_shared<Variable>(
				field.type,
				( variable->value_type == ValueType::ReferenceMut && field.is_mutable ) ? ValueType::ReferenceMut : ValueType::ReferenceImut,
				Variable::Location::Pointer,
				variable->name + "." + field_name,
				ForceCreateConstantIndexGEP( function_context, variable->type.GetLLVMType(), variable->llvm_value, field.index ) );

		if( variable->constexpr_value != nullptr )
			result->constexpr_value= variable->constexpr_value->getAggregateElement( field.index );

		variable->children[ field.index ]= result;
		result->parent= variable;

		function_context.variables_state.AddNode( result );

		return Value( result, src_loc );
	}
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
	{
		const bool prev_is_functionless_context= function_context.is_functionless_context;
		function_context.is_functionless_context= true;
		const auto state= SaveFunctionContextState( function_context );
		{
			const StackVariablesStorage dummy_stack_variables_storage( function_context );
			for( const Synt::Expression* const in_arg : { &left_expr, &right_expr } )
				args.push_back( PreEvaluateArg( *in_arg, names, function_context ) );
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
				REPORT_ERROR( MoveAssignForNonFinalPolymorphClass, names.GetErrors(), src_loc, args.front().type );
		}

		// Move here, instead of calling copy-assignment operator. Before moving we must also call destructor for destination.
		const VariablePtr r_var_real= BuildExpressionCode( right_expr, names, function_context ).GetVariable();

		const VariablePtr r_var_lock=
			std::make_shared<Variable>(
				r_var_real->type,
				ValueType::ReferenceMut,
				r_var_real->location,
				r_var_real->name + " lock",
				r_var_real->llvm_value );
		function_context.variables_state.AddNode( r_var_lock );
		function_context.variables_state.TryAddLink( r_var_real, r_var_lock, names.GetErrors(), src_loc );

		const VariablePtr l_var_real= BuildExpressionCode( left_expr, names, function_context ).GetVariable();

		SetupReferencesInCopyOrMove( function_context, l_var_real, r_var_lock, names.GetErrors(), src_loc );

		function_context.variables_state.RemoveNode( r_var_lock );
		function_context.variables_state.MoveNode( r_var_real );

		if( !function_context.is_functionless_context )
		{
			if( l_var_real->type.HaveDestructor() )
				CallDestructor( l_var_real->llvm_value, l_var_real->type, function_context, names.GetErrors(), src_loc );

			U_ASSERT( r_var_real->location == Variable::Location::Pointer );
			CopyBytes( l_var_real->llvm_value, r_var_real->llvm_value, l_var_real->type, function_context );
			CreateLifetimeEnd( function_context, r_var_real->llvm_value );
		}

		const VariablePtr move_result=
			std::make_shared<Variable>( void_type_, ValueType::Value, Variable::Location::LLVMRegister );
		return Value( move_result, src_loc );
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
				EnsureLLVMFunctionCreated( *overloaded_operator ),
				overloaded_operator->type,
				src_loc,
				nullptr,
				{ &left_expr, &right_expr },
				evaluate_args_in_reverse_order,
				names,
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
	NamesScope& names,
	FunctionContext& function_context )
{
	if( op == OverloadedOperator::Assign )
	{
		const VariablePtr r_var= BuildExpressionCodeEnsureVariable( right_expr, names, function_context );

		const VariablePtr r_var_lock=
			std::make_shared<Variable>(
				r_var->type,
				ValueType::ReferenceImut,
				r_var->location,
				r_var->name + " lock",
				r_var->llvm_value );
		function_context.variables_state.AddNode( r_var_lock );
		function_context.variables_state.TryAddLink( r_var, r_var_lock, names.GetErrors(), src_loc );

		const VariablePtr l_var= BuildExpressionCodeEnsureVariable( left_expr, names, function_context );
		if( function_context.variables_state.HaveOutgoingLinks( l_var ) )
			REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), src_loc, l_var->name );

		U_ASSERT( l_var->type == r_var->type ); // Checked before.
		if( !l_var->type.IsCopyAssignable() )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_var->type );
			function_context.variables_state.RemoveNode( r_var_lock );
			return ErrorValue();
		}
		if( l_var->value_type != ValueType::ReferenceMut )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), src_loc );
			function_context.variables_state.RemoveNode( r_var_lock );
			return ErrorValue();
		}

		SetupReferencesInCopyOrMove( function_context, l_var, r_var_lock, names.GetErrors(), src_loc );

		BuildCopyAssignmentOperatorPart(
			l_var->llvm_value, r_var->llvm_value,
			l_var->type,
			function_context );

		function_context.variables_state.RemoveNode( r_var_lock );

		const VariablePtr result=
			std::make_shared<Variable>( void_type_, ValueType::Value, Variable::Location::LLVMRegister );
		return Value( result, src_loc );
	}
	else if( op == OverloadedOperator::CompareEqual )
	{
		const VariablePtr l_var= BuildExpressionCodeEnsureVariable(  left_expr, names, function_context );
		const VariablePtr l_var_lock=
			std::make_shared<Variable>(
				l_var->type,
				ValueType::ReferenceImut,
				l_var->location,
				l_var->name + " lock",
				l_var->llvm_value );
		function_context.variables_state.AddNode( l_var_lock );
		function_context.variables_state.TryAddLink( l_var, l_var_lock, names.GetErrors(), src_loc );

		const VariablePtr r_var= BuildExpressionCodeEnsureVariable( right_expr, names, function_context );
		if( function_context.variables_state.HaveOutgoingMutableNodes( r_var ) )
			REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), src_loc, r_var->name );

		function_context.variables_state.RemoveNode( l_var_lock );

		U_ASSERT( r_var->type == l_var->type ); // Checked before.

		if( !l_var->type.IsEqualityComparable() )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_var->type );
			return ErrorValue();
		}

		U_ASSERT( l_var->location == Variable::Location::Pointer );
		U_ASSERT( r_var->location == Variable::Location::Pointer );

		const VariableMutPtr result=
			std::make_shared<Variable>(
				bool_type_,
				ValueType::Value,
				Variable::Location::LLVMRegister,
				OverloadedOperatorToString(op) );

		if( l_var->constexpr_value != nullptr && r_var->constexpr_value != nullptr )
			result->llvm_value= result->constexpr_value= ConstexprCompareEqual( l_var->constexpr_value, r_var->constexpr_value, l_var->type, names, src_loc );
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
				function_context.function->getBasicBlockList().push_back( false_basic_block );
				function_context.llvm_ir_builder.SetInsertPoint( false_basic_block );
				function_context.llvm_ir_builder.CreateBr( end_basic_block );

				// End basic block.
				function_context.function->getBasicBlockList().push_back( end_basic_block );
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
		return Value( result, src_loc );
	}
	else
	{
		const VariablePtr l_var= BuildExpressionCodeEnsureVariable( left_expr, names, function_context );
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_var->type );
		return ErrorValue();
	}
}

std::optional<Value> CodeBuilder::TryCallOverloadedUnaryOperator(
	const VariablePtr& variable,
	const OverloadedOperator op,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( variable->type.GetClassType() == nullptr )
		return std::nullopt;

	ArgsVector<FunctionType::Param> args;
	args.emplace_back();
	args.back().type= variable->type;
	args.back().value_type= variable->value_type;

	const FunctionVariable* const overloaded_operator= GetOverloadedOperator( args, op, names, src_loc );

	if( overloaded_operator == nullptr )
		return std::nullopt;

	if( !( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
		function_context.have_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

	const std::pair<VariablePtr, llvm::Value*> fetch_result=
		TryFetchVirtualFunction( variable, *overloaded_operator, function_context, names.GetErrors(), src_loc );

	return
		DoCallFunction(
			fetch_result.second,
			overloaded_operator->type,
			src_loc,
			fetch_result.first,
			{},
			false,
			names,
			function_context );
}

std::optional<Value> CodeBuilder::TryCallOverloadedPostfixOperator(
	const VariablePtr& variable,
	const llvm::ArrayRef<Synt::Expression>& synt_args,
	const OverloadedOperator op,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	ArgsVector<FunctionType::Param> actual_args;
	actual_args.reserve( 1 + synt_args.size() );

	{
		const bool prev_is_functionless_context= function_context.is_functionless_context;
		function_context.is_functionless_context= true;
		const auto state= SaveFunctionContextState( function_context );
		{
			const StackVariablesStorage dummy_stack_variables_storage( function_context );
			actual_args.push_back( GetArgExtendedType( *variable ) );
			for( const Synt::Expression& arg_expression : synt_args )
				actual_args.push_back( PreEvaluateArg( arg_expression, names, function_context ) );
		}

		RestoreFunctionContextState( function_context, state );
		function_context.is_functionless_context= prev_is_functionless_context;
	}

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
		function->type,
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

	const VariableMutPtr result= std::make_shared<Variable>();
	result->location= Variable::Location::LLVMRegister;
	result->value_type= ValueType::Value;
	result->name= BinaryOperatorToString(binary_operator);

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
				REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), src_loc, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
				return ErrorValue();
			}
			if( !( l_fundamental_type != nullptr || l_type.GetRawPointerType() != nullptr || l_type.GetEnumType() != nullptr ) )
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
			else if( const auto enum_type= l_type.GetEnumType() )
				is_signed= IsSignedInteger( enum_type->underlaying_type.fundamental_type );

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
				REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), src_loc, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
				return ErrorValue();
			}
			if( !( l_fundamental_type != nullptr || l_type.GetRawPointerType() != nullptr || l_type.GetEnumType() != nullptr ) )
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
			else if( const auto enum_type= l_type.GetEnumType() )
				is_signed= IsSignedInteger( enum_type->underlaying_type.fundamental_type );

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
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || l_fundamental_type->fundamental_type == U_FundamentalType::bool_ ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
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
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_type );
				return ErrorValue();
			}
			if( r_fundamental_type == nullptr || !IsUnsignedInteger( r_fundamental_type->fundamental_type ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, r_type );
				return ErrorValue();
			}

			if( l_value_for_op != nullptr && r_value_for_op != nullptr )
			{
				const uint64_t l_type_size= l_fundamental_type->GetSize();
				const uint64_t r_type_size= r_fundamental_type->GetSize();

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
			REPORT_ERROR( ConstantExpressionResultIsUndefined, names.GetErrors(), src_loc );
			result->constexpr_value= nullptr;
		}
	}

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return Value( result, src_loc );
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

	const VariableMutPtr result= std::make_shared<Variable>();
	result->location= Variable::Location::LLVMRegister;
	result->value_type= ValueType::Value;
	result->name= BinaryOperatorToString(binary_operator);

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
			result->type= r_var.type;
			ptr_value= r_value_for_op;
		}
		else if( const auto r_fundamental_type= r_var.type.GetFundamentalType() )
		{
			int_size= r_fundamental_type->GetSize();
			int_type= r_fundamental_type->fundamental_type;
			index_value= r_value_for_op;

			U_ASSERT( l_var.type.GetRawPointerType() != nullptr );
			result->type= l_var.type;
			ptr_value= l_value_for_op;
		}
		else
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, l_var.type );
			return ErrorValue();
		}

		const Type& element_type= result->type.GetRawPointerType()->element_type;
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

		if( !function_context.is_functionless_context )
		{
			if( int_size < ptr_size )
			{
				if( IsSignedInteger( int_type ) )
					index_value= function_context.llvm_ir_builder.CreateSExt( index_value, fundamental_llvm_types_.int_ptr );
				else
					index_value= function_context.llvm_ir_builder.CreateZExt( index_value, fundamental_llvm_types_.int_ptr );
			}
			result->llvm_value= function_context.llvm_ir_builder.CreateGEP( element_type.GetLLVMType(), ptr_value, index_value );
		}
	}
	else if( binary_operator == BinaryOperatorType::Sub )
	{
		const auto ptr_type= l_var.type.GetRawPointerType();
		if( ptr_type == nullptr )
		{
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), src_loc, l_var.type, r_var.type, BinaryOperatorToString( binary_operator ) );
			return ErrorValue();
		}

		if( !EnsureTypeComplete( ptr_type->element_type ) )
		{
			// Complete types required for pointer arithmetic.
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, ptr_type->element_type );
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

			const U_FundamentalType diff_type= fundamental_llvm_types_.int_ptr->getIntegerBitWidth() == 32u ? U_FundamentalType::i32_ : U_FundamentalType::i64_;
			llvm::Type* const diff_llvm_type= GetFundamentalLLVMType( diff_type );

			result->type= FundamentalType( diff_type, diff_llvm_type );

			const auto element_size= data_layout_.getTypeAllocSize( ptr_type->element_type.GetLLVMType() );
			if( element_size == 0 )
			{
				REPORT_ERROR( DifferenceBetweenRawPointersWithZeroElementSize, names.GetErrors(), src_loc, l_var.type );
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

			const uint64_t ptr_size= fundamental_llvm_types_.int_ptr->getIntegerBitWidth() / 8;
			const uint64_t int_size= r_fundamental_type->GetSize();

			if( !IsInteger( r_fundamental_type->fundamental_type ) || int_size > ptr_size )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, r_var.type );
				return ErrorValue();
			}

			result->type= l_var.type;

			if( !function_context.is_functionless_context )
			{
				llvm::Value* index_value= r_value_for_op;
				if( int_size < ptr_size )
				{
					if( IsSignedInteger( r_fundamental_type->fundamental_type ) )
						index_value= function_context.llvm_ir_builder.CreateSExt( index_value, fundamental_llvm_types_.int_ptr );
					else
						index_value= function_context.llvm_ir_builder.CreateZExt( index_value, fundamental_llvm_types_.int_ptr );
				}
				llvm::Value* const index_value_negative= function_context.llvm_ir_builder.CreateNeg( index_value );
				result->llvm_value= function_context.llvm_ir_builder.CreateGEP( ptr_type->element_type.GetLLVMType(), l_value_for_op, index_value_negative );
			}
		}
		else
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), src_loc, r_var.type );
			return ErrorValue();
		}
	}
	else{ U_ASSERT(false); }

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return Value( result, src_loc );
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
	const VariablePtr l_var= BuildExpressionCodeEnsureVariable( l_expression, names, function_context );

	if( l_var->type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), binary_operator.src_loc_, bool_type_, l_var->type );
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

		if( binary_operator.operator_type_ == BinaryOperatorType::LazyLogicalAnd )
			function_context.llvm_ir_builder.CreateCondBr( l_var_in_register, r_part_block, block_after_operator );
		else if( binary_operator.operator_type_ == BinaryOperatorType::LazyLogicalOr )
			function_context.llvm_ir_builder.CreateCondBr( l_var_in_register, block_after_operator, r_part_block );
		else U_ASSERT(false);

		function_context.function->getBasicBlockList().push_back( r_part_block );
		function_context.llvm_ir_builder.SetInsertPoint( r_part_block );
	}

	ReferencesGraph variables_state_before_r_branch= function_context.variables_state;

	llvm::Value* r_var_in_register= nullptr;
	llvm::Constant* r_var_constepxr_value= nullptr;
	{
		// Right part of lazy operator is conditinal. So, we must destroy its temporaries only in this condition.
		// We doesn`t needs longer lifetime of expression temporaries, because we use only bool result.
		const StackVariablesStorage r_var_temp_variables_storage( function_context );

		const VariablePtr r_var= BuildExpressionCodeEnsureVariable( r_expression, names, function_context );
		if( r_var->type != bool_type_ )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), binary_operator.src_loc_, bool_type_, r_var->type );
			return ErrorValue();
		}
		r_var_constepxr_value= r_var->constexpr_value;
		r_var_in_register= CreateMoveToLLVMRegisterInstruction( *r_var, function_context );

		// Destroy r_var temporaries in this branch.
		CallDestructors( r_var_temp_variables_storage, names, function_context, src_loc );
	}
	function_context.variables_state= MergeVariablesStateAfterIf( { variables_state_before_r_branch, function_context.variables_state }, names.GetErrors(), src_loc );

	const VariableMutPtr result=
		std::make_shared<Variable>(
			bool_type_,
			ValueType::Value,
			Variable::Location::LLVMRegister,
			BinaryOperatorToString(binary_operator.operator_type_) );

	if( !function_context.is_functionless_context )
	{
		llvm::BasicBlock* const r_part_end_block= function_context.llvm_ir_builder.GetInsertBlock();

		function_context.llvm_ir_builder.CreateBr( block_after_operator );
		function_context.function->getBasicBlockList().push_back( block_after_operator );
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
		if( binary_operator.operator_type_ == BinaryOperatorType::LazyLogicalAnd )
			result->constexpr_value= llvm::ConstantExpr::getAnd( l_var->constexpr_value, r_var_constepxr_value );
		else if( binary_operator.operator_type_ == BinaryOperatorType::LazyLogicalOr )
			result->constexpr_value= llvm::ConstantExpr::getOr ( l_var->constexpr_value, r_var_constepxr_value );
		else
			U_ASSERT(false);
	}

	function_context.variables_state.AddNode( result );

	RegisterTemporaryVariable( function_context, result );
	return Value( result, src_loc );
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

	const VariablePtr var= BuildExpressionCodeEnsureVariable( expression, names, function_context );

	const VariableMutPtr result=
		std::make_shared<Variable>(
			type,
			var->value_type == ValueType::ReferenceMut ? ValueType::ReferenceMut : ValueType::ReferenceImut, // "ValueType" here converts into ConstReference
			Variable::Location::Pointer,
			"cast</" + type.ToString() + "/>(" + var->name + ")" );
	function_context.variables_state.AddNode( result );
	function_context.variables_state.TryAddLink( var, result, names.GetErrors(), src_loc );

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
		// Complete types required for both safe and unsafe casting.
		// This needs, becasue we must emit same code for places where types yet not complete, and where they are complete.
		if( !EnsureTypeComplete( type ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, type );

		if( !EnsureTypeComplete( var->type ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, var->type );

		if( ReferenceIsConvertible( var->type, type, names.GetErrors(), src_loc ) )
		{
			if( !function_context.is_functionless_context )
				result->llvm_value= CreateReferenceCast( src_value, var->type, type, function_context );
		}
		else
		{
			if( !function_context.is_functionless_context )
				result->llvm_value= function_context.llvm_ir_builder.CreatePointerCast( src_value, type.GetLLVMType()->getPointerTo() );
			if( !enable_unsafe )
				REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, type, var->type );
		}
	}

	RegisterTemporaryVariable( function_context, result );

	return Value( result, src_loc );
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
			function_context.have_non_constexpr_operations_inside= true; // Calling function, using pointer, is not constexpr. We can not garantee, that called function is constexpr.

			// Call function pointer directly.
			if( function_pointer->function_type.params.size() != synt_args.size() )
			{
				REPORT_ERROR( InvalidFunctionArgumentCount, names.GetErrors(), src_loc, synt_args.size(), function_pointer->function_type.params.size() );
				return ErrorValue();
			}

			std::vector<const Synt::Expression*> args;
			args.reserve( synt_args.size() );
			for( const Synt::Expression& arg : synt_args )
				args.push_back( &arg );

			llvm::Value* const func_itself= CreateMoveToLLVMRegisterInstruction( *callable_variable, function_context );

			return
				DoCallFunction(
					func_itself, function_pointer->function_type, src_loc,
					nullptr, args, false,
					names, function_context );
		}

		// Try to call overloaded () operator.
		// DO NOT fill "this" here and continue this function because we should process callable object as non-this.

		if( auto res= TryCallOverloadedPostfixOperator( callable_variable, synt_args, OverloadedOperator::Call, src_loc, names, function_context ) )
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

		{
			const bool prev_is_functionless_context= function_context.is_functionless_context;
			function_context.is_functionless_context= true;
			const auto state= SaveFunctionContextState( function_context );
			{
				const StackVariablesStorage dummy_stack_variables_storage( function_context );

				if( this_ != nullptr )
					actual_args.push_back( GetArgExtendedType( *this_ ) );

				for( const Synt::Expression& arg_expression : synt_args )
					actual_args.push_back( PreEvaluateArg( arg_expression, names, function_context ) );
			}

			RestoreFunctionContextState( function_context, state );
			function_context.is_functionless_context= prev_is_functionless_context;
		}

		function_ptr=
			GetOverloadedFunction( *functions_set, actual_args, this_ != nullptr, names.GetErrors(), src_loc );
	}

	// SPRACHE_TODO - try get function with "this" parameter in signature and without it.
	// We must support static functions call using "this".
	if( function_ptr == nullptr )
		return ErrorValue();
	const FunctionVariable& function= *function_ptr;
	const FunctionType& function_type= function.type;

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

	llvm::Value* llvm_function_ptr= EnsureLLVMFunctionCreated( function );
	if( this_ != nullptr )
	{
		auto fetch_result= TryFetchVirtualFunction( this_, function, function_context, names.GetErrors(), src_loc );
		llvm_function_ptr= fetch_result.second;
		this_= fetch_result.first;
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
	const VariablePtr& this_, // optional
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
		this_ == nullptr ? llvm::ArrayRef<VariablePtr>() : llvm::ArrayRef<VariablePtr>( this_ ),
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
	const llvm::ArrayRef<VariablePtr>& preevaluated_args,
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

	// TODO - use vector of pairs instead.
	ArgsVector< VariablePtr > args_nodes;
	args_nodes.resize( arg_count, nullptr );
	ArgsVector< VariablePtr > locked_args_inner_references;
	locked_args_inner_references.resize( arg_count, nullptr );

	ArgsVector<llvm::Value*> value_args_for_lifetime_end_call;

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
			expr= BuildExpressionCodeEnsureVariable( *args[ arg_number - preevaluated_args.size() ], names, function_context );
			src_loc= Synt::GetExpressionSrcLoc( *args[ arg_number - preevaluated_args.size() ] );
		}

		if( param.value_type != ValueType::Value )
		{
			if( !ReferenceIsConvertible( expr->type, param.type, names.GetErrors(), call_src_loc ) &&
				GetConversionConstructor( expr->type, param.type, names.GetErrors(), src_loc ) == nullptr )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, param.type, expr->type );
				continue;
			}

			if( param.value_type == ValueType::ReferenceMut )
			{
				if( expr->value_type == ValueType::Value )
				{
					REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), src_loc );
					continue;
				}
				if( expr->value_type == ValueType::ReferenceImut )
				{
					REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), src_loc );
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
						const auto conversion_constructor= GetConversionConstructor( expr->type, param.type, names.GetErrors(), src_loc );
						U_ASSERT( conversion_constructor != nullptr );
						expr= ConvertVariable( expr, param.type, *conversion_constructor, names, function_context, src_loc );
						llvm_args[arg_number]= expr->llvm_value;
					}
				}
			}

			// Lock references.
			args_nodes[arg_number]=
				std::make_shared<Variable>(
				param.type,
				param.value_type,
				Variable::Location::Pointer,
				"reference_arg " + std::to_string(i) );
			function_context.variables_state.AddNode( args_nodes[arg_number] );
			function_context.variables_state.TryAddLink( expr, args_nodes[arg_number], names.GetErrors(), src_loc );

			// Lock inner references.
			const auto inner_references= function_context.variables_state.GetAccessibleVariableNodesInnerReferences( expr );
			if( !inner_references.empty() )
			{
				EnsureTypeComplete( param.type );
				if( param.type.ReferencesTagsCount() > 0 )
				{
					bool is_mutable= false;
					for( const VariablePtr& inner_reference : inner_references )
						is_mutable= is_mutable || inner_reference->value_type == ValueType::ReferenceMut;

					locked_args_inner_references[arg_number]=
						std::make_shared<Variable>(
							invalid_type_,
							is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut,
							Variable::Location::Pointer, // TODO - is this correct?
							"inner reference lock " + std::to_string(i) );

					function_context.variables_state.AddNode( locked_args_inner_references[arg_number] );

					for( const VariablePtr& inner_reference : inner_references )
						function_context.variables_state.TryAddLink( inner_reference, locked_args_inner_references[arg_number], names.GetErrors(), src_loc );
				}
			}
		}
		else
		{
			args_nodes[arg_number]=
				std::make_shared<Variable>(
					param.type,
					ValueType::Value,
					Variable::Location::Pointer, // TODO - is this correct?
					"value_arg_" + std::to_string(i) );

			function_context.variables_state.AddNode( args_nodes[arg_number] );

			if( !ReferenceIsConvertible( expr->type, param.type, names.GetErrors(), call_src_loc ) &&
				GetConversionConstructor( expr->type, param.type, names.GetErrors(), src_loc ) == nullptr )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, param.type, expr->type );
				continue;
			}

			if( expr->type != param.type )
			{
				if( expr->type.ReferenceIsConvertibleTo( param.type ) ){}
				else
				{
					const auto conversion_constructor= GetConversionConstructor( expr->type, param.type, names.GetErrors(), src_loc );
					U_ASSERT( conversion_constructor != nullptr );
					expr= ConvertVariable( expr, param.type, *conversion_constructor, names, function_context, src_loc );
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
			}
			else if( param.type.GetClassType() != nullptr || param.type.GetArrayType() != nullptr || param.type.GetTupleType() != nullptr )
			{
				// Lock inner references.
				// Do it only if arg type can contain any reference inside.
				// Do it before potential moving.
				EnsureTypeComplete( param.type ); // arg type for value arg must be already complete.
				if( param.type.ReferencesTagsCount() > 0u )
				{
					const auto inner_references= function_context.variables_state.GetAccessibleVariableNodesInnerReferences( expr );
					if( !inner_references.empty() )
					{
						bool is_mutable= false;
						for( const VariablePtr& inner_reference : inner_references )
							is_mutable= is_mutable || inner_reference->value_type == ValueType::ReferenceMut;

						const auto value_arg_inner_node=
							function_context.variables_state.CreateNodeInnerReference( args_nodes[arg_number], is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut );

						for( const VariablePtr& inner_reference : inner_references )
							function_context.variables_state.TryAddLink( inner_reference, value_arg_inner_node, names.GetErrors(), src_loc );
					}
				}

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

					if( single_scalar_type == nullptr )
					{
						llvm_args[arg_number]= expr->llvm_value;
						if( !function_context.is_functionless_context )
							value_args_for_lifetime_end_call.push_back( expr->llvm_value );
					}
					else
					{
						if( !function_context.is_functionless_context )
						{
							llvm::Value* const value= function_context.llvm_ir_builder.CreateLoad( single_scalar_type, expr->llvm_value );
							CreateLifetimeEnd( function_context, expr->llvm_value );
							llvm_args[arg_number]= value;
						}
					}
				}
				else
				{
					if( !param.type.IsCopyConstructible() )
					{
						// Can not call function with value parameter, because for value parameter needs copy, but parameter type is not copyable.
						REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), src_loc, param.type );
						continue;
					}
					// Allow value params of abstract types (it is useful in templates) but disallow call of such functions.
					if( param.type.IsAbstract() )
					{
						REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), src_loc, param.type );
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
							// Save address into temporary container to call lifetime.end after call.
							value_args_for_lifetime_end_call.push_back( arg_copy );
						}
						else
						{
							// If this is a single scalar type - just load value and end lifetime of address of copy.
							llvm::Value* const value= function_context.llvm_ir_builder.CreateLoad( single_scalar_type, arg_copy );
							CreateLifetimeEnd( function_context, arg_copy );
							llvm_args[arg_number]= value;
						}
					}
				}
			}
			else U_ASSERT( false );
		}

		// Destroy unused temporary variables after each argument evaluation.
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), call_src_loc );
	} // for args

	const bool return_value_is_composite= function_type.ReturnsCompositeValue();
	const bool return_value_is_sret= FunctionTypeIsSRet( function_type );

	const VariableMutPtr result= std::make_shared<Variable>();
	result->type= function_type.return_type;
	result->value_type= function_type.return_value_type;
	result->name= "fn_result " + result->type.ToString();
	if( function_type.return_value_type != ValueType::Value )
	{
		result->location= Variable::Location::Pointer;
	}
	else
	{
		if( !EnsureTypeComplete( function_type.return_type ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), call_src_loc, function_type.return_type );

		result->location= return_value_is_composite ? Variable::Location::Pointer : Variable::Location::LLVMRegister;
	}
	function_context.variables_state.AddNode( result );

	if( return_value_is_composite )
	{
		if( !EnsureTypeComplete( function_type.return_type ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), call_src_loc, function_type.return_type );

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
		function_type.return_value_type == ValueType::Value && function_type.return_type.ReferencesTagsCount() == 0u )
	{
		const Interpreter::ResultConstexpr evaluation_result=
			constexpr_function_evaluator_.EvaluateConstexpr( function_as_real_function, constant_llvm_args );

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
	}
	else if( !function_context.is_functionless_context && std::find( llvm_args.begin(), llvm_args.end(), nullptr ) == llvm_args.end() )
	{
		llvm::FunctionType* llvm_function_type= nullptr;
		if( const auto really_function= llvm::dyn_cast<llvm::Function>(function) )
			llvm_function_type= really_function->getFunctionType();
		else
			llvm_function_type= GetLLVMFunctionType( function_type );

		llvm::CallInst* const call_instruction= function_context.llvm_ir_builder.CreateCall( llvm_function_type, function, llvm_args );
		call_instruction->setCallingConv( function_type.calling_convention );

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

	// Clear inner references locks. Do this BEFORE result references management.
	for( const VariablePtr& node : locked_args_inner_references )
		function_context.variables_state.RemoveNode( node );
	locked_args_inner_references.clear();

	// Call "lifetime.end" just right after call for value args, allocated on stack of this function.
	// It is fine because there is no way to return reference to value arg (reference protection does not allow this).
	for( llvm::Value* const value_arg_var : value_args_for_lifetime_end_call )
		CreateLifetimeEnd( function_context, value_arg_var );

	// Prepare result references.
	if( function_type.return_value_type != ValueType::Value )
	{
		for( const FunctionType::ParamReference& arg_reference : function_type.return_references )
		{
			if( arg_reference.second == FunctionType::c_arg_reference_tag_number )
				function_context.variables_state.TryAddLink( args_nodes[arg_reference.first], result, names.GetErrors(), call_src_loc );
			else
				for( const VariablePtr& accesible_node : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( args_nodes[arg_reference.first] ) )
					function_context.variables_state.TryAddLink( accesible_node, result,  names.GetErrors(), call_src_loc );
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
				const auto node_kind= args_nodes[arg_reference.first]->value_type;

				if( node_kind == ValueType::Value || node_kind == ValueType::ReferenceMut )
					inner_reference_is_mutable= true;
				else if( node_kind == ValueType::ReferenceImut ) {}
				else U_ASSERT( false ); // Unexpected node kind.
			}
			else
			{
				for( const VariablePtr& accesible_node : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( args_nodes[arg_reference.first] ) )
				{
					if( accesible_node->value_type == ValueType::Value || accesible_node->value_type == ValueType::ReferenceMut )
						inner_reference_is_mutable= true;
					else if( accesible_node->value_type == ValueType::ReferenceImut ) {}
					else U_ASSERT( false ); // Unexpected node kind.
				}
			}
		}

		// Then, create inner node and link input nodes with it.
		const auto inner_reference_node=
			function_context.variables_state.CreateNodeInnerReference( result, inner_reference_is_mutable ? ValueType::ReferenceMut : ValueType::ReferenceImut );

		for( const FunctionType::ParamReference& arg_reference : function_type.return_references )
		{
			if( arg_reference.second == FunctionType::c_arg_reference_tag_number )
				function_context.variables_state.TryAddLink( args_nodes[arg_reference.first], inner_reference_node, names.GetErrors(), call_src_loc );
			else
				for( const VariablePtr& accesible_node : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( args_nodes[arg_reference.first] ) )
					function_context.variables_state.TryAddLink( accesible_node, inner_reference_node, names.GetErrors(), call_src_loc );
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
			U_ASSERT( function_type.params[ referene_pollution.src.first ].value_type != ValueType::Value );
			src_nodes.emplace( args_nodes[ referene_pollution.src.first ] );

			if( function_type.params[ referene_pollution.src.first ].value_type == ValueType::ReferenceMut )
				src_variables_is_mut= true;
		}
		else
		{
			// Variables, referenced by inner argument references.
			U_ASSERT( referene_pollution.src.second == 0u );// Currently we support one tag per struct.

			if( function_type.params[ referene_pollution.src.first ].type.ReferencesTagsCount() == 0 )
				continue;

			for( const VariablePtr& inner_reference : function_context.variables_state.GetAccessibleVariableNodesInnerReferences( args_nodes[ referene_pollution.src.first ] ) )
			{
				src_nodes.insert( inner_reference );
				if( inner_reference->value_type != ValueType::ReferenceImut )
					src_variables_is_mut= true;
			}
		}

		if( function_type.params[ dst_arg ].value_type != ValueType::Value && !src_nodes.empty() )
		{
			const bool dst_inner_reference_is_mut= function_type.params[ dst_arg ].type.GetInnerReferenceType() == InnerReferenceType::Mut;
			// Even if reference-pollution is mutable, but if src vars is immutable, link as immutable.
			const bool result_node_is_mut= src_variables_is_mut && dst_inner_reference_is_mut;

			for( const VariablePtr& dst_node : function_context.variables_state.GetAllAccessibleVariableNodes( args_nodes[ dst_arg ] ) )
			{
				VariablePtr inner_reference= function_context.variables_state.GetNodeInnerReference( dst_node );
				if( inner_reference == nullptr )
				{
					inner_reference=
						function_context.variables_state.CreateNodeInnerReference(
							dst_node,
							result_node_is_mut ? ValueType::ReferenceMut : ValueType::ReferenceImut );
				}
				if( ( inner_reference->value_type == ValueType::ReferenceMut  && !result_node_is_mut ) ||
					( inner_reference->value_type == ValueType::ReferenceImut &&  result_node_is_mut ))
					REPORT_ERROR( InnerReferenceMutabilityChanging, names.GetErrors(), call_src_loc, inner_reference->name );

				for( const VariablePtr& src_node : src_nodes )
					function_context.variables_state.TryAddLink( src_node, inner_reference, names.GetErrors(), call_src_loc );
			}
		}
		else
		{
			// Does it have sence, write references to value argument?
		}
	}

	for( const VariablePtr& node : args_nodes )
		function_context.variables_state.RemoveNode( node );
	args_nodes.clear();

	DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), call_src_loc );
	RegisterTemporaryVariable( function_context, result );

	return Value( result, call_src_loc );
}

VariablePtr CodeBuilder::BuildTempVariableConstruction(
	const Type& type,
	const std::vector<Synt::Expression>& synt_args,
	const SrcLoc& src_loc,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( !EnsureTypeComplete( type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, type );
		return nullptr;
	}
	else if( type.IsAbstract() )
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), src_loc, type );

	const VariableMutPtr variable=
		std::make_shared<Variable>(
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
			std::make_shared<Variable>(
				type,
				ValueType::ReferenceMut,
				Variable::Location::Pointer,
				variable->name,
				variable->llvm_value );
		function_context.variables_state.AddNode( variable_for_initialization );
		function_context.variables_state.AddLink( variable, variable_for_initialization );

		variable->constexpr_value= ApplyConstructorInitializer( variable_for_initialization, synt_args, src_loc, names, function_context );

		function_context.variables_state.RemoveNode( variable_for_initialization );
	}

	RegisterTemporaryVariable( function_context, variable );
	return variable;
}

VariablePtr CodeBuilder::ConvertVariable(
	const VariablePtr variable,
	const Type& dst_type,
	const FunctionVariable& conversion_constructor,
	NamesScope& names,
	FunctionContext& function_context,
	const SrcLoc& src_loc )
{
	if( !EnsureTypeComplete( dst_type ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), src_loc, dst_type );
		return nullptr;
	}

	const VariableMutPtr result=
		std::make_shared<Variable>(
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
			std::make_shared<Variable>(
				dst_type,
				ValueType::ReferenceMut,
				Variable::Location::Pointer,
				result->name,
				result->llvm_value );
		function_context.variables_state.AddNode( result_for_initialization );
		function_context.variables_state.AddLink( result, result_for_initialization );

		DoCallFunction(
			EnsureLLVMFunctionCreated( conversion_constructor ),
			conversion_constructor.type,
			src_loc,
			{ result_for_initialization, variable },
			{},
			false,
			names,
			function_context,
			false );

		CallDestructors( temp_variables_storage, names, function_context, src_loc );

		function_context.variables_state.RemoveNode( result_for_initialization );
	}

	RegisterTemporaryVariable( function_context, result );
	return result;
}

bool CodeBuilder::EvaluateBoolConstantExpression( NamesScope& names, FunctionContext& function_context, const Synt::Expression& expression )
{
	const VariablePtr v= BuildExpressionCodeEnsureVariable( expression, names, function_context );
	if( v->type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), Synt::GetExpressionSrcLoc( expression ), bool_type_, v->type );
		return false;
	}
	if( v->constexpr_value == nullptr )
	{
		REPORT_ERROR( ExpectedConstantExpression, names.GetErrors(), Synt::GetExpressionSrcLoc( expression ) );
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
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), Synt::GetExpressionSrcLoc( expression ) );

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
