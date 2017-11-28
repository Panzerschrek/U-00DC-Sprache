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

static Synt::OverloadedOperator GetOverloadedOperatorForBinaryOperator( const Synt::BinaryOperatorType binary_operator_type )
{
	using Synt::BinaryOperatorType;
	using Synt::OverloadedOperator;
	switch( binary_operator_type )
	{
	case BinaryOperatorType::Add: return OverloadedOperator::Add;
	case BinaryOperatorType::Sub: return OverloadedOperator::Sub;
	case BinaryOperatorType::Mul: return OverloadedOperator::Mul;
	case BinaryOperatorType::Div: return OverloadedOperator::Div;
	case BinaryOperatorType::Rem: return OverloadedOperator::Rem;
	case BinaryOperatorType::And: return OverloadedOperator::And;
	case BinaryOperatorType::Or : return OverloadedOperator::Or ;
	case BinaryOperatorType::Xor: return OverloadedOperator::Xor;
	case BinaryOperatorType::Equal: return OverloadedOperator::Equal;
	case BinaryOperatorType::NotEqual: return OverloadedOperator::NotEqual;
	case BinaryOperatorType::Less: return OverloadedOperator::Less;
	case BinaryOperatorType::LessEqual: return OverloadedOperator::LessEqual;
	case BinaryOperatorType::Greater: return OverloadedOperator::Greater;
	case BinaryOperatorType::GreaterEqual: return OverloadedOperator::GreaterEqual;
	case BinaryOperatorType::ShiftLeft : return OverloadedOperator::ShiftLeft ;
	case BinaryOperatorType::ShiftRight: return OverloadedOperator::ShiftRight;
	default: U_ASSERT(false); return OverloadedOperator::None;
	};
}

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

