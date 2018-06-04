#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "keywords.hpp"
#include "lang_types.hpp"

#include "code_builder.hpp"

#define CHECK_RETURN_ERROR_VALUE(value) if( value.GetErrorValue() != nullptr ) { return value; }
#define CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(value) if( value.GetType() == NontypeStub::TemplateDependentValue ) { return value; }

namespace U
{

namespace CodeBuilderPrivate
{

Value CodeBuilder::BuildExpressionCodeAndDestroyTemporaries(
	const Synt::IExpressionComponent& expression,
	NamesScope& names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	const Value result= BuildExpressionCode( expression, names, function_context );

	CallDestructors( *function_context.stack_variables_stack.back(), function_context, expression.GetFilePos() );

	return result;
}

boost::optional<Value> CodeBuilder::TryCallOverloadedBinaryOperator(
	const OverloadedOperator op,
	const Synt::SyntaxElementBase& op_syntax_element,
	const Synt::IExpressionComponent&  left_expr,
	const Synt::IExpressionComponent& right_expr,
	const bool evaluate_args_in_reverse_order,
	const FilePos& file_pos,
	NamesScope& names,
	FunctionContext& function_context )
{
	const FunctionVariable* overloaded_operator= nullptr;

	const auto cache_it= function_context.overloading_resolutin_cache.find( &op_syntax_element );
	if( cache_it != function_context.overloading_resolutin_cache.end() )
		overloaded_operator= cache_it->second.get_ptr();
	else
	{
		std::vector<Function::Arg> args;
		args.reserve( 2u );
		const size_t error_count_before= errors_.size();

		bool needs_move_assign= false;
		bool needs_function_pointer_assignment= false;
		// Know args types.
		{
			// Prepare dummy function context for first pass.
			FunctionContext dummy_function_context(
				function_context.return_type,
				function_context.return_value_is_mutable,
				function_context.return_value_is_reference,
				llvm_context_,
				dummy_function_context_->function );
			const StackVariablesStorage dummy_stack_variables_storage( dummy_function_context );
			dummy_function_context.this_= function_context.this_;
			dummy_function_context.whole_this_is_unavailable= function_context.whole_this_is_unavailable;
			dummy_function_context.variables_state= function_context.variables_state;
			function_context.variables_state.DeactivateLocks();

			const Value l_var_value= BuildExpressionCode( left_expr , names, dummy_function_context );
			const Value r_var_value= BuildExpressionCode( right_expr, names, dummy_function_context );

			function_context.overloading_resolutin_cache.insert(
				dummy_function_context.overloading_resolutin_cache.begin(),
				dummy_function_context.overloading_resolutin_cache.end() );

			CHECK_RETURN_ERROR_VALUE(l_var_value);
			CHECK_RETURN_ERROR_VALUE(l_var_value);
			CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(l_var_value);
			CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(r_var_value);

			const Variable* const l_var= l_var_value.GetVariable();
			const Variable* const r_var= r_var_value.GetVariable();

			if( l_var == nullptr )
				errors_.push_back( ReportExpectedVariable( file_pos, l_var_value.GetType().ToString() ) );
			if( r_var == nullptr )
				errors_.push_back( ReportExpectedVariable( file_pos, r_var_value.GetType().ToString() ) );
			if( l_var == nullptr || r_var == nullptr )
				return Value(ErrorValue());

			// Try apply move-assignment for class types.
			needs_move_assign=
				op == OverloadedOperator::Assign && r_var->value_type == ValueType::Value &&
				r_var->type == l_var->type && r_var->type.GetClassType() != nullptr &&
				l_var->value_type == ValueType::Reference;

			args.emplace_back();
			args.back().type= l_var->type;
			args.back().is_reference= l_var->value_type != ValueType::Value;
			args.back().is_mutable= l_var->value_type == ValueType::Reference;

			args.emplace_back();
			args.back().type= r_var->type;
			args.back().is_reference= r_var->value_type != ValueType::Value;
			args.back().is_mutable= r_var->value_type == ValueType::Reference;
		}
		errors_.resize( error_count_before );
		function_context.variables_state.ActivateLocks();

		// Apply here move-assignment for class types.
		if( needs_move_assign )
		{
			// Move here, instead of calling copy-assignment operator. Before moving we must also call destructor for destination.
			const Variable l_var_real= *BuildExpressionCode(  left_expr, names, function_context ).GetVariable();
			const Variable r_var_real= *BuildExpressionCode( right_expr, names, function_context ).GetVariable();
			if( l_var_real.type.HaveDestructor() )
				CallDestructor( l_var_real.llvm_value, l_var_real.type, function_context, file_pos );
			CopyBytes( r_var_real.llvm_value, l_var_real.llvm_value, l_var_real.type, function_context );

			// Write references from src to dst and check it.
			for( const StoredVariablePtr& l_var_variable : l_var_real.referenced_variables )
			for( const StoredVariablePtr& r_var_variable : r_var_real.referenced_variables )
			{
				for( const auto& inner_reference : function_context.variables_state.GetVariableReferences( r_var_variable ) )
				{
					const bool ok= function_context.variables_state.AddPollution( l_var_variable, inner_reference.first, inner_reference.second.IsMutable() );
					if( !ok )
						errors_.push_back( ReportReferenceProtectionError( file_pos, inner_reference.first->name ) );
				}
			}

			U_ASSERT( r_var_real.referenced_variables.size() == 1u );
			function_context.variables_state.Move( *r_var_real.referenced_variables.begin() );

			Variable move_result;
			move_result.type= void_type_;
			return Value( move_result, file_pos );
		}

		overloaded_operator= GetOverloadedOperator( args, op, file_pos );

		if( overloaded_operator == nullptr )
			function_context.overloading_resolutin_cache[ &op_syntax_element ]= boost::none;
		else
			function_context.overloading_resolutin_cache[ &op_syntax_element ]= *overloaded_operator;
	}

	if( overloaded_operator != nullptr )
	{
		if( overloaded_operator->virtual_table_index != ~0u )
		{
			// We can not fetch virtual function here, because "this" may be evaluated as second operand for some binary operators.
			errors_.push_back( ReportNotImplemented( file_pos, "calling virtual binary operators" ) );
		}

		std::vector<const Synt::IExpressionComponent*> synt_args;
		synt_args.reserve( 2u );
		synt_args.push_back( & left_expr );
		synt_args.push_back( &right_expr );
		return DoCallFunction( overloaded_operator->llvm_function, *overloaded_operator->type.GetFunctionType(), file_pos, nullptr, synt_args, evaluate_args_in_reverse_order, names, function_context );
	}

	return boost::none;
}

Value CodeBuilder::BuildExpressionCode(
	const Synt::IExpressionComponent& expression,
	NamesScope& names,
	FunctionContext& function_context )
{
	Value result;

	if( const auto binary_operator=
		dynamic_cast<const Synt::BinaryOperator*>(&expression) )
	{
		if( binary_operator->operator_type_ == BinaryOperatorType::LazyLogicalAnd ||
			binary_operator->operator_type_ == BinaryOperatorType::LazyLogicalOr )
		{
			return
				BuildLazyBinaryOperator(
					*binary_operator->left_,
					*binary_operator->right_,
					*binary_operator,
					binary_operator->file_pos_,
					names,
					function_context );
		}
		else
		{
			const boost::optional<Value> overloaded_operator_call_try=
				TryCallOverloadedBinaryOperator(
					GetOverloadedOperatorForBinaryOperator( binary_operator->operator_type_ ),
					*binary_operator,
					*binary_operator->left_, *binary_operator->right_,
					false,
					binary_operator->file_pos_,
					names,
					function_context );
			if( overloaded_operator_call_try != boost::none )
				return *overloaded_operator_call_try;

			Value l_var_value=
				BuildExpressionCode(
					*binary_operator->left_,
					names,
					function_context );
			Variable* const l_var= l_var_value.GetVariable(); U_ASSERT( l_var != nullptr );

			if( l_var->type.GetFundamentalType() != nullptr || l_var->type.GetEnumType() != nullptr || l_var->type.GetFunctionPointerType() != nullptr )
			{
				// Save l_var in register, because build-in binary operators require value-parameters.
				if( l_var->location == Variable::Location::Pointer )
				{
					l_var->llvm_value= CreateMoveToLLVMRegisterInstruction( *l_var, function_context );
					l_var->location= Variable::Location::LLVMRegister;
				}
				l_var->value_type= ValueType::Value;
			}

			Value r_var_value=
				BuildExpressionCode(
					*binary_operator->right_,
					names,
					function_context );
			Variable* const r_var= r_var_value.GetVariable(); U_ASSERT( r_var != nullptr );

			if( l_var->type.GetTemplateDependentType() != nullptr || r_var->type.GetTemplateDependentType() != nullptr )
			{
				Variable result;
				result.type= GetNextTemplateDependentType();
				result.value_type= ValueType::Value;
				return Value( result, binary_operator->file_pos_ );
			}

			return BuildBinaryOperator( *l_var, *r_var, binary_operator->operator_type_, binary_operator->file_pos_, function_context );
		}
	}
	else if( const auto named_operand=
		dynamic_cast<const Synt::NamedOperand*>(&expression) )
	{
		result= BuildNamedOperand( *named_operand, names, function_context );
	}
	else if( const auto numeric_constant=
		dynamic_cast<const Synt::NumericConstant*>(&expression) )
	{
		result= BuildNumericConstant( *numeric_constant, function_context );
	}
	else if( const auto boolean_constant=
		dynamic_cast<const Synt::BooleanConstant*>(&expression) )
	{
		result= Value( BuildBooleanConstant( *boolean_constant, function_context ), boolean_constant->file_pos_ );
	}
	else if( const auto bracket_expression=
		dynamic_cast<const Synt::BracketExpression*>(&expression) )
	{
		result= BuildExpressionCode( *bracket_expression->expression_, names, function_context );
	}
	else if( const auto type_name_in_expression=
		dynamic_cast<const Synt::TypeNameInExpression*>(&expression) )
	{
		result=
			Value(
				PrepareType( type_name_in_expression->type_name, names ),
				type_name_in_expression->file_pos_ );
	}
	else if( const auto move_operator=
		dynamic_cast<const Synt::MoveOperator*>(&expression) )
	{
		result= BuildMoveOpeator( *move_operator, names, function_context );
	}
	else if( const auto cast_ref= dynamic_cast<const Synt::CastRef*>(&expression) )
	{
		result= BuildCastRef( *cast_ref, names, function_context );
	}
	else if( const auto cast_ref_unsafe= dynamic_cast<const Synt::CastRefUnsafe*>(&expression) )
	{
		result= BuildCastRefUnsafe( *cast_ref_unsafe, names, function_context );
	}
	else if( const auto cast_imut= dynamic_cast<const Synt::CastImut*>(&expression) )
	{
		result= BuildCastImut( *cast_imut, names, function_context );
	}
	else if( const auto cast_mut= dynamic_cast<const Synt::CastMut*>(&expression) )
	{
		result= BuildCastMut( *cast_mut, names, function_context );
	}
	else U_ASSERT(false);

	if( const auto expression_with_unary_operators=
		dynamic_cast<const Synt::ExpressionComponentWithUnaryOperators*>( &expression ) )
	{
		for( const Synt::IUnaryPostfixOperatorPtr& postfix_operator : expression_with_unary_operators->postfix_operators_ )
		{
			if( const auto indexation_operator=
				dynamic_cast<const Synt::IndexationOperator*>( postfix_operator.get() ) )
			{
				result= BuildIndexationOperator( result, *indexation_operator, names, function_context );
			}
			else if( const auto member_access_operator=
				dynamic_cast<const Synt::MemberAccessOperator*>( postfix_operator.get() ) )
			{
				result= BuildMemberAccessOperator( result, *member_access_operator, names, function_context );
			}
			else if( const auto call_operator=
				dynamic_cast<const Synt::CallOperator*>( postfix_operator.get() ) )
			{
				result= BuildCallOperator( result, *call_operator, names, function_context );
			}
			else
				U_ASSERT(false);
		} // for unary postfix operators

		for( const Synt::IUnaryPrefixOperatorPtr& prefix_operator : expression_with_unary_operators->prefix_operators_ )
		{
			if( result.GetTemplateDependentValue() != nullptr )
				continue;
			const Variable* const var= result.GetVariable();
			if( var == nullptr )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( expression_with_unary_operators->file_pos_, result.GetType().ToString() ) );
				continue;
			}

			std::vector<Function::Arg> args;
			args.emplace_back();
			args.back().type= var->type;
			args.back().is_mutable= var->value_type == ValueType::Reference;
			args.back().is_reference= var->value_type != ValueType::Value;

			OverloadedOperator op= OverloadedOperator::None;
			if( dynamic_cast<const Synt::UnaryMinus*>( prefix_operator.get() ) != nullptr )
				op= OverloadedOperator::Sub;
			else if( dynamic_cast<const Synt::UnaryPlus*>( prefix_operator.get() ) != nullptr )
				op= OverloadedOperator::Add;
			else if( dynamic_cast<const Synt::LogicalNot*>( prefix_operator.get() ) != nullptr )
				op= OverloadedOperator::LogicalNot;
			else if( dynamic_cast<const Synt::BitwiseNot*>( prefix_operator.get() ) != nullptr )
				op= OverloadedOperator::BitwiseNot;
			else U_ASSERT( false );

			const FunctionVariable* const overloaded_operator= GetOverloadedOperator( args, op, expression_with_unary_operators->file_pos_ );
			if( overloaded_operator != nullptr )
			{
				if( overloaded_operator->is_this_call && overloaded_operator->virtual_table_index != ~0u )
				{
					const auto fetch_result= TryFetchVirtualFunction( *var, *overloaded_operator, function_context );
					result= DoCallFunction( fetch_result.second, *overloaded_operator->type.GetFunctionType(), expression_with_unary_operators->file_pos_, &fetch_result.first, {}, false, names, function_context );
				}
				else
					result= DoCallFunction( overloaded_operator->llvm_function, *overloaded_operator->type.GetFunctionType(), expression_with_unary_operators->file_pos_, var, {}, false, names, function_context );
			}
			else
			{
				if( const auto unary_minus=
					dynamic_cast<const Synt::UnaryMinus*>( prefix_operator.get() ) )
				{
					result= BuildUnaryMinus( result, *unary_minus, function_context );
				}
				else if( const auto unary_plus=
					dynamic_cast<const Synt::UnaryPlus*>( prefix_operator.get() ) )
				{
					// TODO - maybe do something here?
					(void)unary_plus;
				}
				else if( const auto logical_not=
					dynamic_cast<const Synt::LogicalNot*>( prefix_operator.get() ) )
				{
					result= BuildLogicalNot( result, *logical_not, function_context );
				}
				else if( const auto bitwise_not=
					dynamic_cast<const Synt::BitwiseNot*>( prefix_operator.get() ) )
				{
					result= BuildBitwiseNot( result, *bitwise_not, function_context );
				}
				else
					U_ASSERT(false);
			}
		} // for unary prefix operators