Value CodeBuilder::BuildExpressionCode(
	const Synt::IExpressionComponent& expression,
	NamesScope& names,
	FunctionContext& function_context )
{
	Value result;

	if( const auto binary_operator=
		dynamic_cast<const Synt::BinaryOperator*>(&expression) )
	{
		if( binary_operator->operator_type_ == Synt::BinaryOperatorType::LazyLogicalAnd ||
			binary_operator->operator_type_ == Synt::BinaryOperatorType::LazyLogicalOr )
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
			std::vector<Function::Arg> args;
			args.reserve( 2u );
			const size_t error_count_before= errors_.size();

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

				const Value l_var_value=
					BuildExpressionCode(
						*binary_operator->left_,
						names,
						dummy_function_context );

				const Value r_var_value=
					BuildExpressionCode(
						*binary_operator->right_,
						names,
						dummy_function_context );

				CHECK_RETURN_ERROR_VALUE(l_var_value);
				CHECK_RETURN_ERROR_VALUE(l_var_value);
				CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(l_var_value);
				CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(r_var_value);

				const Variable* const l_var= l_var_value.GetVariable();
				const Variable* const r_var= r_var_value.GetVariable();
				if( l_var == nullptr )
					errors_.push_back( ReportExpectedVariableInBinaryOperator( binary_operator->file_pos_, l_var_value.GetType().ToString() ) );
				if( r_var == nullptr )
					errors_.push_back( ReportExpectedVariableInBinaryOperator( binary_operator->file_pos_, r_var_value.GetType().ToString() ) );
				if( l_var == nullptr || r_var == nullptr )
					return ErrorValue();

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

			const FunctionVariable* const overloaded_operator= GetOverloadedOperator( args, GetOverloadedOperatorForBinaryOperator( binary_operator->operator_type_ ) );
			if( overloaded_operator != nullptr )
			{
				std::vector<const Synt::IExpressionComponent*> synt_args;
				synt_args.reserve( 2u );
				synt_args.push_back( binary_operator->left_. get() );
				synt_args.push_back( binary_operator->right_.get() );
				return DoCallFunction( *overloaded_operator, binary_operator->file_pos_, nullptr, synt_args, names, function_context );
			}
			else
			{
				Value l_var_value=
					BuildExpressionCode(
						*binary_operator->left_,
						names,
						function_context );
				Variable* const l_var= l_var_value.GetVariable(); U_ASSERT( l_var != nullptr );

				if( l_var->type.GetFundamentalType() != nullptr )
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
				PrepareType( type_name_in_expression->file_pos_, type_name_in_expression->type_name, names ),
				type_name_in_expression->file_pos_ );
	}
	else
		U_ASSERT(false); // TODO

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
				result= BuildMemberAccessOperator( result, *member_access_operator, function_context );
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

			Synt::OverloadedOperator op= Synt::OverloadedOperator::None;
			if( dynamic_cast<const Synt::UnaryMinus*>( prefix_operator.get() ) != nullptr )
				op= Synt::OverloadedOperator::Sub;
			else if( dynamic_cast<const Synt::UnaryPlus*>( prefix_operator.get() ) != nullptr )
				op= Synt::OverloadedOperator::Add;
			else if( dynamic_cast<const Synt::LogicalNot*>( prefix_operator.get() ) != nullptr )
				op= Synt::OverloadedOperator::LogicalNot;
			else if( dynamic_cast<const Synt::BitwiseNot*>( prefix_operator.get() ) != nullptr )
				op= Synt::OverloadedOperator::BitwiseNot;
			else U_ASSERT( false );

			const FunctionVariable* const overloaded_operator= GetOverloadedOperator( args, op );
			if( overloaded_operator != nullptr )
			{
				result= DoCallFunction( *overloaded_operator, expression_with_unary_operators->file_pos_, var, {}, names, function_context );
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
	const Synt::BinaryOperatorType binary_operator,
	const FilePos& file_pos,
	FunctionContext& function_context )
{
	const bool arguments_are_constexpr= l_var.constexpr_value != nullptr && r_var.constexpr_value != nullptr;

	Variable result;

	const Type& l_type= l_var.type;
	const Type& r_type= r_var.type;
	const FundamentalType* const l_fundamental_type= l_type.GetFundamentalType();
	const FundamentalType* const r_fundamental_type= r_var.type.GetFundamentalType();

	using BinaryOperatorType= Synt::BinaryOperatorType;
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
		if( l_fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, l_type.ToString() ) );
			return ErrorValue();
		}
		else
		{
			const bool if_float= IsFloatingPoint( l_fundamental_type->fundamental_type );
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || if_float || l_fundamental_type->fundamental_type == U_FundamentalType::Bool ) )
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

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( result );
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
		BuildExpressionCodeAndDestroyTemporaries( r_expression, names, function_context );
		return l_var_value;
	}
	if( l_var_value.GetType().GetTemplateDependentType() != nullptr )
	{
		BuildExpressionCodeAndDestroyTemporaries( r_expression, names, function_context );
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
	if( binary_operator.operator_type_ == Synt::BinaryOperatorType::LazyLogicalAnd )
		function_context.llvm_ir_builder.CreateCondBr( l_var_in_register, r_part_block, block_after_operator );
	else if( binary_operator.operator_type_ == Synt::BinaryOperatorType::LazyLogicalOr )
		function_context.llvm_ir_builder.CreateCondBr( l_var_in_register, block_after_operator, r_part_block );
	else U_ASSERT(false);

	function_context.function->getBasicBlockList().push_back( r_part_block );
	function_context.llvm_ir_builder.SetInsertPoint( r_part_block );

	// Right part of lazy operator is conditinal. So, we must destroy it temporaries only in this condition.
	// We doesn`t needs longer lifetime of epxression temporaries, because we use only bool result.
	const Value r_var_value= BuildExpressionCodeAndDestroyTemporaries( r_expression, names, function_context );
	CHECK_RETURN_ERROR_VALUE(r_var_value);

	llvm::Value* r_var_in_register= nullptr;
	llvm::Constant* r_var_constepxr_value= nullptr;
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
		if( binary_operator.operator_type_ == Synt::BinaryOperatorType::LazyLogicalAnd )
			result.constexpr_value= llvm::ConstantExpr::getAnd( l_var.constexpr_value, r_var_constepxr_value );
		else if( binary_operator.operator_type_ == Synt::BinaryOperatorType::LazyLogicalOr )
			result.constexpr_value= llvm::ConstantExpr::getOr ( l_var.constexpr_value, r_var_constepxr_value );
		else
			U_ASSERT(false);
	}

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( result );
	result.referenced_variables.emplace( stored_result );
	function_context.stack_variables_stack.back()->RegisterVariable( stored_result );

	return Value( result, file_pos );

	#undef RETURN_UNDEF_BOOL
}

Value CodeBuilder::BuildNamedOperand(
	const Synt::NamedOperand& named_operand,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( named_operand.name_.components.size() == 1u &&
		named_operand.name_.components.back().name == Keywords::this_ &&
		named_operand.name_.components.back().template_parameters.empty() )
	{
		if( function_context.this_ == nullptr || function_context.is_constructor_initializer_list_now )
		{
			errors_.push_back( ReportThisUnavailable( named_operand.file_pos_ ) );
			return ErrorValue();
		}
		return Value( *function_context.this_, named_operand.file_pos_ );
	}

	const NamesScope::InsertedName* name_entry= ResolveName( named_operand.file_pos_, names, named_operand.name_ );
	if( !name_entry )
	{
		errors_.push_back( ReportNameNotFound( named_operand.file_pos_, named_operand.name_ ) );
		return ErrorValue();
	}

	if( const ClassField* const field= name_entry->second.GetClassField() )
	{
		if( function_context.this_ == nullptr )
		{
			errors_.push_back( ReportClassFiledAccessInStaticMethod( named_operand.file_pos_, named_operand.name_.components.back().name ) );
			return ErrorValue();
		}

		const ClassProxyPtr class_= field->class_.lock();
		U_ASSERT( class_ != nullptr && "Class is dead? WTF?" );

		// SPRACHE_TODO - allow access to parents fields here.
		if( Type(class_) != function_context.this_->type )
		{
			errors_.push_back( ReportAccessOfNonThisClassField( named_operand.file_pos_, named_operand.name_.components.back().name ) );
			return ErrorValue();
		}

		if( function_context.is_constructor_initializer_list_now &&
			function_context.uninitialized_this_fields.find( field ) != function_context.uninitialized_this_fields.end() )
		{
			errors_.push_back( ReportFieldIsNotInitializedYet( named_operand.file_pos_, named_operand.name_.components.back().name ) );
			return ErrorValue();
		}

		Variable field_variable;
		field_variable.type= field->type;
		field_variable.location= Variable::Location::Pointer;
		field_variable.value_type= function_context.this_->value_type;
		field_variable.referenced_variables= function_context.this_->referenced_variables;

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
		field_variable.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( function_context.this_->llvm_value, index_list );

		return Value( field_variable, named_operand.file_pos_ );
	}
	else if( const OverloadedFunctionsSet* const overloaded_functions_set=
		name_entry->second.GetFunctionsSet() )
	{
		if( function_context.this_ != nullptr )
		{
			if( function_context.is_constructor_initializer_list_now )
			{
				// SPRACHE_TODO - allow call of static methods and parents methods.
				errors_.push_back( ReportMethodsCallInConstructorInitializerListIsForbidden( named_operand.file_pos_, named_operand.name_.components.back().name ) );
				return ErrorValue();
			}

			// Trying add "this" to functions set.
			const Class* const class_= function_context.this_->type.GetClassType();

			const NamesScope::InsertedName* const same_set_in_class=
				class_->members.GetThisScopeName( named_operand.name_.components.back().name );
			// SPRACHE_TODO - add "this" for functions from parent classes.
			if( name_entry == same_set_in_class )
			{
				ThisOverloadedMethodsSet this_overloaded_methods_set;
				this_overloaded_methods_set.this_= *function_context.this_;
				this_overloaded_methods_set.overloaded_methods_set= *overloaded_functions_set;
				return this_overloaded_methods_set;
			}
		}
	}
	else if( const StoredVariablePtr referenced_variable= name_entry->second.GetStoredVariable() )
	{
		// Unwrap stored variable here.
		Variable result;
		result= referenced_variable->content;
		if( !referenced_variable->is_reference )
		{
			result.referenced_variables.emplace( referenced_variable );

			// If we have mutable reference to variable, we can not access variable itself.
			if( referenced_variable->content.value_type == ValueType::Reference && referenced_variable->mut_use_counter.use_count() >= 2u )
				errors_.push_back( ReportAccessingVariableThatHaveMutableReference( named_operand.file_pos_ ) );
		}

		return Value( result, name_entry->second.GetFilePos() );
	}

	return name_entry->second;
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

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( result );
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

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( result );
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

	const Array* array_type= value.GetType().GetArrayType();
	if( array_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( indexation_operator.file_pos_, value.GetType().ToString() ) );
		return ErrorValue();
	}
	const Variable& variable= *value.GetVariable();

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
	result.value_type= variable.value_type;
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

	result.llvm_value=
		function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef< llvm::Value*> ( index_list, 2u ) );

	return Value( result, indexation_operator.file_pos_ );
}