		return result;
	}
}

Value CodeBuilder::BuildBinaryOperator(
	const Variable& l_var,
	const Variable& r_var,
	const BinaryOperatorType binary_operator,
	const FilePos& file_pos,
	FunctionContext& function_context )
{
	const bool arguments_are_constexpr= l_var.constexpr_value != nullptr && r_var.constexpr_value != nullptr;

	Variable result;

	const Type& l_type= l_var.type;
	const Type& r_type= r_var.type;
	const FundamentalType* const l_fundamental_type= l_type.GetFundamentalType();
	const FundamentalType* const r_fundamental_type= r_var.type.GetFundamentalType();

	using BinaryOperatorType= BinaryOperatorType;
	switch( binary_operator )
	{
	case BinaryOperatorType::Add:
	case BinaryOperatorType::Sub:
	case BinaryOperatorType::Mul:
	case BinaryOperatorType::Div:
	case BinaryOperatorType::Rem:

		if( r_var.type != l_var.type )
		{
			errors_.push_back( ReportNoMatchBinaryOperatorForGivenTypes( file_pos, r_var.type.ToString(), l_var.type.ToString(), BinaryOperatorToString( binary_operator ) ) );
			return ErrorValue();
		}
		if( l_fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
			return ErrorValue();
		}
		else
		{
			if( l_type.SizeOf() < 4u )
			{
				// Operation supported only for 32 and 64bit operands
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
				return ErrorValue();
			}
			const bool is_float= IsFloatingPoint( l_fundamental_type->fundamental_type );
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || is_float ) )
			{
				// this operations allowed only for integer and floating point operands.
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
				return ErrorValue();
			}

			const bool is_signed= IsSignedInteger( l_fundamental_type->fundamental_type );

			llvm::Value* l_value_for_op= nullptr;
			llvm::Value* r_value_for_op= nullptr;
			llvm::Value* result_value= nullptr;
			if( !arguments_are_constexpr )
			{
				l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
				r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			}

			switch( binary_operator )
			{
			case BinaryOperatorType::Add:
				if( is_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFAdd( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFAdd( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getAdd( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateAdd( l_value_for_op, r_value_for_op );
				}
				break;

			case BinaryOperatorType::Sub:
				if( is_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFSub( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFSub( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getSub( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateSub( l_value_for_op, r_value_for_op );
				}
				break;

			case BinaryOperatorType::Div:
				if( is_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFDiv( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFDiv( l_value_for_op, r_value_for_op );
				}
				else if( is_signed )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getSDiv( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateSDiv( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getUDiv( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateUDiv( l_value_for_op, r_value_for_op );
				}
				break;

			case BinaryOperatorType::Mul:
				if( is_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFMul( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFMul( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getMul( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateMul( l_value_for_op, r_value_for_op );
				}
				break;

			case BinaryOperatorType::Rem:
				if( is_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFRem( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFRem( l_value_for_op, r_value_for_op );
				}
				else if( is_signed )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getSRem( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateSRem( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getURem( l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateURem( l_value_for_op, r_value_for_op );
				}
				break;

			default: U_ASSERT( false ); break;
			};

			if( arguments_are_constexpr )
				result_value= result.constexpr_value;
			else U_ASSERT( result_value != nullptr );

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type= r_var.type;
			result.llvm_value= result_value;
		}
		break;


	case BinaryOperatorType::Equal:
	case BinaryOperatorType::NotEqual:

		if( r_var.type != l_var.type )
		{
			errors_.push_back( ReportNoMatchBinaryOperatorForGivenTypes( file_pos, r_var.type.ToString(), l_var.type.ToString(), BinaryOperatorToString( binary_operator ) ) );
			return ErrorValue();
		}
		else if( l_var.type.GetFunctionPointerType() != nullptr )
		{
			llvm::Value* l_value_for_op= nullptr;
			llvm::Value* r_value_for_op= nullptr;
			llvm::Value* result_value= nullptr;
			if( !arguments_are_constexpr )
			{
				l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
				r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			}
			switch( binary_operator )
			{
			case BinaryOperatorType::Equal:
				if( arguments_are_constexpr )
					result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_EQ, l_var.constexpr_value, r_var.constexpr_value );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpEQ( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::NotEqual:
				if( arguments_are_constexpr )
					result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_NE, l_var.constexpr_value, r_var.constexpr_value );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpNE( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT(false); break;
			}

			if( arguments_are_constexpr )
				result_value= result.constexpr_value;
			else U_ASSERT( result_value != nullptr );

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type= bool_type_;
			result.llvm_value= result_value;
		}
		else if( !( l_var.type.GetFundamentalType() != nullptr || l_var.type.GetEnumType() != nullptr ) )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
			return ErrorValue();
		}
		else
		{
			const FundamentalType raw_fundamental_type= l_fundamental_type != nullptr ? *l_fundamental_type : l_var.type.GetEnumType()->underlaying_type;

			const bool if_float= IsFloatingPoint( raw_fundamental_type.fundamental_type );
			if( !( IsInteger( raw_fundamental_type.fundamental_type ) || if_float || raw_fundamental_type.fundamental_type == U_FundamentalType::Bool ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
				return ErrorValue();
			}

			llvm::Value* l_value_for_op= nullptr;
			llvm::Value* r_value_for_op= nullptr;
			llvm::Value* result_value= nullptr;
			if( !arguments_are_constexpr )
			{
				l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
				r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			}

			switch( binary_operator )
			{
			// TODO - select ordered/unordered comparision flags for floats.
			case BinaryOperatorType::Equal:
				if( if_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_UEQ, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpUEQ( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_EQ, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateICmpEQ( l_value_for_op, r_value_for_op );
				}
				break;

			case BinaryOperatorType::NotEqual:
				if( if_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_UNE, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpUNE( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_NE, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateICmpNE( l_value_for_op, r_value_for_op );
				}
				break;

			default: U_ASSERT( false ); break;
			};

			if( arguments_are_constexpr )
				result_value= result.constexpr_value;
			else U_ASSERT( result_value != nullptr );

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type= bool_type_;
			result.llvm_value= result_value;
		}
		break;

	case BinaryOperatorType::Less:
	case BinaryOperatorType::LessEqual:
	case BinaryOperatorType::Greater:
	case BinaryOperatorType::GreaterEqual:

		if( r_var.type != l_var.type )
		{
			errors_.push_back( ReportNoMatchBinaryOperatorForGivenTypes( file_pos, r_var.type.ToString(), l_var.type.ToString(), BinaryOperatorToString( binary_operator ) ) );
			return ErrorValue();
		}
		if( l_fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
			return ErrorValue();
		}
		else
		{
			const bool if_float= IsFloatingPoint( l_fundamental_type->fundamental_type );
			const bool is_signed= IsSignedInteger( l_fundamental_type->fundamental_type );
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || if_float ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
				return ErrorValue();
			}

			llvm::Value* l_value_for_op= nullptr;
			llvm::Value* r_value_for_op= nullptr;
			llvm::Value* result_value= nullptr;
			if( !arguments_are_constexpr )
			{
				l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
				r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			}

			switch( binary_operator )
			{
			// TODO - select ordered/unordered comparision flags for floats.
			case BinaryOperatorType::Less:
				if( if_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_ULT, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpULT( l_value_for_op, r_value_for_op );
				}
				else if( is_signed )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_SLT, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateICmpSLT( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_ULT, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateICmpULT( l_value_for_op, r_value_for_op );
				}
				break;

			case BinaryOperatorType::LessEqual:
				if( if_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_ULE, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpULE( l_value_for_op, r_value_for_op );
				}
				else if( is_signed )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_SLE, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateICmpSLE( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_ULE, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateICmpULE( l_value_for_op, r_value_for_op );
				}
				break;

			case BinaryOperatorType::Greater:
				if( if_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_UGT, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpUGT( l_value_for_op, r_value_for_op );
				}
				else if( is_signed )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_SGT, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateICmpSGT( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_UGT, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateICmpUGT( l_value_for_op, r_value_for_op );
				}
				break;

			case BinaryOperatorType::GreaterEqual:
				if( if_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_UGE, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpUGE( l_value_for_op, r_value_for_op );
				}
				else if( is_signed )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_SGE, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateICmpSGE( l_value_for_op, r_value_for_op );
				}
				else
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getICmp( llvm::CmpInst::ICMP_UGE, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateICmpUGE( l_value_for_op, r_value_for_op );
				}
				break;

			default: U_ASSERT( false ); break;
			};

			if( arguments_are_constexpr )
				result_value= result.constexpr_value;
			else U_ASSERT( result_value != nullptr );

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type= bool_type_;
			result.llvm_value= result_value;
		}
		break;

	case BinaryOperatorType::And:
	case BinaryOperatorType::Or:
	case BinaryOperatorType::Xor:

		if( r_var.type != l_var.type )
		{
			errors_.push_back( ReportNoMatchBinaryOperatorForGivenTypes( file_pos, r_var.type.ToString(), l_var.type.ToString(), BinaryOperatorToString( binary_operator ) ) );
			return ErrorValue();
		}
		if( l_fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
			return ErrorValue();
		}
		else
		{
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || l_fundamental_type->fundamental_type == U_FundamentalType::Bool ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
				return ErrorValue();
			}

			llvm::Value* l_value_for_op= nullptr;
			llvm::Value* r_value_for_op= nullptr;
			llvm::Value* result_value= nullptr;
			if( !arguments_are_constexpr )
			{
				l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
				r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			}

			switch( binary_operator )
			{
			case BinaryOperatorType::And:
				if( arguments_are_constexpr )
					result.constexpr_value= llvm::ConstantExpr::getAnd( l_var.constexpr_value, r_var.constexpr_value );
				else
					result_value= function_context.llvm_ir_builder.CreateAnd( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Or:
				if( arguments_are_constexpr )
					result.constexpr_value= llvm::ConstantExpr::getOr( l_var.constexpr_value, r_var.constexpr_value );
				else
					result_value= function_context.llvm_ir_builder.CreateOr( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Xor:
				if( arguments_are_constexpr )
					result.constexpr_value= llvm::ConstantExpr::getXor( l_var.constexpr_value, r_var.constexpr_value );
				else
					result_value= function_context.llvm_ir_builder.CreateXor( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			if( arguments_are_constexpr )
				result_value= result.constexpr_value;
			else U_ASSERT( result_value != nullptr );

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type= l_type;
			result.llvm_value= result_value;
		}
		break;

	case BinaryOperatorType::ShiftLeft :
	case BinaryOperatorType::ShiftRight:
		{
			if( l_fundamental_type == nullptr || !IsInteger( l_fundamental_type->fundamental_type ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
				return ErrorValue();
			}
			if( r_fundamental_type == nullptr || !IsUnsignedInteger( r_fundamental_type->fundamental_type ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, r_type.ToString() ) );
				return ErrorValue();
			}

			if( l_var.constexpr_value != nullptr && r_var.constexpr_value != nullptr )
			{
				// Convert value of shift to type of shifted value. LLVM Reuqired this.
				llvm::Constant* r_value_for_op= r_var.constexpr_value;
				if( r_var.type.SizeOf() > l_var.type.SizeOf() )
					r_value_for_op= llvm::ConstantExpr::getTrunc( r_value_for_op, l_var.type.GetLLVMType() );
				else if( r_var.type.SizeOf() < l_var.type.SizeOf() )
					r_value_for_op= llvm::ConstantExpr::getZExt( r_value_for_op, l_var.type.GetLLVMType() );

				if( binary_operator == BinaryOperatorType::ShiftLeft )
					result.constexpr_value= llvm::ConstantExpr::getShl( l_var.constexpr_value, r_value_for_op );
				else if( binary_operator == BinaryOperatorType::ShiftRight )
				{
					if( IsSignedInteger( l_fundamental_type->fundamental_type ) )
						result.constexpr_value= llvm::ConstantExpr::getAShr( l_var.constexpr_value, r_value_for_op );
					else
						result.constexpr_value= llvm::ConstantExpr::getLShr( l_var.constexpr_value, r_value_for_op );
				}
				else U_ASSERT(false);

				result.llvm_value= result.constexpr_value;
			}
			else
			{
				llvm::Value* const l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
				llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );

				// Convert value of shift to type of shifted value. LLVM Reuqired this.
				if( r_var.type.SizeOf() > l_var.type.SizeOf() )
					r_value_for_op= function_context.llvm_ir_builder.CreateTrunc( r_value_for_op, l_var.type.GetLLVMType() );
				else if( r_var.type.SizeOf() < l_var.type.SizeOf() )
					r_value_for_op= function_context.llvm_ir_builder.CreateZExt( r_value_for_op, l_var.type.GetLLVMType() );

				if( binary_operator == BinaryOperatorType::ShiftLeft )
					result.llvm_value= function_context.llvm_ir_builder.CreateShl( l_value_for_op, r_value_for_op );
				else if( binary_operator == BinaryOperatorType::ShiftRight )
				{
					if( IsSignedInteger( l_fundamental_type->fundamental_type ) )
						result.llvm_value= function_context.llvm_ir_builder.CreateAShr( l_value_for_op, r_value_for_op );
					else
						result.llvm_value= function_context.llvm_ir_builder.CreateLShr( l_value_for_op, r_value_for_op );
				}
				else U_ASSERT(false);
			}

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type= l_type;
		}
		break;

	case BinaryOperatorType::LazyLogicalAnd:
	case BinaryOperatorType::LazyLogicalOr:
	case BinaryOperatorType::Last:
		U_ASSERT(false);
		break;
	};

	if( result.constexpr_value != nullptr )
	{
		// Undef value can occurs in integer division by zero or something like it.
		// But, if inputs are undef, this means, that they are template-dependent and this is not error case.
		if( llvm::dyn_cast<llvm::UndefValue >(result.constexpr_value) != nullptr &&
			llvm::dyn_cast<llvm::UndefValue >(r_var.constexpr_value) == nullptr &&
			llvm::dyn_cast<llvm::UndefValue >(l_var.constexpr_value) == nullptr )
		{
			errors_.push_back( ReportConstantExpressionResultIsUndefined( file_pos ) );
			result.constexpr_value= nullptr;
		}
	}

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( BinaryOperatorToString(binary_operator), result );
	result.referenced_variables.emplace( stored_result );
	function_context.stack_variables_stack.back()->RegisterVariable( stored_result );

	return Value( result, file_pos );
}

Value CodeBuilder::BuildLazyBinaryOperator(
	const Synt::IExpressionComponent& l_expression,
	const Synt::IExpressionComponent& r_expression,
	const Synt::BinaryOperator& binary_operator,
	const FilePos& file_pos,
	NamesScope& names,
	FunctionContext& function_context )
{
	#define RETURN_UNDEF_BOOL\
	{\
		Variable result;\
		result.value_type= ValueType::Value;\
		result.location= Variable::Location::LLVMRegister;\
		result.type= bool_type_;\
		result.llvm_value= llvm::UndefValue::get( fundamental_llvm_types_.bool_ );\
		return Value( result, file_pos );\
	}

	const Value l_var_value= BuildExpressionCode( l_expression, names, function_context );
	CHECK_RETURN_ERROR_VALUE(l_var_value);

	if( l_var_value.GetType() == NontypeStub::TemplateDependentValue )
	{
		BuildExpressionCode( r_expression, names, function_context );
		return l_var_value;
	}
	if( l_var_value.GetType().GetTemplateDependentType() != nullptr )
	{
		BuildExpressionCode( r_expression, names, function_context );
		RETURN_UNDEF_BOOL
	}

	if( l_var_value.GetType() != bool_type_ )
	{
		errors_.push_back( ReportTypesMismatch( binary_operator.file_pos_, bool_type_.ToString(), l_var_value.GetType().ToString() ) );
		return ErrorValue();
	}
	const Variable l_var= *l_var_value.GetVariable();

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

	VariablesState variables_state_before_r_branch= function_context.variables_state;
	variables_state_before_r_branch.DeactivateLocks();

	llvm::Value* r_var_in_register= nullptr;
	llvm::Constant* r_var_constepxr_value= nullptr;
	{
		// Right part of lazy operator is conditinal. So, we must destroy its temporaries only in this condition.
		// We doesn`t needs longer lifetime of epxression temporaries, because we use only bool result.
		const StackVariablesStorage r_var_temp_variables_storage( function_context );

		const Value r_var_value= BuildExpressionCode( r_expression, names, function_context );
		CHECK_RETURN_ERROR_VALUE(r_var_value);

		if( r_var_value.GetType().GetTemplateDependentType() != nullptr )
			RETURN_UNDEF_BOOL
		if( r_var_value.GetType() == NontypeStub::TemplateDependentValue )
			r_var_in_register= r_var_constepxr_value= llvm::UndefValue::get( fundamental_llvm_types_.bool_ );
		else
		{
			if( r_var_value.GetType() != bool_type_ )
			{
				errors_.push_back( ReportTypesMismatch( binary_operator.file_pos_, bool_type_.ToString(), r_var_value.GetType().ToString() ) );
				return ErrorValue();
			}
			const Variable& r_var= *r_var_value.GetVariable();
			r_var_constepxr_value= r_var.constexpr_value;
			r_var_in_register= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
		}

		// Destroy r_var temporaries in this branch.
		CallDestructors( *function_context.stack_variables_stack.back(), function_context, file_pos );
	}
	function_context.variables_state= MergeVariablesStateAfterIf( { variables_state_before_r_branch, function_context.variables_state }, file_pos );

	function_context.llvm_ir_builder.CreateBr( block_after_operator );
	function_context.function->getBasicBlockList().push_back( block_after_operator );
	function_context.llvm_ir_builder.SetInsertPoint( block_after_operator );

	llvm::PHINode* const phi= function_context.llvm_ir_builder.CreatePHI( fundamental_llvm_types_.bool_, 2u );
	phi->addIncoming( l_var_in_register, l_part_block );
	phi->addIncoming( r_var_in_register, r_part_block );

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

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( BinaryOperatorToString(binary_operator.operator_type_), result );
	result.referenced_variables.emplace( stored_result );
	function_context.stack_variables_stack.back()->RegisterVariable( stored_result );
	return Value( result, file_pos );

	#undef RETURN_UNDEF_BOOL
}

Value CodeBuilder::BuildCastRef( const Synt::CastRef& cast_ref, NamesScope& names, FunctionContext& function_context )
{
	return DoReferenceCast( cast_ref.file_pos_, cast_ref.type_, cast_ref.expression_, false, names, function_context );
}

Value CodeBuilder::BuildCastRefUnsafe( const Synt::CastRefUnsafe& cast_ref_unsafe, NamesScope& names, FunctionContext& function_context )
{
	if( !function_context.is_in_unsafe_block )
		errors_.push_back( ReportUnsafeReferenceCastOutsideUnsafeBlock( cast_ref_unsafe.file_pos_ ) );

	return DoReferenceCast( cast_ref_unsafe.file_pos_, cast_ref_unsafe.type_, cast_ref_unsafe.expression_, true, names, function_context );
}

Value CodeBuilder::DoReferenceCast(
	const FilePos& file_pos,
	const Synt::ITypeNamePtr& type_name,
	const Synt::IExpressionComponentPtr& expression,
	bool enable_unsafe,
	NamesScope& names,
	FunctionContext& function_context )
{
	const Type type= PrepareType( type_name, names );
	if( type == invalid_type_ )
		return ErrorValue();

	const Value expr= BuildExpressionCode( *expression, names, function_context );
	CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(expr);
	CHECK_RETURN_ERROR_VALUE(expr);
	const Variable* var= expr.GetVariable();
	if( var == nullptr )
	{
		errors_.push_back( ReportExpectedVariable( file_pos, expr.GetType().ToString() ) );
		return ErrorValue();
	}

	Variable result;
	result.type= type;
	result.value_type= var->value_type == ValueType::Reference ? ValueType::Reference : ValueType::ConstReference; // "ValueType" here converts inot ConstReference.
	result.location= Variable::Location::Pointer;
	result.referenced_variables= var->referenced_variables;

	llvm::Value* src_value= var->llvm_value;
	if( var->location == Variable::Location::LLVMRegister )
	{
		src_value= function_context.alloca_ir_builder.CreateAlloca( var->type.GetLLVMType() );
		function_context.llvm_ir_builder.CreateStore( var->llvm_value, src_value );
	}

	if( type == var->type )
		result.llvm_value= src_value;
	else if( type == void_type_ )
		result.llvm_value= CreateReferenceCast( src_value, var->type, type, function_context );
	else
	{
		// Complete types required for both safe and unsafe casting, except unsafe void to anything cast.
		// This needs, becasue we must emit same code for places where types yet not complete, and where they are complete.
		if( type.IsIncomplete() )
			errors_.push_back( ReportUsingIncompleteType( file_pos, type.ToString() ) );
		if( var->type.IsIncomplete() && !( enable_unsafe && var->type == void_type_ ) )
			errors_.push_back( ReportUsingIncompleteType( file_pos, var->type.ToString() ) );

		if( var->type.ReferenceIsConvertibleTo( type ) )
			result.llvm_value= CreateReferenceCast( src_value, var->type, type, function_context );
		else
		{
			result.llvm_value= function_context.llvm_ir_builder.CreatePointerCast( src_value, llvm::PointerType::get( type.GetLLVMType(), 0 ) );
			if( !enable_unsafe )
				errors_.push_back( ReportTypesMismatch( file_pos, type.ToString(), var->type.ToString() ) );
		}
	}

	return Value( result, file_pos );
}

Value CodeBuilder::BuildCastImut( const Synt::CastImut& cast_imut, NamesScope& names, FunctionContext& function_context )
{
	const Value expr= BuildExpressionCode( *cast_imut.expression_, names, function_context );
	CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(expr);
	CHECK_RETURN_ERROR_VALUE(expr);
	const Variable* var= expr.GetVariable();
	if( var == nullptr )
	{
		errors_.push_back( ReportExpectedVariable( cast_imut.file_pos_, expr.GetType().ToString() ) );
		return ErrorValue();
	}

	Variable result= *var;
	result.value_type= ValueType::ConstReference;

	if( var->location == Variable::Location::LLVMRegister )
	{
		result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( var->type.GetLLVMType() );
		function_context.llvm_ir_builder.CreateStore( var->llvm_value, result.llvm_value );
	}

	return Value( result, cast_imut.file_pos_ );
}

Value CodeBuilder::BuildCastMut( const Synt::CastMut& cast_mut, NamesScope& names, FunctionContext& function_context )
{
	if( !function_context.is_in_unsafe_block )
		errors_.push_back( ReportMutableReferenceCastOutsideUnsafeBlock( cast_mut.file_pos_ ) );

	const Value expr= BuildExpressionCode( *cast_mut.expression_, names, function_context );
	CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(expr);
	CHECK_RETURN_ERROR_VALUE(expr);
	const Variable* var= expr.GetVariable();
	if( var == nullptr )
	{
		errors_.push_back( ReportExpectedVariable( cast_mut.file_pos_, expr.GetType().ToString() ) );
		return ErrorValue();
	}

	Variable result= *var;
	result.value_type= ValueType::Reference;

	if( var->location == Variable::Location::LLVMRegister )
	{
		result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( var->type.GetLLVMType() );
		function_context.llvm_ir_builder.CreateStore( var->llvm_value, result.llvm_value );
	}

	return Value( result, cast_mut.file_pos_ );
}

Value CodeBuilder::BuildNamedOperand(
	const Synt::NamedOperand& named_operand,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( named_operand.name_.components.size() == 1u &&
		named_operand.name_.components.back().template_parameters.empty() )
	{
		if( named_operand.name_.components.back().name == Keywords::this_ )
		{
			if( function_context.this_ == nullptr || function_context.whole_this_is_unavailable )
			{
				errors_.push_back( ReportThisUnavailable( named_operand.file_pos_ ) );
				return ErrorValue();
			}
			return Value( *function_context.this_, named_operand.file_pos_ );
		}
		else if( named_operand.name_.components.back().name == Keywords::base_ )
		{
			if( function_context.this_ == nullptr )
			{
				errors_.push_back( ReportBaseUnavailable( named_operand.file_pos_ ) );
				return ErrorValue();
			}
			const Class& class_= *function_context.this_->type.GetClassType();
			if( class_.base_class == nullptr )
			{
				errors_.push_back( ReportBaseUnavailable( named_operand.file_pos_ ) );
				return ErrorValue();
			}
			if( function_context.whole_this_is_unavailable && ( !function_context.base_initialized || class_.base_class->class_->kind == Class::Kind::Abstract ) )
			{
				errors_.push_back( ReportFieldIsNotInitializedYet( named_operand.file_pos_, Keyword( Keywords::base_ ) ) );
				return ErrorValue();
			}

			Variable base= *function_context.this_;
			base.type= class_.base_class;
			base.llvm_value= CreateReferenceCast( function_context.this_->llvm_value, function_context.this_->type, base.type, function_context );
			return Value( base, named_operand.file_pos_ );
		}
	}

	const NamesScope::InsertedName* name_entry= ResolveName( named_operand.file_pos_, names, named_operand.name_ );
	if( !name_entry )
	{
		errors_.push_back( ReportNameNotFound( named_operand.file_pos_, named_operand.name_ ) );
		return ErrorValue();
	}

	const ProgramString back_name_component= named_operand.name_.components.back().name ;
	if( !function_context.is_in_unsafe_block &&
		( back_name_component == Keywords::constructor_ || back_name_component == Keywords::destructor_ ) )
		errors_.push_back( ReportExplicitAccessToThisMethodIsUnsafe( named_operand.file_pos_, back_name_component ) );

	if( const ClassField* const field= name_entry->second.GetClassField() )
	{
		if( function_context.this_ == nullptr )
		{
			errors_.push_back( ReportClassFiledAccessInStaticMethod( named_operand.file_pos_, named_operand.name_.components.back().name ) );
			return ErrorValue();
		}

		const ClassProxyPtr class_= field->class_.lock();
		U_ASSERT( class_ != nullptr && "Class is dead? WTF?" );

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

		const ClassProxyPtr field_class_proxy= field->class_.lock();
		U_ASSERT( field_class_proxy != nullptr );

		llvm::Value* actual_field_class_ptr= nullptr;
		if( field_class_proxy == function_context.this_->type.GetClassTypeProxy() )
			actual_field_class_ptr= function_context.this_->llvm_value;
		else
		{
			// For parent filed we needs make several GEP isntructions.
			ClassProxyPtr actual_field_class= function_context.this_->type.GetClassTypeProxy();
			actual_field_class_ptr= function_context.this_->llvm_value;
			while( actual_field_class != field_class_proxy )
			{
				if( actual_field_class->class_->base_class == nullptr )
				{
					errors_.push_back( ReportAccessOfNonThisClassField( named_operand.file_pos_, named_operand.name_.components.back().name ) );
					return ErrorValue();
				}

				index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(actual_field_class->class_->base_class_field_number) ) );
				actual_field_class_ptr= function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );
				actual_field_class= actual_field_class->class_->base_class;
			}
		}

		if( function_context.whole_this_is_unavailable &&
			function_context.uninitialized_this_fields.find( field ) != function_context.uninitialized_this_fields.end() )
		{
			errors_.push_back( ReportFieldIsNotInitializedYet( named_operand.file_pos_, named_operand.name_.components.back().name ) );
			return ErrorValue();
		}
		if( function_context.whole_this_is_unavailable &&
			field_class_proxy != function_context.this_->type.GetClassTypeProxy() &&
			!function_context.base_initialized )
		{
			errors_.push_back( ReportFieldIsNotInitializedYet( named_operand.file_pos_, Keyword( Keywords::base_ ) ) );
			return ErrorValue();
		}

		Variable field_variable;
		field_variable.type= field->type;
		field_variable.location= Variable::Location::Pointer;
		field_variable.value_type= ( function_context.this_->value_type == ValueType::Reference && field->is_mutable ) ? ValueType::Reference : ValueType::ConstReference;
		field_variable.referenced_variables= function_context.this_->referenced_variables;

		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
		field_variable.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );

		if( field->is_reference )
		{
			field_variable.value_type= field->is_mutable ? ValueType::Reference : ValueType::ConstReference;
			field_variable.llvm_value= function_context.llvm_ir_builder.CreateLoad( field_variable.llvm_value );

			field_variable.referenced_variables.clear();
			for( const StoredVariablePtr& struct_variable : function_context.this_->referenced_variables )
			{
				for( const auto& referenced_variable_pair : function_context.variables_state.GetVariableReferences( struct_variable ) )
					field_variable.referenced_variables.insert( referenced_variable_pair.first );
			}
		}

		return Value( field_variable, named_operand.file_pos_ );
	}
	else if( const OverloadedFunctionsSet* const overloaded_functions_set=
		name_entry->second.GetFunctionsSet() )
	{
		if( function_context.this_ != nullptr )
		{
			// Trying add "this" to functions set.
			const Class* const class_= function_context.this_->type.GetClassType();

			// SPRACHE_TODO - mabe this kind of search is incorrect?
			const NamesScope::InsertedName* const same_set_in_class=
				class_->members.GetThisScopeName( named_operand.name_.components.back().name );
			// SPRACHE_TODO - add "this" for functions from parent classes.
			if( name_entry == same_set_in_class )
			{
				if( !function_context.whole_this_is_unavailable )
				{
					// Append "this" only if whole "this" is available.
					ThisOverloadedMethodsSet this_overloaded_methods_set;
					this_overloaded_methods_set.this_= *function_context.this_;
					this_overloaded_methods_set.overloaded_methods_set= *overloaded_functions_set;
					return this_overloaded_methods_set;
				}
			}
		}
	}
	else if( const StoredVariablePtr referenced_variable= name_entry->second.GetStoredVariable() )
	{
		// Unwrap stored variable here.
		Variable result;
		result= referenced_variable->content;
		if( referenced_variable->kind == StoredVariable::Kind::Variable ||
			referenced_variable->kind == StoredVariable::Kind::ReferenceArg )
		{
			if( function_context.variables_state.VariableIsMoved( referenced_variable ) )
				errors_.push_back( ReportAccessingMovedVariable( named_operand.file_pos_, referenced_variable->name ) );

			result.referenced_variables.emplace( referenced_variable );

			// If we have mutable reference to variable, we can not access variable itself.
			if( referenced_variable->content.value_type == ValueType::Reference && referenced_variable->mut_use_counter.use_count() >= 2u )
				errors_.push_back( ReportAccessingVariableThatHaveMutableReference( named_operand.file_pos_, referenced_variable->name ) );
		}
		else if( referenced_variable->kind == StoredVariable::Kind::ArgInnerVariable )
		{
			// If we have mutable reference to variable, we can not access variable itself.
			if( referenced_variable->content.value_type == ValueType::Reference && referenced_variable->mut_use_counter.use_count() >= 2u )
				errors_.push_back( ReportAccessingVariableThatHaveMutableReference( named_operand.file_pos_, referenced_variable->name ) );
		}

		return Value( result, name_entry->second.GetFilePos() );
	}

	return name_entry->second;
}

Value CodeBuilder::BuildMoveOpeator( const Synt::MoveOperator& move_operator, NamesScope& names, FunctionContext& function_context )
{
	Synt::ComplexName complex_name;
	complex_name.components.emplace_back();
	complex_name.components.back().name= move_operator.var_name_;
	complex_name.components.back().is_generated= true;

	const NamesScope::InsertedName* const resolved_name= ResolveName( move_operator.file_pos_, names, complex_name );
	if( resolved_name == nullptr )
	{
		errors_.push_back( ReportNameNotFound( move_operator.file_pos_, move_operator.var_name_ ) );
		return ErrorValue();
	}
	const StoredVariablePtr variable_for_move= resolved_name->second.GetStoredVariable();
	if( variable_for_move == nullptr ||
		variable_for_move->kind != StoredVariable::Kind::Variable )
	{
		errors_.push_back( ReportExpectedVariable( move_operator.file_pos_, resolved_name->second.GetType().ToString() ) );
		return ErrorValue();
	}

	// TODO - maybe allow moving for immutable variables?
	if( variable_for_move->content.value_type != ValueType::Reference )
	{
		errors_.push_back( ReportExpectedReferenceValue( move_operator.file_pos_ ) );
		return ErrorValue();
	}
	if( function_context.variables_state.VariableIsMoved( variable_for_move ) )
	{
		errors_.push_back( ReportAccessingMovedVariable( move_operator.file_pos_, variable_for_move->name ) );
		return ErrorValue();
	}

	// If this is mutable variable - it is stack variable or value argument.
	// This can not be temp variable, global variable, or inner argument variable.

	if( variable_for_move->mut_use_counter.use_count() > 1u || variable_for_move->imut_use_counter.use_count() > 1u )
	{
		errors_.push_back( ReportMovedVariableHaveReferences( move_operator.file_pos_, variable_for_move->name ) );
		return ErrorValue();
	}

	Variable content= variable_for_move->content;
	content.value_type= ValueType::Value;
	content.referenced_variables.clear();

	const StoredVariablePtr moved_result= std::make_shared<StoredVariable>( "_moved_"_SpC + variable_for_move->name, content );
	content.referenced_variables.emplace( moved_result );
	function_context.stack_variables_stack.back()->RegisterVariable( moved_result );

	// We must save inner references of moved variable.
	for( const auto& inner_variable : function_context.variables_state.GetVariableReferences( variable_for_move ) )
	{
		const bool ok= function_context.variables_state.AddPollution( moved_result, inner_variable.first, inner_variable.second.IsMutable() );
		if( !ok )
			errors_.push_back( ReportReferenceProtectionError( move_operator.file_pos_, inner_variable.first->name ) );
	}
	function_context.variables_state.Move( variable_for_move );

	return Value( content, move_operator.file_pos_ );
}

Value CodeBuilder::BuildNumericConstant(
	const Synt::NumericConstant& numeric_constant,
	FunctionContext& function_context )
{
	const U_FundamentalType type= GetNumericConstantType( numeric_constant );
	if( type == U_FundamentalType::InvalidType )
	{
		errors_.push_back( ReportUnknownNumericConstantType( numeric_constant.file_pos_, numeric_constant.type_suffix_ ) );
		return ErrorValue();
	}
	llvm::Type* const llvm_type= GetFundamentalLLVMType( type );

	Variable result;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;
	result.type= FundamentalType( type, llvm_type );

	if( IsInteger( type ) )
		result.constexpr_value=
			llvm::Constant::getIntegerValue( llvm_type, llvm::APInt( llvm_type->getIntegerBitWidth(), uint64_t(numeric_constant.value_) ) );
	else if( IsFloatingPoint( type ) )
		result.constexpr_value=
			llvm::ConstantFP::get( llvm_type, static_cast<double>( numeric_constant.value_) );
	else
		U_ASSERT(false);

	result.llvm_value= result.constexpr_value;

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( "numeric constant"_SpC, result );
	result.referenced_variables.emplace( stored_result );
	function_context.stack_variables_stack.back()->RegisterVariable( stored_result );

	return Value( result, numeric_constant.file_pos_ );
}

Variable CodeBuilder::BuildBooleanConstant(
	const Synt::BooleanConstant& boolean_constant,
	FunctionContext& function_context )
{
	Variable result;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;
	result.type= bool_type_;

	result.llvm_value= result.constexpr_value=
		llvm::Constant::getIntegerValue(
			fundamental_llvm_types_.bool_ ,
			llvm::APInt( 1u, uint64_t(boolean_constant.value_) ) );

	const StoredVariablePtr stored_result=
		std::make_shared<StoredVariable>(
			Keyword( boolean_constant.value_ ? Keywords::true_ : Keywords::false_ ),
			result );
	result.referenced_variables.emplace( stored_result );
	function_context.stack_variables_stack.back()->RegisterVariable( stored_result );

	return result;
}

Value CodeBuilder::BuildIndexationOperator(
	const Value& value,
	const Synt::IndexationOperator& indexation_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);

	if( value.GetType() == NontypeStub::TemplateDependentValue )
	{
		BuildExpressionCode(
			*indexation_operator.index_,
			names,
			function_context );
		return value;
	}
	if( value.GetType().GetTemplateDependentType() != nullptr )
	{
		BuildExpressionCode(
			*indexation_operator.index_,
			names,
			function_context );
		Variable result;
		result.value_type= value.GetVariable()->value_type;
		result.type= GetNextTemplateDependentType();
		return value;
	}

	if( value.GetVariable() == nullptr )
	{
		errors_.push_back( ReportExpectedVariable( indexation_operator.file_pos_, value.GetType().ToString() ) );
		return ErrorValue();
	}

	const Variable& variable= *value.GetVariable();

	if( variable.type.GetClassType() != nullptr ) // If this is class - try call overloaded [] operator.
	{
		std::vector<Function::Arg> args;
		args.reserve( 2u );
		const size_t error_count_before= errors_.size();

		args.emplace_back();
		args.back().type= variable.type;
		args.back().is_reference= variable.value_type != ValueType::Value;
		args.back().is_mutable= variable.value_type == ValueType::Reference;

		// Know type of index.
		{
			// Prepare dummy function context for first pass.
			FunctionContext dummy_function_context(
				function_context.return_type,
				function_context.return_value_is_mutable,
				function_context.return_value_is_reference,
				llvm_context_,
				dummy_function_context_->function );
			const StackVariablesStorage dummy_stack_variables_storage( dummy_function_context );
			dummy_function_context.this_= function_context.this_;
			dummy_function_context.whole_this_is_unavailable= function_context.whole_this_is_unavailable;
			dummy_function_context.variables_state= function_context.variables_state;
			function_context.variables_state.DeactivateLocks();

			const Value index_value= BuildExpressionCode( *indexation_operator.index_, names, dummy_function_context );

			function_context.overloading_resolutin_cache.insert(
				dummy_function_context.overloading_resolutin_cache.begin(),
				dummy_function_context.overloading_resolutin_cache.end() );

			CHECK_RETURN_ERROR_VALUE(index_value);
			CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(index_value);

			const Variable* const index_variable= index_value.GetVariable();
			if( index_variable == nullptr )
			{
				errors_.push_back( ReportExpectedVariable( indexation_operator.index_->GetFilePos(), index_value.GetType().ToString() ) );
				return ErrorValue();
			}

			args.emplace_back();
			args.back().type= index_variable->type;
			args.back().is_reference= index_variable->value_type != ValueType::Value;
			args.back().is_mutable= index_variable->value_type == ValueType::Reference;
		}
		errors_.resize( error_count_before );
		function_context.variables_state.ActivateLocks();

		const FunctionVariable* const overloaded_operator=
			GetOverloadedOperator( args, OverloadedOperator::Indexing, indexation_operator.file_pos_ );
		if( overloaded_operator != nullptr )
		{
			if( overloaded_operator->is_this_call && overloaded_operator->virtual_table_index != ~0u  )
			{
				const auto fetch_result= TryFetchVirtualFunction( variable, *overloaded_operator, function_context );
				return
					DoCallFunction(
						fetch_result.second,
						*overloaded_operator->type.GetFunctionType(),
						indexation_operator.file_pos_,
						&fetch_result.first, { indexation_operator.index_.get() }, false,
						names, function_context );
			}
			else
				return
					DoCallFunction(
						overloaded_operator->llvm_function,
						*overloaded_operator->type.GetFunctionType(),
						indexation_operator.file_pos_,
						&variable, { indexation_operator.index_.get() }, false,
						names, function_context );

			function_context.overloading_resolutin_cache[ &indexation_operator ]= *overloaded_operator;
		}
		else
			function_context.overloading_resolutin_cache[ &indexation_operator ]= boost::none;
	}

	const Array* const array_type= variable.type.GetArrayType();
	if( array_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( indexation_operator.file_pos_, value.GetType().ToString() ) );
		return ErrorValue();
	}

	// Lock array. We must prevent modification of array in index calcualtion.
	std::vector<VariableStorageUseCounter> array_locks;
	for( const StoredVariablePtr& stored_variable : variable.referenced_variables )
		array_locks.push_back( variable.value_type == ValueType::Reference ? stored_variable->mut_use_counter : stored_variable->imut_use_counter );

	const Value index_value=
		BuildExpressionCode(
			*indexation_operator.index_,
			names,
			function_context );
	CHECK_RETURN_ERROR_VALUE(index_value);
	CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(index_value);

	if( index_value.GetType().GetTemplateDependentType() != nullptr )
	{
		Variable result;
		result.location= Variable::Location::Pointer;
		result.value_type= variable.value_type;
		result.referenced_variables= variable.referenced_variables;
		result.type= array_type->type;
		return Value( result, indexation_operator.file_pos_ );
	}

	const FundamentalType* const index_fundamental_type= index_value.GetType().GetFundamentalType();
	if( index_fundamental_type == nullptr ||
		!IsUnsignedInteger( index_fundamental_type->fundamental_type ) )
	{
		errors_.push_back( ReportTypesMismatch( indexation_operator.file_pos_, "any unsigned integer"_SpC, index_value.GetType().ToString() ) );
		return ErrorValue();
	}
	const Variable& index= *index_value.GetVariable();

	if( variable.location != Variable::Location::Pointer )
	{
		// TODO - Strange variable location.
		return ErrorValue();
	}

	// If index is constant and not undefined and array size is not undefined - statically check index.
	if( index.constexpr_value != nullptr && llvm::dyn_cast<llvm::UndefValue>(index.constexpr_value) == nullptr &&
		array_type->size != Array::c_undefined_size )
	{
		const SizeType index_value= SizeType( index.constexpr_value->getUniqueInteger().getLimitedValue() );
		if( index_value >= array_type->size )
			errors_.push_back( ReportArrayIndexOutOfBounds( indexation_operator.file_pos_, index_value, array_type->size ) );
	}

	Variable result;
	result.location= Variable::Location::Pointer;
	result.value_type= variable.value_type == ValueType::Reference ? ValueType::Reference : ValueType::ConstReference;
	result.referenced_variables= variable.referenced_variables;
	result.type= array_type->type;

	if( variable.constexpr_value != nullptr && index.constexpr_value != nullptr )
	{
		if( llvm::dyn_cast<llvm::UndefValue>(variable.constexpr_value) != nullptr ||
			llvm::dyn_cast<llvm::UndefValue>(index.constexpr_value) != nullptr )
			result.constexpr_value= llvm::UndefValue::get( array_type->llvm_type )->getElementValue( index.constexpr_value );
		else
			result.constexpr_value= variable.constexpr_value->getAggregateElement( index.constexpr_value );
	}

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
	index_list[1]= CreateMoveToLLVMRegisterInstruction( index, function_context );

	// If index is not const and array size is not undefined - check bounds.
	if( index.constexpr_value == nullptr && array_type->size != Array::c_undefined_size )
	{
		const Type size_type= FundamentalType( U_FundamentalType::u32, fundamental_llvm_types_.u32 ); // SPRACHE_TODO - use native size_type.

		llvm::Value* index_value= index_list[1];
		if( index.type.SizeOf() > size_type.SizeOf() )
			index_value= function_context.llvm_ir_builder.CreateTrunc( index_value, fundamental_llvm_types_.i32 );
		else if( index.type.SizeOf() < size_type.SizeOf() )
			index_value= function_context.llvm_ir_builder.CreateZExt( index_value, fundamental_llvm_types_.i32 );

		llvm::Value* const condition=
			function_context.llvm_ir_builder.CreateICmpUGE( // if( index >= array_size ) {halt;}
				index_value,
				llvm::Constant::getIntegerValue( size_type.GetLLVMType(), llvm::APInt( 32u, array_type->size ) ) );

		llvm::BasicBlock* const halt_block= llvm::BasicBlock::Create( llvm_context_ );
		llvm::BasicBlock* const block_after_if= llvm::BasicBlock::Create( llvm_context_ );
		function_context.llvm_ir_builder.CreateCondBr( condition, halt_block, block_after_if );

		function_context.function->getBasicBlockList().push_back( halt_block );
		function_context.llvm_ir_builder.SetInsertPoint( halt_block );
		function_context.llvm_ir_builder.CreateCall( halt_func_ );
		function_context.llvm_ir_builder.CreateRetVoid(); // terminate block.

		function_context.function->getBasicBlockList().push_back( block_after_if );
		function_context.llvm_ir_builder.SetInsertPoint( block_after_if );
	}

	result.llvm_value=
		function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef< llvm::Value*> ( index_list, 2u ) );

	return Value( result, indexation_operator.file_pos_ );
}

Value CodeBuilder::BuildMemberAccessOperator(
	const Value& value,
	const Synt::MemberAccessOperator& member_access_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);
	CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(value);
	if( value.GetType().GetTemplateDependentType() != nullptr )
		return TemplateDependentValue();

	Class* const class_type= value.GetType().GetClassType();
	if( class_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( member_access_operator.file_pos_, value.GetType().ToString() ) );
		return ErrorValue();
	}

	if( class_type->completeness != Class::Completeness::Complete )
	{
		errors_.push_back( ReportUsingIncompleteType( member_access_operator.file_pos_, value.GetType().ToString() ) );
		return ErrorValue();
	}

	const Variable& variable= *value.GetVariable();

	const NamesScope::InsertedName* const class_member= class_type->members.GetThisScopeName( member_access_operator.member_name_ );
	if( class_member == nullptr )
	{
		errors_.push_back( ReportNameNotFound( member_access_operator.file_pos_, member_access_operator.member_name_ ) );
		return ErrorValue();
	}

	if( !function_context.is_in_unsafe_block &&
		( member_access_operator.member_name_ == Keywords::constructor_ || member_access_operator.member_name_ == Keywords::destructor_ ) )
		errors_.push_back( ReportExplicitAccessToThisMethodIsUnsafe( member_access_operator.file_pos_,  member_access_operator.member_name_ ) );

	if( names.GetAccessFor( value.GetType().GetClassTypeProxy() ) < class_type->GetMemberVisibility( member_access_operator.member_name_ ) )
		errors_.push_back( ReportAccessingNonpublicClassMember( member_access_operator.file_pos_, class_type->members.GetThisNamespaceName(), member_access_operator.member_name_ ) );

	if( const OverloadedFunctionsSet* functions_set= class_member->second.GetFunctionsSet() )
	{
		if( member_access_operator.have_template_parameters )
		{
			if( functions_set->template_functions.empty() )
				errors_.push_back( ReportValueIsNotTemplate( member_access_operator.file_pos_ ) );
			else
			{
				const NamesScope::InsertedName* const inserted_name=
					GenTemplateFunctionsUsingTemplateParameters(
						member_access_operator.file_pos_,
						functions_set->template_functions,
						member_access_operator.template_parameters,
						class_type->members,
						names );
				if( inserted_name == nullptr )
					return ErrorValue();

				functions_set= inserted_name->second.GetFunctionsSet();
			}
		}
		ThisOverloadedMethodsSet this_overloaded_methods_set;
		this_overloaded_methods_set.this_= variable;
		this_overloaded_methods_set.overloaded_methods_set= *functions_set;
		return this_overloaded_methods_set;
	}

	if( member_access_operator.have_template_parameters )
		errors_.push_back( ReportValueIsNotTemplate( member_access_operator.file_pos_ ) );

	const ClassField* const field= class_member->second.GetClassField();
	if( field == nullptr )
	{
		errors_.push_back( ReportNotImplemented( member_access_operator.file_pos_, "class members, except fields or methods" ) );
		return ErrorValue();
	}

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

	const ClassProxyPtr field_class_proxy= field->class_.lock();
	U_ASSERT( field_class_proxy != nullptr );

	llvm::Value* actual_field_class_ptr= nullptr;
	if( field_class_proxy == value.GetType().GetClassTypeProxy() )
		actual_field_class_ptr= variable.llvm_value;
	else
	{
		// For parent filed we needs make several GEP isntructions.
		ClassProxyPtr actual_field_class= value.GetType().GetClassTypeProxy();
		actual_field_class_ptr= variable.llvm_value;
		while( actual_field_class != field_class_proxy )
		{
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(actual_field_class->class_->base_class_field_number) ) );
			actual_field_class_ptr= function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );

			actual_field_class= actual_field_class->class_->base_class;
			U_ASSERT(actual_field_class != nullptr );
		}
	}

	index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );

	Variable result;
	result.location= Variable::Location::Pointer;
	result.value_type= ( variable.value_type == ValueType::Reference && field->is_mutable ) ? ValueType::Reference : ValueType::ConstReference;
	result.referenced_variables= variable.referenced_variables;
	result.type= field->type;
	result.llvm_value=
		function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );

	if( variable.constexpr_value != nullptr )
	{
		result.constexpr_value= variable.constexpr_value->getAggregateElement( static_cast<unsigned int>( field->index ) );
		if( field->is_reference )
		{
			// TODO - what if storage for constexpr reference valus is not "GlobalVariable"?
			llvm::GlobalVariable* const var= llvm::dyn_cast<llvm::GlobalVariable>( result.constexpr_value );
			result.constexpr_value= var->getInitializer();
		}
	}

	if( field->is_reference )
	{
		result.value_type= field->is_mutable ? ValueType::Reference : ValueType::ConstReference;
		result.llvm_value= function_context.llvm_ir_builder.CreateLoad( result.llvm_value );

		result.referenced_variables.clear();
		for( const StoredVariablePtr& struct_variable : variable.referenced_variables )
		{
			for( const auto& referenced_variable_pair : function_context.variables_state.GetVariableReferences( struct_variable ) )
				result.referenced_variables.insert( referenced_variable_pair.first );
		}
	}

	return Value( result, member_access_operator.file_pos_ );
}

Value CodeBuilder::BuildCallOperator(
	const Value& function_value,
	const Synt::CallOperator& call_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(function_value);

	if( function_value.GetType() == NontypeStub::TemplateDependentValue ||
		function_value.GetType().GetTemplateDependentType() != nullptr)
	{
		for( const Synt::IExpressionComponentPtr& arg_expression : call_operator.arguments_ )
			BuildExpressionCode( *arg_expression, names, function_context );
		Variable result;
		result.type= GetNextTemplateDependentType();
		return Value( result, call_operator.file_pos_ );
	}

	if( const Type* const type= function_value.GetTypeName() )
		return Value( BuildTempVariableConstruction( *type, call_operator, names, function_context ), call_operator.file_pos_ );

	const Variable* this_= nullptr;
	const OverloadedFunctionsSet* functions_set= function_value.GetFunctionsSet();

	if( functions_set != nullptr )
	{}
	else if( const ThisOverloadedMethodsSet* const this_overloaded_methods_set=
		function_value.GetThisOverloadedMethodsSet() )
	{
		functions_set= &this_overloaded_methods_set->overloaded_methods_set;
		this_= &this_overloaded_methods_set->this_;
	}
	else if( const Variable* const callable_variable= function_value.GetVariable() )
	{
		if( const Class* const class_type= callable_variable->type.GetClassType() )
		{
			// For classes try to find () operator inside it.
			if( const NamesScope::InsertedName* const name=
				class_type->members.GetThisScopeName( OverloadedOperatorToString( OverloadedOperator::Call ) ) )
			{
				functions_set= name->second.GetFunctionsSet();
				U_ASSERT( functions_set != nullptr ); // If we found (), this must be functions set.
				this_= callable_variable;
				// SPRACHE_TODO - maybe support not only thiscall () operators ?
			}
		}
		else if( const FunctionPointer* const function_pointer= callable_variable->type.GetFunctionPointerType() )
		{
			// Call function pointer directly.
			std::vector<const Synt::IExpressionComponent*> args;
			for( const auto& arg : call_operator.arguments_ )
				args.push_back( arg.get() );

			llvm::Value* const func_itself= CreateMoveToLLVMRegisterInstruction( *callable_variable, function_context );

			return
				DoCallFunction(
					func_itself, function_pointer->function, call_operator.file_pos_,
					nullptr, args, false,
					names, function_context );
		}
	}

	if( functions_set == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( call_operator.file_pos_, function_value.GetType().ToString() ) );
		return ErrorValue();
	}

	size_t this_count= this_ == nullptr ? 0u : 1u;
	size_t total_args= this_count + call_operator.arguments_.size();

	const size_t error_count_before= errors_.size();

	const FunctionVariable* function_ptr= nullptr;

	// Make preevaluation af arguments for selection of overloaded function. Try also get function from cache.
	const auto cache_it= function_context.overloading_resolutin_cache.find( &call_operator );
	if( cache_it != function_context.overloading_resolutin_cache.end() &&
		!call_operator.arguments_.empty() ) // empty check - hack for dummy call operator from empty initializer.
		function_ptr= cache_it->second.get_ptr();
	else
	{
		std::vector<Function::Arg> actual_args;
		actual_args.reserve( total_args );

		bool args_are_template_dependent= false;

		// Prepare dummy function context for first pass.
		FunctionContext dummy_function_context(
			function_context.return_type,
			function_context.return_value_is_mutable,
			function_context.return_value_is_reference,
			llvm_context_,
			dummy_function_context_->function );
		const StackVariablesStorage dummy_stack_variables_storage( dummy_function_context );
		dummy_function_context.this_= function_context.this_;
		dummy_function_context.whole_this_is_unavailable= function_context.whole_this_is_unavailable;
		dummy_function_context.variables_state= function_context.variables_state; // TODO - support copy-on-write for variables_state
		function_context.variables_state.DeactivateLocks();

		// Push "this" argument.
		if( this_ != nullptr )
		{
			actual_args.emplace_back();
			actual_args.back().type= this_->type;
			actual_args.back().is_reference= true;
			actual_args.back().is_mutable= this_->value_type == ValueType::Reference;
		}
		// Push arguments from call operator.
		for( const Synt::IExpressionComponentPtr& arg_expression : call_operator.arguments_ )
		{
			U_ASSERT( arg_expression != nullptr );
			const Value expr_value= BuildExpressionCode( *arg_expression, names, dummy_function_context );
			CHECK_RETURN_ERROR_VALUE(expr_value);

			if( expr_value.GetType() == NontypeStub::TemplateDependentValue )
			{
				args_are_template_dependent= true;
				continue;
			}

			const Variable* const expr= expr_value.GetVariable();
			if( expr == nullptr )
			{
				errors_.push_back( ReportExpectedVariable( arg_expression->GetFilePos(), expr_value.GetType().ToString() ) );
				return ErrorValue();
			}

			actual_args.emplace_back();
			actual_args.back().type= expr->type;
			actual_args.back().is_reference= expr->value_type != ValueType::Value;
			actual_args.back().is_mutable= expr->value_type == ValueType::Reference;
		}

		function_context.overloading_resolutin_cache.insert(
			dummy_function_context.overloading_resolutin_cache.begin(),
			dummy_function_context.overloading_resolutin_cache.end() );

		if( args_are_template_dependent )
			return Value( Type(NontypeStub::TemplateDependentValue), call_operator.file_pos_ );

		function_ptr=
			GetOverloadedFunction( *functions_set, actual_args, this_ != nullptr, call_operator.file_pos_ );

		if( function_ptr == nullptr )
			function_context.overloading_resolutin_cache[ &call_operator ]= boost::none;
		else
			function_context.overloading_resolutin_cache[ &call_operator ]= *function_ptr;
	}
	function_context.variables_state.ActivateLocks();

	// SPRACHE_TODO - try get function with "this" parameter in signature and without it.
	// We must support static functions call using "this".
	if( function_ptr == nullptr )
		return ErrorValue();
	const FunctionVariable& function= *function_ptr;
	const Function& function_type= *function.type.GetFunctionType();

	if( this_ != nullptr && !function.is_this_call )
	{
		// Static function call via "this".
		// Just dump first "this" arg.
		this_count--;
		total_args--;
		this_= nullptr;
	}

	if( this_ == nullptr && function.is_this_call )
	{
		errors_.push_back( ReportCallOfThiscallFunctionUsingNonthisArgument( call_operator.file_pos_ ) );
		return ErrorValue();
	}

	errors_.resize( error_count_before ); // Drop errors from first pass.

	std::vector<const Synt::IExpressionComponent*> synt_args;
	for( const Synt::IExpressionComponentPtr& arg : call_operator.arguments_ )
		synt_args.push_back( arg.get() );

	Variable this_casted;
	llvm::Value* llvm_function_ptr= function.llvm_function;
	if( this_ != nullptr )
	{
		auto fetch_result= TryFetchVirtualFunction( *this_, function, function_context );
		this_casted= std::move( fetch_result.first );
		llvm_function_ptr= fetch_result.second;
		this_= &this_casted;
	}

	return DoCallFunction( llvm_function_ptr, function_type, call_operator.file_pos_, this_, synt_args, false, names, function_context );
}