Value CodeBuilder::BuildMemberAccessOperator(
	const Value& value,
	const Synt::MemberAccessOperator& member_access_operator,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);
	CHECK_RETURN_TEMPLATE_DEPENDENT_VALUE(value);
	if( value.GetType().GetTemplateDependentType() != nullptr )
		return TemplateDependentValue();

	const Class* const class_type= value.GetType().GetClassType();
	if( class_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( member_access_operator.file_pos_, value.GetType().ToString() ) );
		return ErrorValue();
	}

	if( class_type->is_incomplete )
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

	if( const OverloadedFunctionsSet* const functions_set= class_member->second.GetFunctionsSet() )
	{
		ThisOverloadedMethodsSet this_overloaded_methods_set;
		this_overloaded_methods_set.this_= variable;
		this_overloaded_methods_set.overloaded_methods_set= *functions_set;
		return this_overloaded_methods_set;
	}

	const ClassField* const field= class_member->second.GetClassField();
	if( field == nullptr )
	{
		errors_.push_back( ReportNotImplemented( member_access_operator.file_pos_, "class members, except fields or methods" ) );
		return ErrorValue();
	}

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
	index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );

	Variable result;
	result.location= Variable::Location::Pointer;
	result.value_type= variable.value_type;
	result.referenced_variables= variable.referenced_variables;
	result.type= field->type;
	result.llvm_value=
		function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, index_list );
	return Value( result, member_access_operator.file_pos_ );
}

Value CodeBuilder::BuildCallOperator(
	const Value& function_value,
	const Synt::CallOperator& call_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(function_value);

	if( function_value.GetType() == NontypeStub::TemplateDependentValue )
	{
		for( const Synt::IExpressionComponentPtr& arg_expression : call_operator.arguments_ )
			BuildExpressionCode( *arg_expression, names, function_context );
		return function_value;
	}
	if( function_value.GetType().GetTemplateDependentType() != nullptr )
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

	if( functions_set == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( call_operator.file_pos_, function_value.GetType().ToString() ) );
		return ErrorValue();
	}

	size_t this_count= this_ == nullptr ? 0u : 1u;
	size_t total_args= this_count + call_operator.arguments_.size();
	std::vector<Function::Arg> actual_args;
	actual_args.reserve( total_args );

	const size_t error_count_before= errors_.size();

	{ // Make preevaluation af arguments for selection of overloaded function.
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
				errors_.push_back( ReportExpectedVariableAsArgument( arg_expression->GetFilePos(), expr_value.GetType().ToString() ) );
				return ErrorValue();
			}

			actual_args.emplace_back();
			actual_args.back().type= expr->type;
			actual_args.back().is_reference= expr->value_type != ValueType::Value;
			actual_args.back().is_mutable= expr->value_type == ValueType::Reference;
		}
		if( args_are_template_dependent )
			return Value( Type(NontypeStub::TemplateDependentValue), call_operator.file_pos_ );
	}

	// SPRACHE_TODO - try get function with "this" parameter in signature and without it.
	// We must support static functions call using "this".
	const FunctionVariable* const function_ptr=
		GetOverloadedFunction( *functions_set, actual_args, this_ != nullptr, call_operator.file_pos_ );
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
		actual_args.erase( actual_args.begin() );
	}
	U_ASSERT( function_type.args.size() == actual_args.size() );

	if( this_ == nullptr && function.is_this_call )
	{
		errors_.push_back( ReportCallOfThiscallFunctionUsingNonthisArgument( call_operator.file_pos_ ) );
		return ErrorValue();
	}

	errors_.resize( error_count_before ); // Drop errors from first pass.

	std::vector<const Synt::IExpressionComponent*> synt_args;
	for( const Synt::IExpressionComponentPtr& arg : call_operator.arguments_ )
		synt_args.push_back( arg.get() );

	return DoCallFunction( function, call_operator.file_pos_, this_, synt_args, names, function_context );
}