Value CodeBuilder::DoCallFunction(
	llvm::Value* function,
	const Function& function_type,
	const FilePos& call_file_pos,
	const Variable* first_arg,
	std::vector<const Synt::IExpressionComponent*> args,
	const bool evaluate_args_in_reverse_order,
	NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( !( evaluate_args_in_reverse_order && first_arg != nullptr ) );

	if( function_type.unsafe && !function_context.is_in_unsafe_block )
		errors_.push_back( ReportUnsafeFunctionCallOutsideUnsafeBlock( call_file_pos ) );

	const size_t first_arg_count= first_arg == 0u ? 0u : 1u;
	const size_t arg_count= args.size() + first_arg_count;
	U_ASSERT( arg_count == function_type.args.size() );

	std::vector<llvm::Value*> llvm_args;
	llvm_args.resize( arg_count, nullptr );
	std::unordered_map<StoredVariablePtr, VaraibleReferencesCounter> locked_variable_counters;
	std::vector<VariableStorageUseCounter> temp_args_locks; // We need lock reference argument before evaluating next arguments.
	std::vector< std::unordered_set<StoredVariablePtr> > arg_to_variables( arg_count );
	std::vector< std::pair< std::unordered_set<StoredVariablePtr>, bool > > arg_to_inner_variables( arg_count ); // second param - is mutable

	bool function_result_have_template_dependent_type= false;
	for( unsigned int i= 0u; i < arg_count; i++ )
	{
		const unsigned int j= evaluate_args_in_reverse_order ? arg_count - i - 1u : i;

		const bool is_first_arg= first_arg != nullptr && j == 0u;
		const Function::Arg& arg= function_type.args[j];

		Variable expr;
		if( is_first_arg )
			expr= *first_arg;
		else
			expr= *BuildExpressionCode( *args[ j - first_arg_count ], names, function_context ).GetVariable();

		const FilePos& file_pos= is_first_arg ? file_pos : args[ j - first_arg_count ]->GetFilePos();

		const bool something_have_template_dependent_type= expr.type.GetTemplateDependentType() != nullptr || arg.type.GetTemplateDependentType() != nullptr;
		function_result_have_template_dependent_type= function_result_have_template_dependent_type || something_have_template_dependent_type;

		if( arg.is_reference )
		{
			if( !something_have_template_dependent_type && !expr.type.ReferenceIsConvertibleTo(arg.type) )
			{
				errors_.push_back( ReportTypesMismatch( file_pos, arg.type.ToString(), expr.type.ToString() ) );
				return ErrorValue();
			}

			if( arg.is_mutable )
			{
				if( expr.value_type == ValueType::Value )
				{
					errors_.push_back( ReportExpectedReferenceValue( file_pos ) );
					return ErrorValue();
				}
				if( expr.value_type == ValueType::ConstReference )
				{
					errors_.push_back( ReportBindingConstReferenceToNonconstReference( file_pos ) );
					return ErrorValue();
				}

				if( expr.type == arg.type )
					llvm_args[j]= expr.llvm_value;
				else
					llvm_args[j]= CreateReferenceCast( expr.llvm_value, expr.type, arg.type, function_context );

				// Lock references.
				for( const StoredVariablePtr& referenced_variable : expr.referenced_variables )
				{
					++locked_variable_counters[referenced_variable].mut;
					temp_args_locks.push_back( referenced_variable->mut_use_counter );
					arg_to_variables[j].emplace( referenced_variable );
				}
			}
			else
			{
				if( expr.value_type == ValueType::Value && expr.location == Variable::Location::LLVMRegister && !something_have_template_dependent_type )
				{
					// Bind value to const reference.
					// TODO - support nonfundamental values.
					llvm::Value* const temp_storage= function_context.alloca_ir_builder.CreateAlloca( expr.type.GetLLVMType() );
					function_context.llvm_ir_builder.CreateStore( expr.llvm_value, temp_storage );
					llvm_args[j]= temp_storage;
				}
				else
					llvm_args[j]= expr.llvm_value;

				if( expr.type != arg.type )
					llvm_args[j]= CreateReferenceCast( llvm_args[j], expr.type, arg.type, function_context );

				// Lock references.
				arg_to_inner_variables[j].second= false; // Non-mutable
				for( const StoredVariablePtr& referenced_variable : expr.referenced_variables )
				{
					++locked_variable_counters[referenced_variable].imut;
					temp_args_locks.push_back( referenced_variable->imut_use_counter );
					arg_to_variables[j].emplace( referenced_variable );

					for( const auto& inner_variable_pair : function_context.variables_state.GetVariableReferences( referenced_variable ) )
					{
						arg_to_inner_variables[j].second= arg_to_inner_variables[j].second || inner_variable_pair.second.IsMutable();
						arg_to_inner_variables[j].first.emplace( inner_variable_pair.first );
					}
				}
			}
		}
		else
		{
			if( !something_have_template_dependent_type && !expr.type.ReferenceIsConvertibleTo( arg.type ) )
			{
				errors_.push_back( ReportTypesMismatch( file_pos, arg.type.ToString(), expr.type.ToString() ) );
				return ErrorValue();
			}

			if( arg.type.GetFundamentalType() != nullptr || arg.type.GetEnumType() != nullptr || arg.type.GetFunctionPointerType() != nullptr )
			{
				if( !something_have_template_dependent_type )
					llvm_args[j]= CreateMoveToLLVMRegisterInstruction( expr, function_context );
			}
			else if( const ClassProxyPtr class_type= arg.type.GetClassTypeProxy() )
			{
				// Save references inside referenced variables, because we need check references inside it.
				// Do it only if arg type can contain any reference inside.
				// Do it before potential moving.
				if( arg.type.ReferencesTagsCount() > 0u )
				{
					arg_to_inner_variables[j].second= false; // Non-mutable
					for( const StoredVariablePtr& var_itself : expr.referenced_variables )
					{
						for( const auto& referenced_variable_pair : function_context.variables_state.GetVariableReferences( var_itself ) )
						{
							const StoredVariablePtr& referenced_variable = referenced_variable_pair.first;
							if( referenced_variable_pair.second.IsMutable() )
							{
								++locked_variable_counters[referenced_variable].mut;
								temp_args_locks.push_back( referenced_variable->mut_use_counter );
							}
							else
							{
								++locked_variable_counters[referenced_variable].imut;
								temp_args_locks.push_back( referenced_variable->imut_use_counter );
							}
							arg_to_inner_variables[j].second= arg_to_inner_variables[j].second || referenced_variable_pair.second.IsMutable();
							arg_to_inner_variables[j].first.emplace( referenced_variable );
						}
						//arg_to_variables[j].emplace( var_itself );
					}
				}

				if( !arg.type.IsCopyConstructible() )
				{
					// Can not call function with value parameter, because for value parameter needs copy, but parameter type is not copyable.
					// TODO - print more reliable message.
					errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, arg.type.ToString() ) );
					continue;
				}

				if( something_have_template_dependent_type )
				{}
				else if( expr.value_type == ValueType::Value && expr.type == arg.type )
				{
					// Do not call copy constructors - just move.
					U_ASSERT( expr.referenced_variables.size() == 1u );
					function_context.variables_state.Move( *expr.referenced_variables.begin() );
					llvm_args[j]= expr.llvm_value;
				}
				else
				{
					// Create copy of class value. Call copy constructor.
					llvm::Value* const arg_copy= function_context.alloca_ir_builder.CreateAlloca( arg.type.GetLLVMType() );

					llvm::Value* value_for_copy= expr.llvm_value;
					if( expr.type != arg.type )
						value_for_copy= CreateReferenceCast( value_for_copy, expr.type, arg.type, function_context );
					TryCallCopyConstructor( file_pos, arg_copy, value_for_copy, class_type, function_context );
					llvm_args[j]= arg_copy;
				}
			}
			else if( something_have_template_dependent_type )
			{}
			else
				U_ASSERT( false );
		}
	} // for args

	// Check references.
	temp_args_locks.clear(); // clear temporary locks.
	for( const auto& pair : locked_variable_counters )
	{
		// Check references, passed into function.
		const VaraibleReferencesCounter& counter= pair.second;
		if( counter.mut == 1u && counter.imut == 0u )
		{} // All ok - one mutable reference.
		else if( counter.mut == 0u )
		{} // All ok - 0-infinity immutable references.
		else
		{
			errors_.push_back( ReportReferenceProtectionError( call_file_pos, pair.first->name ) );
			continue;
		}

		// Check interaction between references, passed into function and references on stack.
		const StoredVariable& var= *pair.first;
		if( counter.mut == 1u &&
			( var.imut_use_counter.use_count() > 1u || var.mut_use_counter.use_count() > 2u ) )
		{
			// Pass mutable reference into function, while there are references on stack or somewhere else.
			// We can have one mutable reference on stack, but no more.
			errors_.push_back( ReportReferenceProtectionError( call_file_pos, var.name ) );
		}
		if( counter.mut == 1u && var.mut_use_counter.use_count() == 2u )
		{} // Ok - we take one mutable reference from stack and pass it into function.
		if( counter.mut == 0u && var.imut_use_counter.use_count() > 1u )
		{} // Ok - pass immutable references into function, while mutable references on stack exists.
	}

	const bool return_value_is_sret= function_type.return_type.GetClassType() != nullptr && !function_type.return_value_is_reference;

	if( function_result_have_template_dependent_type )
	{
		Variable dummy_result;
		if( function_type.return_value_is_reference )
		{
			dummy_result.location= Variable::Location::Pointer;
			if( function_type.return_value_is_mutable )
				dummy_result.value_type= ValueType::Reference;
			else
				dummy_result.value_type= ValueType::ConstReference;
		}
		else
			dummy_result.location= return_value_is_sret ? Variable::Location::Pointer : Variable::Location::LLVMRegister;
		dummy_result.type= GetNextTemplateDependentType();
		return Value( dummy_result, call_file_pos );
	}

	llvm::Value* s_ret_value= nullptr;
	if( return_value_is_sret )
	{
		s_ret_value= function_context.alloca_ir_builder.CreateAlloca( function_type.return_type.GetLLVMType() );
		llvm_args.insert( llvm_args.begin(), s_ret_value );
	}

	llvm::Value* call_result=
		function_context.llvm_ir_builder.CreateCall(
			function,
			llvm_args );

	if( return_value_is_sret )
	{
		U_ASSERT( s_ret_value != nullptr );
		call_result= s_ret_value;
	}

	Variable result;

	result.type= function_type.return_type;
	result.llvm_value= call_result;
	if( function_type.return_value_is_reference )
	{
		result.location= Variable::Location::Pointer;
		if( function_type.return_value_is_mutable )
			result.value_type= ValueType::Reference;
		else
			result.value_type= ValueType::ConstReference;
	}
	else
	{
		if( function_type.return_type != void_type_ && function_type.return_type.IsIncomplete() )
			errors_.push_back( ReportUsingIncompleteType( call_file_pos, function_type.return_type.ToString() ) );

		result.location= return_value_is_sret ? Variable::Location::Pointer : Variable::Location::LLVMRegister;
		result.value_type= ValueType::Value;

		const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( "fn result"_SpC, result );
		result.referenced_variables.emplace( stored_result );

		function_context.stack_variables_stack.back()->RegisterVariable( stored_result );
	}

	// Prepare reference result.
	if( function_type.return_value_is_reference )
	{
		// Returned reference refers to args, listed in function type.
		U_ASSERT( arg_to_variables.size() == function_type.args.size() );
		for( const size_t arg_n : function_type.return_references.args_references )
		{
			U_ASSERT( arg_n < arg_to_variables.size() );
			if( function_type.args[ arg_n ].is_reference )
			{
				for( const StoredVariablePtr& var : arg_to_variables[arg_n] )
					result.referenced_variables.emplace(var);
			}
		}

		// Returned reference also may be linked with references, passed inside structs.
		for( const Function::ArgReference& arg_n_and_tag_n : function_type.return_references.inner_args_references )
		{
			U_ASSERT( arg_n_and_tag_n.first < arg_to_variables.size() );
			U_ASSERT( arg_n_and_tag_n.second == 0u ); // Currently, support only 0 or 1 tags

			for( const StoredVariablePtr& var : arg_to_inner_variables[arg_n_and_tag_n.first].first )
			{
				result.referenced_variables.emplace(var);
				for( const StoredVariablePtr& referenced_variable : function_context.variables_state.RecursiveGetAllReferencedVariables(var).variables )
					result.referenced_variables.emplace(referenced_variable);
			}
		}
	}
	else if( function_type.return_type.ReferencesTagsCount() > 0u )
	{
		U_ASSERT( result.referenced_variables.size() == 1u );
		const StoredVariablePtr& stored_result= *result.referenced_variables.begin();

		// Returned value references refers to args, listed in function type.
		U_ASSERT( arg_to_variables.size() == function_type.args.size() );
		for( const size_t arg_n : function_type.return_references.args_references )
		{
			U_ASSERT( arg_n < arg_to_variables.size() );
			if( function_type.args[ arg_n ].is_reference )
			{
				const bool is_mutable= function_type.args[arg_n].is_mutable;
				for( const StoredVariablePtr& var : arg_to_variables[arg_n] )
					function_context.variables_state.AddPollution( stored_result, var, is_mutable );
			}
		}

		// Returned vale references also may be linked with references, passed inside structs.
		for( const Function::ArgReference& arg_n_and_tag_n : function_type.return_references.inner_args_references )
		{
			U_ASSERT( arg_n_and_tag_n.first < arg_to_variables.size() );
			U_ASSERT( arg_n_and_tag_n.second == 0u ); // Currently, support only 0 or 1 tags
			// TODO - if we pass value-argument, we needs to call copy constructor. In this constructor we can swap tags.
			// We need correct result in such case.

			for( const StoredVariablePtr& var : arg_to_inner_variables[arg_n_and_tag_n.first].first )
			{
				const bool is_mutable= arg_to_inner_variables[arg_n_and_tag_n.first].second;
				function_context.variables_state.AddPollution( stored_result, var, is_mutable );

				const VariablesState::AchievableVariables achievable_variables= function_context.variables_state.RecursiveGetAllReferencedVariables( var );
				for( const auto& referenced_variable_pair : achievable_variables.variables )
					function_context.variables_state.AddPollution( stored_result, referenced_variable_pair, achievable_variables.any_variable_is_mutable );
			}
		}
	}

	// Setup references after call.
	for( const Function::ReferencePollution& referene_pollution : function_type.references_pollution )
	{
		const size_t dst_arg= referene_pollution.dst.first;
		U_ASSERT( dst_arg < function_type.args.size() );
		U_ASSERT( function_type.args[ dst_arg ].type.ReferencesTagsCount() > 0u );

		bool src_variables_is_mutable= referene_pollution.src_is_mutable;
		std::unordered_set<StoredVariablePtr> src_variables;
		if( referene_pollution.src.second == Function::c_arg_reference_tag_number )
		{
			// Reference-arg itself
			U_ASSERT( function_type.args[ referene_pollution.src.first ].is_reference );
			src_variables= arg_to_variables[ referene_pollution.src.first ];

			if( !function_type.args[ referene_pollution.src.first ].is_mutable )
				src_variables_is_mutable= false; // Even if reference-pollution is mutable, but if src vars is immutable, link as immutable.
		}
		else
		{
			// Variables, referenced by inner argument references.
			U_ASSERT( referene_pollution.src.second == 0u );// Currently we support one tag per struct.
			bool all_is_mutable= false;
			for( const StoredVariablePtr& referenced_variable : arg_to_inner_variables[ referene_pollution.src.first ].first )
			{
				const VariablesState::AchievableVariables vars= function_context.variables_state.RecursiveGetAllReferencedVariables( referenced_variable );
				src_variables.insert( vars.variables.begin(), vars.variables.end() );
				src_variables.insert( referenced_variable );
				all_is_mutable= all_is_mutable || vars.any_variable_is_mutable || arg_to_inner_variables[ referene_pollution.src.first ].second;
			}

			if( !all_is_mutable )
				src_variables_is_mutable= false; // Even if reference-pollution is mutable, but if src vars is immutable, link as immutable.
		}

		if( function_type.args[ dst_arg ].is_reference )
		{
			for( const StoredVariablePtr& dst_variable : arg_to_variables[ dst_arg ] )
			{
				U_ASSERT( dst_variable->kind == StoredVariable::Kind::Variable || dst_variable->kind == StoredVariable::Kind::ArgInnerVariable  || dst_variable->kind == StoredVariable::Kind::ReferenceArg );
				if( dst_variable->kind == StoredVariable::Kind::ArgInnerVariable )
					errors_.push_back( ReportReferencePollutionForArgReference( call_file_pos ) );

				for( const StoredVariablePtr& src_variable : src_variables )
				{
					const bool ok= function_context.variables_state.AddPollution( dst_variable, src_variable, src_variables_is_mutable );
					if( !ok )
						errors_.push_back( ReportReferenceProtectionError( call_file_pos, src_variable->name ) );
				}
			} // for dst variables
		}
		else
		{
			 // Does it have sence, write references to value argument?
		}
	} // for function_type.references_pollution

	return Value( result, call_file_pos );
}

Variable CodeBuilder::BuildTempVariableConstruction(
	const Type& type,
	const Synt::CallOperator& call_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	const StoredVariablePtr stored_variable= std::make_shared<StoredVariable>( "temp "_SpC + type.ToString(), Variable() );
	function_context.stack_variables_stack.back()->RegisterVariable( stored_variable );
	Variable& variable= stored_variable->content;
	variable.type= type;
	variable.location= Variable::Location::Pointer;
	variable.value_type= ValueType::Reference;
	variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( type.GetLLVMType() );

	variable.referenced_variables.insert(stored_variable);
	variable.constexpr_value= ApplyConstructorInitializer( variable, stored_variable, call_operator, names, function_context );
	variable.referenced_variables.erase(stored_variable);
	variable.value_type= ValueType::Value; // Make value after construction

	variable.referenced_variables.emplace( stored_variable );


	return variable;
}

Value CodeBuilder::BuildUnaryMinus(
	const Value& value,
	const Synt::UnaryMinus& unary_minus,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);
	CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(value);

	if( value.GetType().GetTemplateDependentType() != nullptr )
	{
		Variable result;
		result.value_type= ValueType::Value;
		result.type= GetNextTemplateDependentType();
		return Value( result, unary_minus.file_pos_ );
	}

	const FundamentalType* const fundamental_type= value.GetType().GetFundamentalType();
	if( fundamental_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( unary_minus.file_pos_, value.GetType().ToString() ) );
		return ErrorValue();
	}
	const Variable variable= *value.GetVariable();

	const bool is_float= IsFloatingPoint( fundamental_type->fundamental_type );
	if( !( IsInteger( fundamental_type->fundamental_type ) || is_float ) )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( unary_minus.file_pos_, variable.type.ToString() ) );
		return ErrorValue();
	}
	// TODO - maybe not support unary minus for 8 and 16 bot integer types?

	Variable result;
	result.type= variable.type;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;

	if( variable.constexpr_value != nullptr )
	{
		if( is_float )
			result.llvm_value= result.constexpr_value= llvm::ConstantExpr::getFNeg( variable.constexpr_value );
		else
			result.llvm_value= result.constexpr_value= llvm::ConstantExpr::getNeg( variable.constexpr_value );
	}
	else
	{
		llvm::Value* const value_for_neg= CreateMoveToLLVMRegisterInstruction( variable, function_context );
		if( is_float )
			result.llvm_value= function_context.llvm_ir_builder.CreateFNeg( value_for_neg );
		else
			result.llvm_value= function_context.llvm_ir_builder.CreateNeg( value_for_neg );
	}

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( OverloadedOperatorToString(OverloadedOperator::Sub), result );
	result.referenced_variables.emplace( stored_result );
	function_context.stack_variables_stack.back()->RegisterVariable( stored_result );

	return Value( result, unary_minus.file_pos_ );
}