Value CodeBuilder::DoCallFunction(
	const FunctionVariable& function,
	const FilePos& call_file_pos,
	const Variable* first_arg,
	std::vector<const Synt::IExpressionComponent*> args,
	NamesScope& names,
	FunctionContext& function_context )
{
	const Function& function_type= *function.type.GetFunctionType();

	std::vector<llvm::Value*> llvm_args;
	std::unordered_map<StoredVariablePtr, VaraibleReferencesCounter> locked_variable_conters;
	std::vector<VariableStorageUseCounter> temp_args_locks; // We need lock reference argument before evaluating next arguments.

	llvm::Value* s_ret_value= nullptr;
	if( function.return_value_is_sret )
	{
		s_ret_value= function_context.alloca_ir_builder.CreateAlloca( function_type.return_type.GetLLVMType() );
		llvm_args.push_back( s_ret_value );
	}

	bool function_result_have_template_dependent_type= false;
	const size_t first_arg_count= first_arg == 0u ? 0u : 1u;
	const size_t arg_count= args.size() + first_arg_count;
	for( unsigned int i= 0u; i < arg_count; i++ )
	{
		const bool is_first_arg= first_arg != nullptr && i == 0u;
		const Function::Arg& arg= function_type.args[i];

		Variable expr;
		if( is_first_arg )
			expr= *first_arg;
		else
			expr= *BuildExpressionCode( *args[ i - first_arg_count ], names, function_context ).GetVariable();

		const FilePos& file_pos= is_first_arg ? file_pos : args[ i - first_arg_count ]->GetFilePos();

		const bool something_have_template_dependent_type= expr.type.GetTemplateDependentType() != nullptr || arg.type.GetTemplateDependentType() != nullptr;
		function_result_have_template_dependent_type= function_result_have_template_dependent_type || something_have_template_dependent_type;

		U_ASSERT( something_have_template_dependent_type || expr.type == arg.type );

		if( arg.is_reference )
		{
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

				llvm_args.push_back(expr.llvm_value);

				// Lock references.
				for( const StoredVariablePtr& referenced_variable : expr.referenced_variables )
				{
					++locked_variable_conters[referenced_variable].mut;
					temp_args_locks.push_back( referenced_variable->mut_use_counter );
				}
			}
			else
			{
				if( expr.value_type == ValueType::Value )
				{
					// Bind value to const reference.
					// TODO - support nonfundamental values.
					if( expr.location == Variable::Location::LLVMRegister )
					{
						if( !something_have_template_dependent_type )
						{
							llvm::Value* const temp_storage= function_context.alloca_ir_builder.CreateAlloca( expr.type.GetLLVMType() );
							function_context.llvm_ir_builder.CreateStore( expr.llvm_value, temp_storage );
							llvm_args.push_back( temp_storage );
						}
					}
					else
						llvm_args.push_back( expr.llvm_value );
				}
				else
					llvm_args.push_back( expr.llvm_value );

				// Lock references.
				for( const StoredVariablePtr& referenced_variable : expr.referenced_variables )
				{
					++locked_variable_conters[referenced_variable].imut;
					temp_args_locks.push_back( referenced_variable->imut_use_counter );
				}
			}
		}
		else
		{
			if( arg.type.GetFundamentalType() != nullptr )
			{
				if( !something_have_template_dependent_type )
					llvm_args.push_back( CreateMoveToLLVMRegisterInstruction( expr, function_context ) );
			}
			else if( const ClassProxyPtr class_type= arg.type.GetClassTypeProxy() )
			{
				if( !arg.type.IsCopyConstructible() )
				{
					// Can not call function with value parameter, because for value parameter needs copy, but parameter type is not copyable.
					// TODO - print more reliable message.
					errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, arg.type.ToString() ) );
					continue;
				}

				if( !something_have_template_dependent_type )
				{
					// Create copy of class value. Call copy constructor.
					llvm::Value* const arg_copy= function_context.alloca_ir_builder.CreateAlloca( arg.type.GetLLVMType() );

					TryCallCopyConstructor( file_pos, arg_copy, expr.llvm_value, class_type, function_context );
					llvm_args.push_back( arg_copy );
				}
			}
			else if( something_have_template_dependent_type )
			{}
			else
				U_ASSERT( false );
		}
	}

	// Check references.
	temp_args_locks.clear(); // clear temporary locks.
	for( const auto& pair : locked_variable_conters )
	{
		// Check references, passed into function.
		const VaraibleReferencesCounter& counter= pair.second;
		if( counter.mut == 1u && counter.imut == 0u )
		{} // All ok - one mutable reference.
		else if( counter.mut == 0u )
		{} // All ok - 0-infinity immutable references.
		else
		{
			errors_.push_back( ReportReferenceProtectionError( call_file_pos ) );
			continue;
		}

		// Check interaction between references, passed into function and references on stack.
		const StoredVariable& var= *pair.first;
		if( counter.mut == 1u &&
			( var.imut_use_counter.use_count() > 1u || var.mut_use_counter.use_count() > 2u ) )
		{
			// Pass mutable reference into function, while there are references on stack or somewhere else.
			// We can have one mutable reference on stack, but no more.
			errors_.push_back( ReportReferenceProtectionError( call_file_pos ) );
		}
		if( counter.mut == 1u && var.mut_use_counter.use_count() == 2u )
		{} // Ok - we take one mutable reference from stack and pass it into function.
		if( counter.mut == 0u && var.imut_use_counter.use_count() > 1u )
		{} // Ok - pass immutable references into function, while mutable references on stack exists.
	}

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
			dummy_result.location= function.return_value_is_sret ? Variable::Location::Pointer : Variable::Location::LLVMRegister;
		dummy_result.type= GetNextTemplateDependentType();
		return Value( dummy_result, call_file_pos );
	}

	llvm::Value* call_result=
		function_context.llvm_ir_builder.CreateCall(
			llvm::dyn_cast<llvm::Function>(function.llvm_function),
			llvm_args );

	if( function.return_value_is_sret )
	{
		U_ASSERT( s_ret_value != nullptr );
		call_result= s_ret_value;
	}

	Variable result;

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
		result.location= function.return_value_is_sret ? Variable::Location::Pointer : Variable::Location::LLVMRegister;
		result.value_type= ValueType::Value;

		const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( result );
		result.referenced_variables.emplace( stored_result );

		function_context.stack_variables_stack.back()->RegisterVariable( stored_result );
	}
	result.type= function_type.return_type;
	result.llvm_value= call_result;

	// Prepare reference result.
	if( function_type.return_value_is_reference )
	{
		// SPRACHE_TODO - process function lifetimes. Now - just combine all input function references.
		for( const auto& pair : locked_variable_conters )
			result.referenced_variables.emplace(pair.first);
	}

	return Value( result, call_file_pos );
}

Variable CodeBuilder::BuildTempVariableConstruction(
	const Type& type,
	const Synt::CallOperator& call_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	Variable variable;
	variable.type= type;
	variable.location= Variable::Location::Pointer;
	variable.value_type= ValueType::Reference;
	variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( type.GetLLVMType() );
	variable.constexpr_value= ApplyConstructorInitializer( variable, call_operator, names, function_context );
	variable.value_type= ValueType::Value; // Make value after construction

	const StoredVariablePtr stored_variable= std::make_shared<StoredVariable>( variable );
	variable.referenced_variables.emplace( stored_variable );

	function_context.stack_variables_stack.back()->RegisterVariable( stored_variable );

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

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( result );
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

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( result );
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

	const StoredVariablePtr stored_result= std::make_shared<StoredVariable>( result );
	result.referenced_variables.emplace( stored_result );
	function_context.stack_variables_stack.back()->RegisterVariable( stored_result );

	return Value( result, bitwise_not.file_pos_ );
}

} // namespace CodeBuilderPrivate

} // namespace U