Value CodeBuilder::BuildLogicalNot(
	const Value& value,
	const Synt::LogicalNot& logical_not,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);
	CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(value);

	if( value.GetType().GetTemplateDependentType() != nullptr )
	{
		Variable result;
		result.value_type= ValueType::Value;
		result.type= GetNextTemplateDependentType();
		return Value( result, logical_not.file_pos_ );
	}

	if( value.GetType() != bool_type_ )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( logical_not.file_pos_, value.GetType().ToString() ) );
		return ErrorValue();
	}
	const Variable& variable= *value.GetVariable();

	Variable result;
	result.type= variable.type;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;

	if( variable.constexpr_value != nullptr )
		result.llvm_value= result.constexpr_value= llvm::ConstantExpr::getNot( variable.constexpr_value );
	else
	{
		llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( variable, function_context );
		result.llvm_value= function_context.llvm_ir_builder.CreateNot( value_in_register );
	}

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( OverloadedOperatorToString(OverloadedOperator::LogicalNot), result );
	result.referenced_variables.emplace( stored_result );
	function_context.stack_variables_stack.back()->RegisterVariable( stored_result );

	return Value( result, logical_not.file_pos_ );
}

Value CodeBuilder::BuildBitwiseNot(
	const Value& value,
	const Synt::BitwiseNot& bitwise_not,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);
	CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(value);

	if( value.GetType().GetTemplateDependentType() != nullptr )
	{
		Variable result;
		result.value_type= ValueType::Value;
		result.type= GetNextTemplateDependentType();
		return Value( result, bitwise_not.file_pos_ );
	}

	const FundamentalType* const fundamental_type= value.GetType().GetFundamentalType();
	if( fundamental_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( bitwise_not.file_pos_, value.GetType().ToString() ) );
		return ErrorValue();
	}
	if( !IsInteger( fundamental_type->fundamental_type ) )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( bitwise_not.file_pos_, value.GetType().ToString() ) );
		return ErrorValue();
	}

	const Variable variable= *value.GetVariable();

	Variable result;
	result.type= variable.type;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;

	if( variable.constexpr_value != nullptr )
		result.llvm_value= result.constexpr_value= llvm::ConstantExpr::getNot( variable.constexpr_value );
	else
	{
		llvm::Value* const value_in_register= CreateMoveToLLVMRegisterInstruction( variable, function_context );
		result.llvm_value= function_context.llvm_ir_builder.CreateNot( value_in_register );
	}

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( OverloadedOperatorToString(OverloadedOperator::BitwiseNot), result );
	result.referenced_variables.emplace( stored_result );
	function_context.stack_variables_stack.back()->RegisterVariable( stored_result );

	return Value( result, bitwise_not.file_pos_ );
}

} // namespace CodeBuilderPrivate

} // namespace U
