#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib/assert.hpp"
#include "../lex_synt_lib/keywords.hpp"
#include "../lex_synt_lib/lang_types.hpp"

#include "code_builder.hpp"

#define CHECK_RETURN_ERROR_VALUE(value) if( value.GetErrorValue() != nullptr ) { return value; }

namespace U
{

namespace CodeBuilderPrivate
{

Value CodeBuilder::BuildExpressionCodeAndDestroyTemporaries(
	const Synt::Expression& expression,
	NamesScope& names,
	FunctionContext& function_context )
{
	// Destruction frame for temporary variables of expression.
	const StackVariablesStorage temp_variables_storage( function_context );

	const Value result= BuildExpressionCode( expression, names, function_context );

	CallDestructors( *function_context.stack_variables_stack.back(), names, function_context, Synt::GetExpressionFilePos( expression ) );

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
			REPORT_ERROR( ExpectedVariable, names.GetErrors(), Synt::GetExpressionFilePos( expression ), result.GetKindName() );

		Variable dummy_result;
		dummy_result.type= invalid_type_;
		dummy_result.llvm_value= llvm::UndefValue::get( invalid_type_.GetLLVMType()->getPointerTo() );
		return dummy_result;
	}
	return std::move( *result_variable );
}

boost::optional<Value> CodeBuilder::TryCallOverloadedBinaryOperator(
	const OverloadedOperator op,
	const Synt::SyntaxElementBase& op_syntax_element,
	const Synt::Expression&  left_expr,
	const Synt::Expression& right_expr,
	const bool evaluate_args_in_reverse_order,
	const FilePos& file_pos,
	NamesScope& names,
	FunctionContext& function_context )
{
	const FunctionVariable* overloaded_operator= nullptr;

	const auto cache_it= function_context.overloading_resolution_cache.find( &op_syntax_element );
	if( cache_it != function_context.overloading_resolution_cache.end() )
		overloaded_operator= cache_it->second.get_ptr();
	else
	{
		ArgsVector<Function::Arg> args;

		// Know args types.
		bool needs_move_assign= false;
		const auto state= SaveInstructionsState( function_context );
		{
			const StackVariablesStorage dummy_stack_variables_storage( function_context );

			const Variable l_var= BuildExpressionCodeEnsureVariable( left_expr , names, function_context );
			const Variable r_var= BuildExpressionCodeEnsureVariable( right_expr, names, function_context );

			// Try apply move-assignment for class types.
			needs_move_assign=
				op == OverloadedOperator::Assign && r_var.value_type == ValueType::Value &&
				r_var.type == l_var.type && r_var.type.GetClassType() != nullptr &&
				l_var.value_type == ValueType::Reference;

			args.emplace_back();
			args.back().type= l_var.type;
			args.back().is_reference= l_var.value_type != ValueType::Value;
			args.back().is_mutable= l_var.value_type == ValueType::Reference;

			args.emplace_back();
			args.back().type= r_var.type;
			args.back().is_reference= r_var.value_type != ValueType::Value;
			args.back().is_mutable= r_var.value_type == ValueType::Reference;
		}
		RestoreInstructionsState( function_context, state );

		// Apply here move-assignment for class types.
		if( needs_move_assign )
		{
			// Move here, instead of calling copy-assignment operator. Before moving we must also call destructor for destination.
			const Variable l_var_real= *BuildExpressionCode(  left_expr, names, function_context ).GetVariable();
			const Variable r_var_real= *BuildExpressionCode( right_expr, names, function_context ).GetVariable();
			if( l_var_real.type.HaveDestructor() )
				CallDestructor( l_var_real.llvm_value, l_var_real.type, function_context, names.GetErrors(), file_pos );
			CopyBytes( r_var_real.llvm_value, l_var_real.llvm_value, l_var_real.type, function_context );

			const ReferencesGraphNodePtr& src_node= r_var_real.node;
			const ReferencesGraphNodePtr& dst_node= l_var_real.node;
			if( src_node != nullptr && dst_node != nullptr )
			{
				U_ASSERT( src_node->kind == ReferencesGraphNode::Kind::Variable );
				if( const auto src_node_inner_reference= function_context.variables_state.GetNodeInnerReference( src_node ) )
				{
					ReferencesGraphNodePtr dst_node_inner_reference= function_context.variables_state.GetNodeInnerReference( dst_node );
					if( dst_node_inner_reference == nullptr )
					{
						dst_node_inner_reference= std::make_shared<ReferencesGraphNode>( dst_node->name + " inner variable"_SpC, src_node_inner_reference->kind );
						function_context.variables_state.SetNodeInnerReference( dst_node, dst_node_inner_reference );
					}
					function_context.variables_state.AddLink( src_node_inner_reference, dst_node_inner_reference );
				}
				function_context.variables_state.MoveNode( src_node );
			}

			Variable move_result;
			move_result.type= void_type_;
			return Value( std::move(move_result), file_pos );
		}
		else if( args.front().type == args.back().type && args.front().type.GetTupleType() != nullptr )
			return CallBinaryOperatorForTuple( op, left_expr, right_expr, file_pos, names, function_context );

		overloaded_operator= GetOverloadedOperator( args, op, names.GetErrors(), file_pos );

		if( overloaded_operator == nullptr )
			function_context.overloading_resolution_cache[ &op_syntax_element ]= boost::none;
		else
			function_context.overloading_resolution_cache[ &op_syntax_element ]= *overloaded_operator;
	}

	if( overloaded_operator != nullptr )
	{
		if( overloaded_operator->is_deleted )
			REPORT_ERROR( AccessingDeletedMethod, names.GetErrors(), file_pos );
		if( !( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
			function_context.have_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

		if( overloaded_operator->virtual_table_index != ~0u )
		{
			// We can not fetch virtual function here, because "this" may be evaluated as second operand for some binary operators.
			REPORT_ERROR( NotImplemented, names.GetErrors(), file_pos, "calling virtual binary operators" );
		}

		std::vector<const Synt::Expression*> synt_args;
		synt_args.reserve( 2u );
		synt_args.push_back( & left_expr );
		synt_args.push_back( &right_expr );
		return
			DoCallFunction(
				overloaded_operator->llvm_function,
				*overloaded_operator->type.GetFunctionType(),
				file_pos,
				{},
				synt_args,
				evaluate_args_in_reverse_order,
				names,
				function_context );
	}

	return boost::none;
}

Value CodeBuilder::CallBinaryOperatorForTuple(
	OverloadedOperator op,
	const Synt::Expression&  left_expr,
	const Synt::Expression& right_expr,
	const FilePos& file_pos,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( op == OverloadedOperator::Assign )
	{
		const Variable r_var= BuildExpressionCodeEnsureVariable( right_expr, names, function_context );
		boost::optional<ReferencesGraphNodeHolder> r_var_lock;
		if( r_var.node != nullptr )
		{
			if( function_context.variables_state.HaveOutgoingMutableNodes( r_var.node ) )
				REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), file_pos, r_var.node->name );
			r_var_lock.emplace(
				std::make_shared<ReferencesGraphNode>( "lock "_SpC + r_var.node->name, ReferencesGraphNode::Kind::ReferenceImut ),
				function_context );
			function_context.variables_state.AddLink( r_var.node, r_var_lock->Node() );
		}

		const Variable l_var= BuildExpressionCodeEnsureVariable(  left_expr, names, function_context );
		if( l_var.node != nullptr && function_context.variables_state.HaveOutgoingLinks( l_var.node ) )
			REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), file_pos, l_var.node->name );

		U_ASSERT( l_var.type == r_var.type ); // Checked before.
		if( !l_var.type.IsCopyAssignable() )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_var.type );
			return ErrorValue();
		}
		if( l_var.value_type != ValueType::Reference )
		{
			REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), file_pos );
			return ErrorValue();
		}

		const ReferencesGraphNodePtr& src_node= r_var.node;
		const ReferencesGraphNodePtr& dst_node= l_var.node;
		if( src_node != nullptr && dst_node != nullptr && l_var.type.ReferencesTagsCount() > 0u )
		{
			const auto src_node_inner_references= function_context.variables_state.GetAllAccessibleInnerNodes_r( src_node );
			if( !src_node_inner_references.empty() )
			{
				bool node_is_mutable= false;
				for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
					node_is_mutable= node_is_mutable || src_node_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

				const auto dst_node_inner_reference= std::make_shared<ReferencesGraphNode>( dst_node->name + " inner variable"_SpC, node_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
				function_context.variables_state.SetNodeInnerReference( dst_node, dst_node_inner_reference );
				for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
					function_context.variables_state.AddLink( src_node_inner_reference, dst_node_inner_reference );
			}
		}

		BuildCopyAssignmentOperatorPart(
			l_var.llvm_value, r_var.llvm_value,
			l_var.type,
			function_context );

		Variable result;
		result.type= void_type_for_ret_;
		return Value( std::move(result), file_pos );
	}
	else
	{
		const Variable l_var= BuildExpressionCodeEnsureVariable( left_expr, names, function_context );
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_var.type );
		return ErrorValue();
	}
}

Value CodeBuilder::BuildExpressionCode(
	const Synt::Expression& expression,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( const auto binary_operator= boost::get<const Synt::BinaryOperator>(&expression) )
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
			boost::optional<Value> overloaded_operator_call_try=
				TryCallOverloadedBinaryOperator(
					GetOverloadedOperatorForBinaryOperator( binary_operator->operator_type_ ),
					*binary_operator,
					*binary_operator->left_, *binary_operator->right_,
					false,
					binary_operator->file_pos_,
					names,
					function_context );
			if( overloaded_operator_call_try != boost::none )
				return std::move(*overloaded_operator_call_try);

			Variable l_var=
				BuildExpressionCodeEnsureVariable(
					*binary_operator->left_,
					names,
					function_context );

			if( l_var.type.GetFundamentalType() != nullptr || l_var.type.GetEnumType() != nullptr || l_var.type.GetFunctionPointerType() != nullptr )
			{
				// Save l_var in register, because build-in binary operators require value-parameters.
				if( l_var.location == Variable::Location::Pointer )
				{
					l_var.llvm_value= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
					l_var.location= Variable::Location::LLVMRegister;
				}
				l_var.value_type= ValueType::Value;
			}
			DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), binary_operator->file_pos_ );

			const Variable r_var=
				BuildExpressionCodeEnsureVariable(
					*binary_operator->right_,
					names,
					function_context );

			return BuildBinaryOperator( l_var, r_var, binary_operator->operator_type_, binary_operator->file_pos_, names, function_context );
		}
	}

	struct Visitor final : public boost::static_visitor<Value>
	{
		CodeBuilder& this_;
		NamesScope& names;
		FunctionContext& function_context;

		Visitor( CodeBuilder& in_this, NamesScope& in_names, FunctionContext& in_function_context )
			: this_(in_this), names(in_names), function_context(in_function_context)
		{}

		Value operator()( const Synt::EmptyVariant& )
		{
			U_ASSERT(false);
			return ErrorValue();
		}
		Value operator()( const Synt::BinaryOperator& )
		{
			U_ASSERT(false); // Processed earlier.
			return ErrorValue();
		}
		Value operator()( const Synt::TernaryOperator& ternary_operator )
		{
			return this_.BuildTernaryOperator( ternary_operator, names, function_context );
		}
		Value operator()( const Synt::NamedOperand& named_operand )
		{
			return this_.BuildNamedOperand( named_operand, names, function_context );
		}
		Value operator()( const Synt::NumericConstant& numeric_constant )
		{
			return this_.BuildNumericConstant( numeric_constant, names, function_context );
		}
		Value operator()( const Synt::StringLiteral& string_literal )
		{
			return this_.BuildStringLiteral( string_literal, names, function_context );
		}
		Value operator()( const Synt::BooleanConstant& boolean_constant )
		{
			return Value( this_.BuildBooleanConstant( boolean_constant, function_context ), boolean_constant.file_pos_ );
		}
		Value operator()( const Synt::BracketExpression& bracket_expression )
		{
			return this_.BuildExpressionCode( *bracket_expression.expression_, names, function_context );
		}
		Value operator()( const Synt::TypeNameInExpression& type_name_in_expression )
		{
			return Value(
				this_.PrepareType( type_name_in_expression.type_name, names, function_context ),
				type_name_in_expression.file_pos_ );
		}
		Value operator()( const Synt::MoveOperator& move_operator )
		{
			return this_.BuildMoveOpeator( move_operator, names, function_context );
		}
		Value operator()( const Synt::CastRef& cast_ref )
		{
			return this_.BuildCastRef( cast_ref, names, function_context );
		}
		Value operator()( const Synt::CastRefUnsafe& cast_ref_unsafe )
		{
			return this_.BuildCastRefUnsafe( cast_ref_unsafe, names, function_context );
		}
		Value operator()( const Synt::CastImut& cast_imut )
		{
			return this_.BuildCastImut( cast_imut, names, function_context );
		}
		Value operator()( const Synt::CastMut& cast_mut )
		{
			return this_.BuildCastMut( cast_mut, names, function_context );
		}
		Value operator()( const Synt::TypeInfo& typeinfo_ )
		{
			return this_.BuildTypeinfoOperator( typeinfo_, names, function_context );
		}
	};
	Visitor visitor( *this, names, function_context );
	Value result= boost::apply_visitor( visitor, expression );

	struct ExpressionWithUnaryOperatorsVisitor final : public boost::static_visitor<const Synt::ExpressionComponentWithUnaryOperators*>
	{
		const Synt::ExpressionComponentWithUnaryOperators* operator()( const Synt::ExpressionComponentWithUnaryOperators& expression_with_unary_operators ) const
		{
			return &expression_with_unary_operators;
		}
		const Synt::ExpressionComponentWithUnaryOperators* operator()( const Synt::BinaryOperator& ) const
		{
			return nullptr;
		}
		const Synt::ExpressionComponentWithUnaryOperators* operator()( const Synt::EmptyVariant& ) const
		{
			U_ASSERT(false);
			return nullptr;
		}
	};

	if( const auto expression_with_unary_operators= boost::apply_visitor( ExpressionWithUnaryOperatorsVisitor(), expression ) )
	{
		for( const Synt::UnaryPostfixOperator& postfix_operator : expression_with_unary_operators->postfix_operators_ )
		{
			if( const auto indexation_operator= boost::get<const Synt::IndexationOperator>( &postfix_operator ) )
				result= BuildIndexationOperator( result, *indexation_operator, names, function_context );
			else if( const auto member_access_operator= boost::get<const Synt::MemberAccessOperator>( &postfix_operator ) )
				result= BuildMemberAccessOperator( result, *member_access_operator, names, function_context );
			else if( const auto call_operator= boost::get<const Synt::CallOperator>( &postfix_operator ) )
				result= BuildCallOperator( result, *call_operator, names, function_context );
			else U_ASSERT(false);
		} // for unary postfix operators

		for( const Synt::UnaryPrefixOperator& prefix_operator : expression_with_unary_operators->prefix_operators_ )
		{
			const Variable* const var= result.GetVariable();
			if( var == nullptr )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), expression_with_unary_operators->file_pos_, result.GetKindName() );
				continue;
			}

			ArgsVector<Function::Arg> args;
			args.emplace_back();
			args.back().type= var->type;
			args.back().is_mutable= var->value_type == ValueType::Reference;
			args.back().is_reference= var->value_type != ValueType::Value;

			OverloadedOperator op= OverloadedOperator::None;
			if( boost::get<const Synt::UnaryMinus>( &prefix_operator ) != nullptr )
				op= OverloadedOperator::Sub;
			else if( boost::get<const Synt::UnaryPlus>( &prefix_operator ) != nullptr )
				op= OverloadedOperator::Add;
			else if( boost::get<const Synt::LogicalNot>( &prefix_operator ) != nullptr )
				op= OverloadedOperator::LogicalNot;
			else if( boost::get<const Synt::BitwiseNot>( &prefix_operator ) != nullptr )
				op= OverloadedOperator::BitwiseNot;
			else U_ASSERT( false );

			const FunctionVariable* const overloaded_operator= GetOverloadedOperator( args, op, names.GetErrors(), expression_with_unary_operators->file_pos_ );
			if( overloaded_operator != nullptr )
			{
				if( !( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
					function_context.have_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

				if( overloaded_operator->is_this_call && overloaded_operator->virtual_table_index != ~0u )
				{
					const auto fetch_result= TryFetchVirtualFunction( *var, *overloaded_operator, function_context, names.GetErrors(), expression_with_unary_operators->file_pos_ );

					result= DoCallFunction( fetch_result.second, *overloaded_operator->type.GetFunctionType(), expression_with_unary_operators->file_pos_, { fetch_result.first }, {}, false, names, function_context );
				}
				else
				{
					result=
						DoCallFunction(
							overloaded_operator->llvm_function,
							*overloaded_operator->type.GetFunctionType(),
							expression_with_unary_operators->file_pos_,
							{ *var },
							{},
							false,
							names,
							function_context );
				}
			}
			else
			{
				if( const auto unary_minus= boost::get<const Synt::UnaryMinus>( &prefix_operator ) )
					result= BuildUnaryMinus( result, *unary_minus, names, function_context );
				else if( const auto unary_plus= boost::get<const Synt::UnaryPlus>( &prefix_operator ) )
				{
					// TODO - maybe do something here?
					(void)unary_plus;
				}
				else if( const auto logical_not= boost::get<const Synt::LogicalNot>( &prefix_operator ) )
					result= BuildLogicalNot( result, *logical_not, names, function_context );
				else if( const auto bitwise_not= boost::get<const Synt::BitwiseNot>( &prefix_operator ) )
					result= BuildBitwiseNot( result, *bitwise_not, names, function_context );
				else U_ASSERT(false);
			}
		} // for unary prefix operators
	}

	return result;
}

Value CodeBuilder::BuildBinaryOperator(
	const Variable& l_var,
	const Variable& r_var,
	const BinaryOperatorType binary_operator,
	const FilePos& file_pos,
	NamesScope& names,
	FunctionContext& function_context )
{
	const bool arguments_are_constexpr= l_var.constexpr_value != nullptr && r_var.constexpr_value != nullptr;

	Variable result;

	const Type& l_type= l_var.type;
	const Type& r_type= r_var.type;
	const FundamentalType* const l_fundamental_type= l_type.GetFundamentalType();
	const FundamentalType* const r_fundamental_type= r_type.GetFundamentalType();

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
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), file_pos, r_var.type, l_var.type,  BinaryOperatorToString( binary_operator ) );
			return ErrorValue();
		}
		if( l_fundamental_type == nullptr )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_type );
			return ErrorValue();
		}
		else
		{
			if( l_fundamental_type->GetSize() < 4u )
			{
				// Operation supported only for 32 and 64bit operands
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_type );
				return ErrorValue();
			}
			const bool is_float= IsFloatingPoint( l_fundamental_type->fundamental_type );
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || is_float ) )
			{
				// this operations allowed only for integer and floating point operands.
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_type );
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
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), file_pos, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
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
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_type );
			return ErrorValue();
		}
		else
		{
			const FundamentalType raw_fundamental_type= l_fundamental_type != nullptr ? *l_fundamental_type : l_var.type.GetEnumType()->underlaying_type;

			const bool if_float= IsFloatingPoint( raw_fundamental_type.fundamental_type );
			if( !( IsInteger( raw_fundamental_type.fundamental_type ) || IsChar( raw_fundamental_type.fundamental_type ) || if_float || raw_fundamental_type == bool_type_ ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_type );
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
			// Use ordered floating point compare operations, which result is false for NaN, except !=. nan != nan must be true.
			case BinaryOperatorType::Equal:
				if( if_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_OEQ, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpOEQ( l_value_for_op, r_value_for_op );
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
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), file_pos, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
			return ErrorValue();
		}
		if( l_fundamental_type == nullptr )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_type );
			return ErrorValue();
		}
		else
		{
			const bool if_float= IsFloatingPoint( l_fundamental_type->fundamental_type );
			const bool is_char= IsChar( l_fundamental_type->fundamental_type );
			const bool is_signed= !is_char && IsSignedInteger( l_fundamental_type->fundamental_type );
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || if_float || is_char ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_type );
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
			// Use ordered floating point compare operations, which result is false for NaN.
			case BinaryOperatorType::Less:
				if( if_float )
				{
					if( arguments_are_constexpr )
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_OLT, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpOLT( l_value_for_op, r_value_for_op );
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
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_OLE, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpOLE( l_value_for_op, r_value_for_op );
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
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_OGT, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpOGT( l_value_for_op, r_value_for_op );
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
						result.constexpr_value= llvm::ConstantExpr::getFCmp( llvm::CmpInst::FCMP_OGE, l_var.constexpr_value, r_var.constexpr_value );
					else
						result_value= function_context.llvm_ir_builder.CreateFCmpOGE( l_value_for_op, r_value_for_op );
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
			REPORT_ERROR( NoMatchBinaryOperatorForGivenTypes, names.GetErrors(), file_pos, r_var.type, l_var.type, BinaryOperatorToString( binary_operator ) );
			return ErrorValue();
		}
		if( l_fundamental_type == nullptr )
		{
			REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_type );
			return ErrorValue();
		}
		else
		{
			if( !( IsInteger( l_fundamental_type->fundamental_type ) || l_fundamental_type->fundamental_type == U_FundamentalType::Bool ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_type );
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
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, l_type );
				return ErrorValue();
			}
			if( r_fundamental_type == nullptr || !IsUnsignedInteger( r_fundamental_type->fundamental_type ) )
			{
				REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, r_type );
				return ErrorValue();
			}
			const SizeType l_type_size= l_fundamental_type->GetSize();
			const SizeType r_type_size= r_fundamental_type->GetSize();

			if( l_var.constexpr_value != nullptr && r_var.constexpr_value != nullptr )
			{
				// Convert value of shift to type of shifted value. LLVM Reuqired this.
				llvm::Constant* r_value_for_op= r_var.constexpr_value;
				if( r_type_size > l_type_size )
					r_value_for_op= llvm::ConstantExpr::getTrunc( r_value_for_op, l_var.type.GetLLVMType() );
				else if( r_type_size < l_type_size )
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
				if( r_type_size > l_type_size )
					r_value_for_op= function_context.llvm_ir_builder.CreateTrunc( r_value_for_op, l_var.type.GetLLVMType() );
				else if( r_type_size < l_type_size )
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
			REPORT_ERROR( ConstantExpressionResultIsUndefined, names.GetErrors(), file_pos );
			result.constexpr_value= nullptr;
		}
	}

	const auto node= std::make_shared<ReferencesGraphNode>( BinaryOperatorToString(binary_operator), ReferencesGraphNode::Kind::Variable );
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( node, result ) );
	result.node= node;
	return Value( std::move(result), file_pos );
}

Value CodeBuilder::BuildLazyBinaryOperator(
	const Synt::Expression& l_expression,
	const Synt::Expression& r_expression,
	const Synt::BinaryOperator& binary_operator,
	const FilePos& file_pos,
	NamesScope& names,
	FunctionContext& function_context )
{
	const Variable l_var= BuildExpressionCodeEnsureVariable( l_expression, names, function_context );

	if( l_var.type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), binary_operator.file_pos_, bool_type_, l_var.type );
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
			REPORT_ERROR( TypesMismatch, names.GetErrors(), binary_operator.file_pos_, bool_type_, r_var.type );
			return ErrorValue();
		}
		r_var_constepxr_value= r_var.constexpr_value;
		r_var_in_register= CreateMoveToLLVMRegisterInstruction( r_var, function_context );

		// Destroy r_var temporaries in this branch.
		CallDestructors( *function_context.stack_variables_stack.back(), names, function_context, file_pos );
	}
	function_context.variables_state= MergeVariablesStateAfterIf( { variables_state_before_r_branch, function_context.variables_state }, names.GetErrors(), file_pos );

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

	const auto node= std::make_shared<ReferencesGraphNode>( BinaryOperatorToString(binary_operator.operator_type_), ReferencesGraphNode::Kind::Variable );
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( node, result ) );
	result.node= node;
	return Value( std::move(result), file_pos );
}

Value CodeBuilder::BuildCastRef( const Synt::CastRef& cast_ref, NamesScope& names, FunctionContext& function_context )
{
	return DoReferenceCast( cast_ref.file_pos_, *cast_ref.type_, *cast_ref.expression_, false, names, function_context );
}

Value CodeBuilder::BuildCastRefUnsafe( const Synt::CastRefUnsafe& cast_ref_unsafe, NamesScope& names, FunctionContext& function_context )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( UnsafeReferenceCastOutsideUnsafeBlock, names.GetErrors(), cast_ref_unsafe.file_pos_ );

	return DoReferenceCast( cast_ref_unsafe.file_pos_, *cast_ref_unsafe.type_, *cast_ref_unsafe.expression_, true, names, function_context );
}

Value CodeBuilder::DoReferenceCast(
	const FilePos& file_pos,
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
	result.value_type= var.value_type == ValueType::Reference ? ValueType::Reference : ValueType::ConstReference; // "ValueType" here converts inot ConstReference.
	result.location= Variable::Location::Pointer;
	result.node= var.node;

	llvm::Value* src_value= var.llvm_value;
	if( var.location == Variable::Location::LLVMRegister )
	{
		src_value= function_context.alloca_ir_builder.CreateAlloca( var.type.GetLLVMType() );
		function_context.llvm_ir_builder.CreateStore( var.llvm_value, src_value );
	}

	if( type == var.type )
		result.llvm_value= src_value;
	else if( type == void_type_ )
		result.llvm_value= CreateReferenceCast( src_value, var.type, type, function_context );
	else
	{
		// Complete types required for both safe and unsafe casting, except unsafe void to anything cast.
		// This needs, becasue we must emit same code for places where types yet not complete, and where they are complete.
		if( !EnsureTypeCompleteness( type, TypeCompleteness::Complete ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), file_pos, type );

		if( !( enable_unsafe && var.type == void_type_ ) && !EnsureTypeCompleteness( var.type, TypeCompleteness::Complete ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), file_pos, var.type );

		if( ReferenceIsConvertible( var.type, type, names.GetErrors(), file_pos ) )
			result.llvm_value= CreateReferenceCast( src_value, var.type, type, function_context );
		else
		{
			result.llvm_value= function_context.llvm_ir_builder.CreatePointerCast( src_value, type.GetLLVMType()->getPointerTo() );
			if( !enable_unsafe )
				REPORT_ERROR( TypesMismatch, names.GetErrors(), file_pos, type, var.type );
		}
	}

	return Value( std::move(result), file_pos );
}

Value CodeBuilder::BuildCastImut( const Synt::CastImut& cast_imut, NamesScope& names, FunctionContext& function_context )
{
	const Variable var= BuildExpressionCodeEnsureVariable( *cast_imut.expression_, names, function_context );

	Variable result= var;
	result.value_type= ValueType::ConstReference;

	if( var.location == Variable::Location::LLVMRegister )
	{
		result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( var.type.GetLLVMType() );
		function_context.llvm_ir_builder.CreateStore( var.llvm_value, result.llvm_value );
	}

	return Value( std::move(result), cast_imut.file_pos_ );
}

Value CodeBuilder::BuildCastMut( const Synt::CastMut& cast_mut, NamesScope& names, FunctionContext& function_context )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( MutableReferenceCastOutsideUnsafeBlock, names.GetErrors(), cast_mut.file_pos_ );

	const Variable var= BuildExpressionCodeEnsureVariable( *cast_mut.expression_, names, function_context );

	Variable result= var;
	result.value_type= ValueType::Reference;

	if( var.location == Variable::Location::LLVMRegister )
	{
		result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( var.type.GetLLVMType() );
		function_context.llvm_ir_builder.CreateStore( var.llvm_value, result.llvm_value );
	}

	return Value( std::move(result), cast_mut.file_pos_ );
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
				REPORT_ERROR( ThisUnavailable, names.GetErrors(), named_operand.file_pos_ );
				return ErrorValue();
			}
			return Value( *function_context.this_, named_operand.file_pos_ );
		}
		else if( named_operand.name_.components.back().name == Keywords::base_ )
		{
			if( function_context.this_ == nullptr )
			{
				REPORT_ERROR( BaseUnavailable, names.GetErrors(), named_operand.file_pos_ );
				return ErrorValue();
			}
			const Class& class_= *function_context.this_->type.GetClassType();
			if( class_.base_class == nullptr )
			{
				REPORT_ERROR( BaseUnavailable, names.GetErrors(), named_operand.file_pos_ );
				return ErrorValue();
			}
			if( function_context.whole_this_is_unavailable && ( !function_context.base_initialized || class_.base_class->class_->kind == Class::Kind::Abstract ) )
			{
				REPORT_ERROR( FieldIsNotInitializedYet, names.GetErrors(), named_operand.file_pos_, Keyword( Keywords::base_ ) );
				return ErrorValue();
			}

			Variable base= *function_context.this_;
			base.type= class_.base_class;
			base.llvm_value= CreateReferenceCast( function_context.this_->llvm_value, function_context.this_->type, base.type, function_context );
			return Value( std::move(base), named_operand.file_pos_ );
		}
	}

	const Value* const value_entry= ResolveValue( named_operand.file_pos_, names, named_operand.name_ );
	if( value_entry == nullptr )
	{
		REPORT_ERROR( NameNotFound, names.GetErrors(), named_operand.file_pos_, named_operand.name_ );
		return ErrorValue();
	}

	const ProgramString& back_name_component= named_operand.name_.components.back().name;
	if( !function_context.is_in_unsafe_block &&
		( back_name_component == Keywords::constructor_ || back_name_component == Keywords::destructor_ ) )
		REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names.GetErrors(), named_operand.file_pos_, back_name_component );

	if( const ClassField* const field= value_entry->GetClassField() )
	{
		if( function_context.this_ == nullptr )
		{
			REPORT_ERROR( ClassFiledAccessInStaticMethod, names.GetErrors(), named_operand.file_pos_, named_operand.name_.components.back().name );
			return ErrorValue();
		}

		const ClassProxyPtr class_= field->class_.lock();
		U_ASSERT( class_ != nullptr && "Class is dead? WTF?" );

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= GetZeroGEPIndex();

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
					REPORT_ERROR( AccessOfNonThisClassField, names.GetErrors(), named_operand.file_pos_, named_operand.name_.components.back().name );
					return ErrorValue();
				}

				index_list[1]= GetFieldGEPIndex( 0u /* base class is allways first field */ );
				actual_field_class_ptr= function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );
				actual_field_class= actual_field_class->class_->base_class;
			}
		}

		if( function_context.whole_this_is_unavailable &&
			function_context.uninitialized_this_fields.find( field ) != function_context.uninitialized_this_fields.end() )
		{
			REPORT_ERROR( FieldIsNotInitializedYet, names.GetErrors(), named_operand.file_pos_, named_operand.name_.components.back().name );
			return ErrorValue();
		}
		if( function_context.whole_this_is_unavailable &&
			field_class_proxy != function_context.this_->type.GetClassTypeProxy() &&
			!function_context.base_initialized )
		{
			REPORT_ERROR( FieldIsNotInitializedYet, names.GetErrors(), named_operand.file_pos_, Keyword( Keywords::base_ ) );
			return ErrorValue();
		}

		Variable field_variable;
		field_variable.type= field->type;
		field_variable.location= Variable::Location::Pointer;
		field_variable.value_type= ( function_context.this_->value_type == ValueType::Reference && field->is_mutable ) ? ValueType::Reference : ValueType::ConstReference;
		field_variable.node= function_context.this_->node;

		index_list[1]= GetFieldGEPIndex( field->index );
		field_variable.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );

		if( field->is_reference )
		{
			field_variable.value_type= field->is_mutable ? ValueType::Reference : ValueType::ConstReference;
			field_variable.llvm_value= function_context.llvm_ir_builder.CreateLoad( field_variable.llvm_value );

			if( function_context.this_->node != nullptr )
			{
				const auto field_node= std::make_shared<ReferencesGraphNode>( "this."_SpC + back_name_component, field->is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
				function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( field_node, field_variable ) );
				field_variable.node= field_node;
				for( const ReferencesGraphNodePtr& node : function_context.variables_state.GetAllAccessibleInnerNodes_r( function_context.this_->node ) )
				{
					if( (  field->is_mutable && function_context.variables_state.HaveOutgoingLinks( node ) ) ||
						( !field->is_mutable && function_context.variables_state.HaveOutgoingMutableNodes( node ) ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), named_operand.file_pos_, node->name );
					else
						function_context.variables_state.AddLink( node, field_node );
				}
			}
		}

		return Value( std::move(field_variable), named_operand.file_pos_ );
	}
	else if( const OverloadedFunctionsSet* const overloaded_functions_set= value_entry->GetFunctionsSet() )
	{
		if( function_context.this_ != nullptr )
		{
			// Trying add "this" to functions set.
			const Class* const class_= function_context.this_->type.GetClassType();

			// SPRACHE_TODO - mabe this kind of search is incorrect?
			const Value* const same_set_in_class=
				class_->members.GetThisScopeValue( named_operand.name_.components.back().name );
			// SPRACHE_TODO - add "this" for functions from parent classes.
			if( value_entry == same_set_in_class )
			{
				if( !function_context.whole_this_is_unavailable )
				{
					// Append "this" only if whole "this" is available.
					ThisOverloadedMethodsSet this_overloaded_methods_set;
					this_overloaded_methods_set.this_= *function_context.this_;
					this_overloaded_methods_set.GetOverloadedFunctionsSet()= *overloaded_functions_set;
					return std::move(this_overloaded_methods_set);
				}
			}
		}
	}
	else if( const Variable* const variable= value_entry->GetVariable() )
	{
		if( variable->node != nullptr && function_context.variables_state.NodeMoved( variable->node ) )
			REPORT_ERROR( AccessingMovedVariable, names.GetErrors(), named_operand.file_pos_, variable->node->name );
	}

	return *value_entry;
}

Value CodeBuilder::BuildTernaryOperator( const Synt::TernaryOperator& ternary_operator, NamesScope& names, FunctionContext& function_context )
{
	const Variable condition= BuildExpressionCodeEnsureVariable( *ternary_operator.condition, names, function_context );
	if( condition.type != bool_type_ )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), ternary_operator.file_pos_, bool_type_, condition.type );
		return ErrorValue();
	}

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
			}
			RestoreInstructionsState( function_context, state );
		}
	}

	if( branches_types[0] != branches_types[1] )
	{
		REPORT_ERROR( TypesMismatch, names.GetErrors(), ternary_operator.file_pos_, branches_types[0], branches_types[1] );
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
		if( !( result.type == void_type_ || result.type == void_type_for_ret_ ) )
		{
			if( !EnsureTypeCompleteness( result.type, TypeCompleteness::Complete ) )
			{
				REPORT_ERROR( UsingIncompleteType, names.GetErrors(), ternary_operator.file_pos_, result.type );
				return ErrorValue();
			}
			result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( result.type.GetLLVMType() );
			result.llvm_value->setName( "select_result" );
		}
	}
	else if( branches_value_types[0] == ValueType::ConstReference || branches_value_types[1] == ValueType::ConstReference )
	{
		result.value_type= ValueType::ConstReference;
		node_kind= ReferencesGraphNode::Kind::ReferenceImut;
	}
	else
	{
		result.value_type= ValueType::Reference;
		node_kind= ReferencesGraphNode::Kind::ReferenceMut;
	}
	const auto result_node= std::make_shared<ReferencesGraphNode>( "select_result"_SpC, node_kind );
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( result_node, result ) );
	result.node= result_node;

	llvm::BasicBlock* const result_block= llvm::BasicBlock::Create( llvm_context_ );
	llvm::BasicBlock* const branches_basic_blocks[2]{ llvm::BasicBlock::Create( llvm_context_ ), llvm::BasicBlock::Create( llvm_context_ ) };

	function_context.llvm_ir_builder.CreateCondBr( CreateMoveToLLVMRegisterInstruction( condition, function_context ), branches_basic_blocks[0], branches_basic_blocks[1] );

	llvm::Value* branches_reference_values[2] { nullptr, nullptr };
	llvm::Constant* branches_constexpr_values[2] { nullptr, nullptr };
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
				if( result.type == void_type_ || result.type == void_type_for_ret_ )
				{}
				else if( result.type.GetFundamentalType() != nullptr || result.type.GetEnumType() != nullptr || result.type.GetFunctionPointerType() != nullptr )
					function_context.llvm_ir_builder.CreateStore( CreateMoveToLLVMRegisterInstruction( branch_result, function_context ), result.llvm_value );
				else if( const ClassProxyPtr class_type= result.type.GetClassTypeProxy() )
				{
					if( branch_result.node != nullptr && result.type.ReferencesTagsCount() > 0u )
					{
						const auto src_node_inner_references= function_context.variables_state.GetAllAccessibleInnerNodes_r( branch_result.node );
						if( !src_node_inner_references.empty() )
						{
							ReferencesGraphNodePtr result_inner_reference= function_context.variables_state.GetNodeInnerReference( result_node );
							if( result_inner_reference == nullptr )
							{
								bool node_is_mutable= false;
								for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
									node_is_mutable= node_is_mutable || src_node_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

								result_inner_reference= std::make_shared<ReferencesGraphNode>( result_node->name + " inner variable"_SpC, node_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
								function_context.variables_state.SetNodeInnerReference( result_node, result_inner_reference );
							}

							for( const ReferencesGraphNodePtr& src_node_inner_reference : src_node_inner_references )
							{
								if( ( result_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceImut && function_context.variables_state.HaveOutgoingMutableNodes( src_node_inner_reference ) ) ||
									( result_inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut  && function_context.variables_state.HaveOutgoingLinks( src_node_inner_reference ) ) )
									REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), ternary_operator.file_pos_, branch_result.node->name );
								else
									function_context.variables_state.AddLink( src_node_inner_reference, result_inner_reference );
							}
						}
					}

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
							REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), ternary_operator.file_pos_, result.type );
							return ErrorValue();
						}
						TryCallCopyConstructor( names.GetErrors(), ternary_operator.file_pos_, result.llvm_value, branch_result.llvm_value, class_type, function_context );
					}
				}
				else
				{
					REPORT_ERROR( NotImplemented, names.GetErrors(), ternary_operator.file_pos_, "move such kind of types" );
					return ErrorValue();
				}
			}
			else
			{
				branches_reference_values[i]= branch_result.llvm_value;
				if( branch_result.node != nullptr )
				{
					if( ( result.value_type == ValueType::ConstReference && function_context.variables_state.HaveOutgoingMutableNodes( branch_result.node ) ) ||
						( result.value_type == ValueType::Reference && function_context.variables_state.HaveOutgoingLinks( branch_result.node ) ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), ternary_operator.file_pos_, branch_result.node->name );
					else
						function_context.variables_state.AddLink( branch_result.node, result_node );
				}
			}

			CallDestructors( *function_context.stack_variables_stack.back(), names, function_context, ternary_operator.file_pos_ );
			function_context.llvm_ir_builder.CreateBr( result_block );
		}
		branches_variables_state[i]= function_context.variables_state;
	}
	function_context.function->getBasicBlockList().push_back( result_block );
	function_context.llvm_ir_builder.SetInsertPoint( result_block );

	function_context.variables_state= MergeVariablesStateAfterIf( branches_variables_state, names.GetErrors(), ternary_operator.file_pos_ );

	if( result.value_type != ValueType::Value )
	{
		llvm::PHINode* const phi= function_context.llvm_ir_builder.CreatePHI( result.type.GetLLVMType()->getPointerTo(), 2u );
		phi->addIncoming( branches_reference_values[0], branches_basic_blocks[0] );
		phi->addIncoming( branches_reference_values[1], branches_basic_blocks[1] );
		result.llvm_value= phi;
	}

	if( condition.constexpr_value != nullptr )
		result.constexpr_value= condition.constexpr_value->getUniqueInteger().getLimitedValue() != 0u ? branches_constexpr_values[0] : branches_constexpr_values[1];

	return Value( result, ternary_operator.file_pos_ );
}

Value CodeBuilder::BuildMoveOpeator( const Synt::MoveOperator& move_operator, NamesScope& names, FunctionContext& function_context )
{
	Synt::ComplexName complex_name;
	complex_name.components.emplace_back();
	complex_name.components.back().name= move_operator.var_name_;

	const Value* const resolved_value= ResolveValue( move_operator.file_pos_, names, complex_name );
	if( resolved_value == nullptr )
	{
		REPORT_ERROR( NameNotFound, names.GetErrors(), move_operator.file_pos_, move_operator.var_name_ );
		return ErrorValue();
	}
	const Variable* const variable_for_move= resolved_value->GetVariable();
	if( variable_for_move == nullptr ||
		variable_for_move->node == nullptr ||
		variable_for_move->node->kind != ReferencesGraphNode::Kind::Variable )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), move_operator.file_pos_, resolved_value->GetKindName() );
		return ErrorValue();
	}
	const ReferencesGraphNodePtr& node= variable_for_move->node;

	bool found_in_variables= false;
	for( const auto& stack_frame : function_context.stack_variables_stack )
	for( const StackVariablesStorage::NodeAndVariable& arg_node : stack_frame->variables_ )
	{
		if( arg_node.first == node )
		{
			found_in_variables= true;
			goto end_variable_search;
		}
	}
	end_variable_search:
	if( !found_in_variables )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), move_operator.file_pos_, resolved_value->GetKindName() );
		return ErrorValue();
	}

	// TODO - maybe allow moving for immutable variables?
	if( variable_for_move->value_type != ValueType::Reference )
	{
		REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), move_operator.file_pos_ );
		return ErrorValue();
	}
	if( function_context.variables_state.NodeMoved( node ) )
	{
		REPORT_ERROR( AccessingMovedVariable, names.GetErrors(), move_operator.file_pos_, node->name );
		return ErrorValue();
	}

	// If this is mutable variable - it is stack variable or value argument.
	// This can not be temp variable, global variable, or inner argument variable.

	if( function_context.variables_state.HaveOutgoingLinks( node ) )
	{
		REPORT_ERROR( MovedVariableHaveReferences, names.GetErrors(), move_operator.file_pos_, node->name );
		return ErrorValue();
	}

	Variable content= *variable_for_move;
	content.value_type= ValueType::Value;

	const ReferencesGraphNodePtr moved_result= std::make_shared<ReferencesGraphNode>( "_moved_"_SpC + node->name, ReferencesGraphNode::Kind::Variable );
	content.node= moved_result;
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( moved_result, content ) );

	// We must save inner references of moved variable.
	if( const auto move_variable_inner_node= function_context.variables_state.GetNodeInnerReference( node ) )
	{
		const auto inner_node= std::make_shared<ReferencesGraphNode>( moved_result->name + " inner node"_SpC, move_variable_inner_node->kind );
		function_context.variables_state.SetNodeInnerReference( moved_result, inner_node );
		function_context.variables_state.AddLink( move_variable_inner_node, inner_node );
	}
	function_context.variables_state.MoveNode( node );

	return Value( std::move(content), move_operator.file_pos_ );
}

Value CodeBuilder::BuildNumericConstant(
	const Synt::NumericConstant& numeric_constant,
	NamesScope& names,
	FunctionContext& function_context )
{
	const U_FundamentalType type= GetNumericConstantType( numeric_constant );
	if( type == U_FundamentalType::InvalidType )
	{
		REPORT_ERROR( UnknownNumericConstantType, names.GetErrors(), numeric_constant.file_pos_, numeric_constant.type_suffix_.data() );
		return ErrorValue();
	}
	llvm::Type* const llvm_type= GetFundamentalLLVMType( type );

	Variable result;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;
	result.type= FundamentalType( type, llvm_type );

	if( IsInteger( type ) || IsChar( type ) )
		result.constexpr_value=
			llvm::Constant::getIntegerValue( llvm_type, llvm::APInt( llvm_type->getIntegerBitWidth(), uint64_t(numeric_constant.value_) ) );
	else if( IsFloatingPoint( type ) )
		result.constexpr_value=
			llvm::ConstantFP::get( llvm_type, static_cast<double>( numeric_constant.value_) );
	else
		U_ASSERT(false);

	result.llvm_value= result.constexpr_value;

	const ReferencesGraphNodePtr node= std::make_shared<ReferencesGraphNode>( ToProgramString( "numeric constant " + std::to_string(numeric_constant.value_) ), ReferencesGraphNode::Kind::Variable );
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( node, result ) );
	result.node= node;
	return Value( std::move(result), numeric_constant.file_pos_ );
}

Value CodeBuilder::BuildStringLiteral( const Synt::StringLiteral& string_literal, NamesScope& names, FunctionContext& function_context )
{
	U_UNUSED( function_context );

	U_FundamentalType char_type= U_FundamentalType::InvalidType;
	SizeType array_size= ~0u; // ~0 - means non-array
	llvm::Constant* initializer= nullptr;

	const ProgramString type_suffix= string_literal.type_suffix_.data();

	if( type_suffix.empty() || type_suffix == "u8"_SpC )
	{
		const std::string value= ToUTF8( string_literal.value_ );

		char_type= U_FundamentalType::char8;
		array_size= value.size();
		initializer= llvm::ConstantDataArray::getString( llvm_context_, value, false /* not null terminated */ );
	}
	else if( type_suffix == "u16"_SpC )
	{
		char_type= U_FundamentalType::char16;
		array_size= string_literal.value_.size();
		initializer=
			llvm::ConstantDataArray::get(
				llvm_context_,
				llvm::ArrayRef<uint16_t>(string_literal.value_.data(), string_literal.value_.size() ) );
	}
	else if( type_suffix == "u32"_SpC )
	{
		std::vector<uint32_t> str;
		str.resize( string_literal.value_.size() );
		for( size_t i= 0u; i < string_literal.value_.size(); ++i )
			str[i]= string_literal.value_[i];

		char_type= U_FundamentalType::char32;
		array_size= str.size();
		initializer= llvm::ConstantDataArray::get( llvm_context_, str );
	}
	// If string literal have char suffix, process it as single char literal.
	else if( type_suffix ==  "c8"_SpC || type_suffix == GetFundamentalTypeName( U_FundamentalType::char8  ) )
	{
		if( string_literal.value_.size() == 1u && GetUTF8CharBytes(string_literal.value_[0]) == 1u )
		{
			char_type= U_FundamentalType::char8 ;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char8 , uint64_t(string_literal.value_[0]), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names.GetErrors(), string_literal.file_pos_, string_literal.value_ );
	}
	else if( type_suffix == "c16"_SpC || type_suffix == GetFundamentalTypeName( U_FundamentalType::char16 ) )
	{
		if( string_literal.value_.size() == 1u )
		{
			char_type= U_FundamentalType::char16;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char16, uint64_t(string_literal.value_[0]), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names.GetErrors(), string_literal.file_pos_, string_literal.value_ );
	}
	else if( type_suffix == "c32"_SpC || type_suffix== GetFundamentalTypeName( U_FundamentalType::char32 ) )
	{
		if( string_literal.value_.size() == 1u )
		{
			char_type= U_FundamentalType::char32;
			initializer= llvm::ConstantInt::get( fundamental_llvm_types_.char32, uint64_t(string_literal.value_[0]), false );
		}
		else
			REPORT_ERROR( InvalidSizeForCharLiteral, names.GetErrors(), string_literal.file_pos_, string_literal.value_ );
	}
	else
	{
		REPORT_ERROR( UnknownStringLiteralSuffix, names.GetErrors(), string_literal.file_pos_, type_suffix );
		return ErrorValue();
	}

	Variable result;
	if( array_size == ~0u )
	{
		result.type= FundamentalType( char_type, GetFundamentalLLVMType( char_type ) );

		result.value_type= ValueType::Value;
		result.location= Variable::Location::LLVMRegister;
		result.llvm_value= result.constexpr_value= initializer;
	}
	else
	{
		Array array_type;
		array_type.type= FundamentalType( char_type, GetFundamentalLLVMType( char_type ) );
		array_type.size= array_size;
		array_type.llvm_type= llvm::ArrayType::get( GetFundamentalLLVMType( char_type ), array_size );
		result.type= std::move(array_type);

		result.value_type= ValueType::ConstReference;
		result.location= Variable::Location::Pointer;

		result.constexpr_value= initializer;
		result.llvm_value=
			CreateGlobalConstantVariable(
				result.type,
				"_string_literal_" + std::to_string( reinterpret_cast<uintptr_t>(&string_literal) ),
				result.constexpr_value );
	}

	return Value( std::move(result), string_literal.file_pos_ );
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

	const ReferencesGraphNodePtr node= std::make_shared<ReferencesGraphNode>( Keyword( boolean_constant.value_ ? Keywords::true_ : Keywords::false_ ), ReferencesGraphNode::Kind::Variable );
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( node, result ) );
	result.node= node;
	return std::move(result);
}

Value CodeBuilder::BuildIndexationOperator(
	const Value& value,
	const Synt::IndexationOperator& indexation_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);

	if( value.GetVariable() == nullptr )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), indexation_operator.file_pos_, value.GetKindName() );
		return ErrorValue();
	}

	const Variable& variable= *value.GetVariable();

	if( variable.type.GetClassType() != nullptr ) // If this is class - try call overloaded [] operator.
	{
		ArgsVector<Function::Arg> args;
		args.emplace_back();
		args.back().type= variable.type;
		args.back().is_reference= variable.value_type != ValueType::Value;
		args.back().is_mutable= variable.value_type == ValueType::Reference;

		// Know type of index.
		const auto state= SaveInstructionsState( function_context );
		{
			const StackVariablesStorage dummy_stack_variables_storage( function_context );

			const Variable index_variable= BuildExpressionCodeEnsureVariable( indexation_operator.index_, names, function_context );

			args.emplace_back();
			args.back().type= index_variable.type;
			args.back().is_reference= index_variable.value_type != ValueType::Value;
			args.back().is_mutable= index_variable.value_type == ValueType::Reference;
		}
		RestoreInstructionsState( function_context, state );

		const FunctionVariable* const overloaded_operator=
			GetOverloadedOperator( args, OverloadedOperator::Indexing, names.GetErrors(), indexation_operator.file_pos_ );
		if( overloaded_operator != nullptr )
		{
			if( !( overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || overloaded_operator->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
				function_context.have_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

			if( overloaded_operator->is_this_call && overloaded_operator->virtual_table_index != ~0u  )
			{
				const auto fetch_result= TryFetchVirtualFunction( variable, *overloaded_operator, function_context, names.GetErrors(), indexation_operator.file_pos_ );
				return
					DoCallFunction(
						fetch_result.second,
						*overloaded_operator->type.GetFunctionType(),
						indexation_operator.file_pos_,
						{ fetch_result.first }, { &indexation_operator.index_ }, false,
						names, function_context );
			}
			else
				return
					DoCallFunction(
						overloaded_operator->llvm_function,
						*overloaded_operator->type.GetFunctionType(),
						indexation_operator.file_pos_,
						{ variable }, { &indexation_operator.index_ }, false,
						names, function_context );

			function_context.overloading_resolution_cache[ &indexation_operator ]= *overloaded_operator;
		}
		else
			function_context.overloading_resolution_cache[ &indexation_operator ]= boost::none;

		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), indexation_operator.file_pos_, value.GetKindName() );
		return ErrorValue();
	}
	else if( const Array* const array_type= variable.type.GetArrayType() )
	{
		// Lock array. We must prevent modification of array in index calcualtion.
		const ReferencesGraphNodeHolder array_lock(
			std::make_shared<ReferencesGraphNode>( "array lock"_SpC, variable.value_type == ValueType::Reference ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut ),
			function_context );
		if( variable.node != nullptr )
			function_context.variables_state.AddLink( variable.node, array_lock.Node() );

		const Variable index= BuildExpressionCodeEnsureVariable( indexation_operator.index_, names, function_context );

		const FundamentalType* const index_fundamental_type= index.type.GetFundamentalType();
		if( index_fundamental_type == nullptr || !IsUnsignedInteger( index_fundamental_type->fundamental_type ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), indexation_operator.file_pos_, "any unsigned integer"_SpC, index.type );
			return ErrorValue();
		}

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
				REPORT_ERROR( ArrayIndexOutOfBounds, names.GetErrors(), indexation_operator.file_pos_, index_value, array_type->size );
		}

		Variable result;
		result.location= Variable::Location::Pointer;
		result.value_type= variable.value_type == ValueType::Reference ? ValueType::Reference : ValueType::ConstReference;
		result.node= variable.node;
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
		index_list[0]= GetZeroGEPIndex();
		index_list[1]= CreateMoveToLLVMRegisterInstruction( index, function_context );

		// If index is not const and array size is not undefined - check bounds.
		if( index.constexpr_value == nullptr && array_type->size != Array::c_undefined_size )
		{
			llvm::Value* index_value= index_list[1];
			const SizeType index_size= index_fundamental_type->GetSize();
			const SizeType size_type_size= size_type_.GetFundamentalType()->GetSize();
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

		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), indexation_operator.file_pos_ ); // Destroy temporaries of index expression.

		result.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, index_list );

		return Value( std::move(result), indexation_operator.file_pos_ );
	}
	else if( const Tuple* const tuple_type= variable.type.GetTupleType() )
	{
		const Variable index= BuildExpressionCodeEnsureVariable( indexation_operator.index_, names, function_context );

		const FundamentalType* const index_fundamental_type= index.type.GetFundamentalType();
		if( index_fundamental_type == nullptr || !IsUnsignedInteger( index_fundamental_type->fundamental_type ) )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), indexation_operator.file_pos_, "any unsigned integer"_SpC, index.type );
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
			REPORT_ERROR( ExpectedConstantExpression, names.GetErrors(), indexation_operator.file_pos_ );
			return ErrorValue();
		}
		const uint64_t index_value= index.constexpr_value->getUniqueInteger().getLimitedValue();
		if( index_value >= tuple_type->elements.size() )
		{
			REPORT_ERROR( TupleIndexOutOfBounds, names.GetErrors(), indexation_operator.file_pos_, index_value, tuple_type->elements.size() );
			return ErrorValue();
		}

		Variable result;
		result.location= Variable::Location::Pointer;
		result.value_type= variable.value_type == ValueType::Reference ? ValueType::Reference : ValueType::ConstReference;
		result.node= variable.node;
		result.type= tuple_type->elements[index_value];
		result.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex(index_value) } );

		return Value( std::move(result), indexation_operator.file_pos_ );
	}
	else
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), indexation_operator.file_pos_, value.GetKindName() );
		return ErrorValue();
	}
}

Value CodeBuilder::BuildMemberAccessOperator(
	const Value& value,
	const Synt::MemberAccessOperator& member_access_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);

	if( value.GetVariable() == nullptr )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), member_access_operator.file_pos_, value.GetKindName() );
		return ErrorValue();
	}
	const Variable& variable= *value.GetVariable();

	Class* const class_type= variable.type.GetClassType();
	if( class_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), member_access_operator.file_pos_, value.GetKindName() );
		return ErrorValue();
	}

	if( !EnsureTypeCompleteness( variable.type, TypeCompleteness::Complete ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), member_access_operator.file_pos_, value.GetKindName() );
		return ErrorValue();
	}

	const Value* const class_member= class_type->members.GetThisScopeValue( member_access_operator.member_name_ );
	if( class_member == nullptr )
	{
		REPORT_ERROR( NameNotFound, names.GetErrors(), member_access_operator.file_pos_, member_access_operator.member_name_ );
		return ErrorValue();
	}

	if( !function_context.is_in_unsafe_block &&
		( member_access_operator.member_name_ == Keywords::constructor_ || member_access_operator.member_name_ == Keywords::destructor_ ) )
		REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names.GetErrors(), member_access_operator.file_pos_,  member_access_operator.member_name_ );

	if( names.GetAccessFor( variable.type.GetClassTypeProxy() ) < class_type->GetMemberVisibility( member_access_operator.member_name_ ) )
		REPORT_ERROR( AccessingNonpublicClassMember, names.GetErrors(), member_access_operator.file_pos_, class_type->members.GetThisNamespaceName(), member_access_operator.member_name_ );

	if( const OverloadedFunctionsSet* functions_set= class_member->GetFunctionsSet() )
	{
		if( member_access_operator.have_template_parameters )
		{
			if( functions_set->template_functions.empty() )
				REPORT_ERROR( ValueIsNotTemplate, names.GetErrors(), member_access_operator.file_pos_ );
			else
			{
				const Value* const inserted_value=
					GenTemplateFunctionsUsingTemplateParameters(
						member_access_operator.file_pos_,
						functions_set->template_functions,
						member_access_operator.template_parameters,
						names );
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

	if( member_access_operator.have_template_parameters )
		REPORT_ERROR( ValueIsNotTemplate, names.GetErrors(), member_access_operator.file_pos_ );

	const ClassField* const field= class_member->GetClassField();
	if( field == nullptr )
	{
		REPORT_ERROR( NotImplemented, names.GetErrors(), member_access_operator.file_pos_, "class members, except fields or methods" );
		return ErrorValue();
	}

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= GetZeroGEPIndex();

	const ClassProxyPtr field_class_proxy= field->class_.lock();
	U_ASSERT( field_class_proxy != nullptr );

	llvm::Value* actual_field_class_ptr= nullptr;
	if( field_class_proxy == variable.type.GetClassTypeProxy() )
		actual_field_class_ptr= variable.llvm_value;
	else
	{
		// For parent filed we needs make several GEP isntructions.
		ClassProxyPtr actual_field_class= variable.type.GetClassTypeProxy();
		actual_field_class_ptr= variable.llvm_value;
		while( actual_field_class != field_class_proxy )
		{
			index_list[1]= GetFieldGEPIndex( 0u /* base class is allways first field */ );
			actual_field_class_ptr= function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );

			actual_field_class= actual_field_class->class_->base_class;
			U_ASSERT(actual_field_class != nullptr );
		}
	}

	index_list[1]= GetFieldGEPIndex( field->index );

	Variable result;
	result.location= Variable::Location::Pointer;
	result.value_type= ( variable.value_type == ValueType::Reference && field->is_mutable ) ? ValueType::Reference : ValueType::ConstReference;
	result.node= variable.node;
	result.type= field->type;
	result.llvm_value=
		function_context.llvm_ir_builder.CreateGEP( actual_field_class_ptr, index_list );

	if( variable.constexpr_value != nullptr )
	{
		llvm::Constant* var_constexpr_value= variable.constexpr_value;
		if( class_type->is_typeinfo ) // HACK!!! Replace old constexpr value with new for typeinfo, because constexpr value for incomplete type may be undef.
		{
			for( const auto& typeinfo_cache_entry : typeinfo_cache_ )
			{
				if( typeinfo_cache_entry.second.type == variable.type )
				{
					var_constexpr_value= typeinfo_cache_entry.second.constexpr_value;
					break;
				}
			}
		}

		result.constexpr_value= var_constexpr_value->getAggregateElement( static_cast<unsigned int>( field->index ) );
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

		if( variable.node != nullptr )
		{
			const auto field_node= std::make_shared<ReferencesGraphNode>( "this."_SpC + member_access_operator.member_name_, field->is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
			function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( field_node, result ) );
			result.node= field_node;
			for( const ReferencesGraphNodePtr& inner_reference : function_context.variables_state.GetAllAccessibleInnerNodes_r( variable.node ) )
			{
				if( (  field->is_mutable && function_context.variables_state.HaveOutgoingLinks( inner_reference ) ) ||
					( !field->is_mutable && function_context.variables_state.HaveOutgoingMutableNodes( inner_reference ) ) )
					REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), member_access_operator.file_pos_, inner_reference->name );
				else
					function_context.variables_state.AddLink( inner_reference, result.node );
			}
		}
	}

	return Value( std::move(result), member_access_operator.file_pos_ );
}

Value CodeBuilder::BuildCallOperator(
	const Value& function_value,
	const Synt::CallOperator& call_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(function_value);

	if( const Type* const type= function_value.GetTypeName() )
		return Value( BuildTempVariableConstruction( *type, call_operator, names, function_context ), call_operator.file_pos_ );

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
		if( const Class* const class_type= callable_variable->type.GetClassType() )
		{
			// For classes try to find () operator inside it.
			if( const Value* const value=
				class_type->members.GetThisScopeValue( OverloadedOperatorToString( OverloadedOperator::Call ) ) )
			{
				functions_set= value->GetFunctionsSet();
				U_ASSERT( functions_set != nullptr ); // If we found (), this must be functions set.
				this_= callable_variable;
				// SPRACHE_TODO - maybe support not only thiscall () operators ?
			}
		}
		else if( const FunctionPointer* const function_pointer= callable_variable->type.GetFunctionPointerType() )
		{
			function_context.have_non_constexpr_operations_inside= true; // Calling function, using pointer, is not constexpr. We can not garantee, that called function is constexpr.

			// Call function pointer directly.
			if( function_pointer->function.args.size() != call_operator.arguments_.size() )
			{
				REPORT_ERROR( InvalidFunctionArgumentCount, names.GetErrors(), call_operator.file_pos_, call_operator.arguments_.size(), function_pointer->function.args.size() );
				return ErrorValue();
			}

			std::vector<const Synt::Expression*> args;
			args.reserve( call_operator.arguments_.size() );
			for( const Synt::Expression& arg : call_operator.arguments_ )
				args.push_back( &arg );

			llvm::Value* const func_itself= CreateMoveToLLVMRegisterInstruction( *callable_variable, function_context );

			return
				DoCallFunction(
					func_itself, function_pointer->function, call_operator.file_pos_,
					{}, args, false,
					names, function_context );
		}
	}

	if( functions_set == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), call_operator.file_pos_, function_value.GetKindName() );
		return ErrorValue();
	}

	size_t this_count= this_ == nullptr ? 0u : 1u;
	size_t total_args= this_count + call_operator.arguments_.size();

	const FunctionVariable* function_ptr= nullptr;

	// Make preevaluation af arguments for selection of overloaded function. Try also get function from cache.
	const auto cache_it= function_context.overloading_resolution_cache.find( &call_operator );
	if( cache_it != function_context.overloading_resolution_cache.end() &&
		!call_operator.arguments_.empty() ) // empty check - hack for dummy call operator from empty initializer.
		function_ptr= cache_it->second.get_ptr();
	else
	{
		ArgsVector<Function::Arg> actual_args;
		actual_args.reserve( total_args );

		const auto state= SaveInstructionsState( function_context );
		{
			const StackVariablesStorage dummy_stack_variables_storage( function_context );

			// Push "this" argument.
			if( this_ != nullptr )
			{
				actual_args.emplace_back();
				actual_args.back().type= this_->type;
				actual_args.back().is_reference= true;
				actual_args.back().is_mutable= this_->value_type == ValueType::Reference;
			}
			// Push arguments from call operator.
			for( const Synt::Expression& arg_expression : call_operator.arguments_ )
			{
				const Variable expr= BuildExpressionCodeEnsureVariable( arg_expression, names, function_context );

				actual_args.emplace_back();
				actual_args.back().type= expr.type;
				actual_args.back().is_reference= expr.value_type != ValueType::Value;
				actual_args.back().is_mutable= expr.value_type == ValueType::Reference;
			}
		}
		RestoreInstructionsState( function_context, state );

		function_ptr=
			GetOverloadedFunction( *functions_set, actual_args, this_ != nullptr, names.GetErrors(), call_operator.file_pos_ );

		if( function_ptr == nullptr )
			function_context.overloading_resolution_cache[ &call_operator ]= boost::none;
		else
			function_context.overloading_resolution_cache[ &call_operator ]= *function_ptr;
	}

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
		REPORT_ERROR( CallOfThiscallFunctionUsingNonthisArgument, names.GetErrors(), call_operator.file_pos_ );
		return ErrorValue();
	}

	if( function_ptr->is_deleted )
		REPORT_ERROR( AccessingDeletedMethod, names.GetErrors(), call_operator.file_pos_ );

	if( !( function_ptr->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprIncomplete || function_ptr->constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete ) )
		function_context.have_non_constexpr_operations_inside= true; // Can not call non-constexpr function in constexpr function.

	std::vector<const Synt::Expression*> synt_args;
	synt_args.reserve( call_operator.arguments_.size() );
	for( const Synt::Expression& arg : call_operator.arguments_ )
		synt_args.push_back( &arg );

	Variable this_casted;
	llvm::Value* llvm_function_ptr= function.llvm_function;
	if( this_ != nullptr )
	{
		auto fetch_result= TryFetchVirtualFunction( *this_, function, function_context, names.GetErrors(), call_operator.file_pos_ );
		this_casted= std::move( fetch_result.first );
		llvm_function_ptr= fetch_result.second;
		this_= &this_casted;
	}

	return
		DoCallFunction(
			llvm_function_ptr, function_type,
			call_operator.file_pos_,
			this_ == nullptr ? std::vector<Variable>() : std::vector<Variable>{ *this_ },
			synt_args, false,
			names, function_context,
			function.constexpr_kind == FunctionVariable::ConstexprKind::ConstexprComplete );
}

Value CodeBuilder::DoCallFunction(
	llvm::Value* function,
	const Function& function_type,
	const FilePos& call_file_pos,
	const std::vector<Variable>& preevaluated_args,
	const std::vector<const Synt::Expression*>& args,
	const bool evaluate_args_in_reverse_order,
	NamesScope& names,
	FunctionContext& function_context,
	const bool func_is_constexpr )
{
	if( function_type.unsafe && !function_context.is_in_unsafe_block )
		REPORT_ERROR( UnsafeFunctionCallOutsideUnsafeBlock, names.GetErrors(), call_file_pos );

	const size_t arg_count= preevaluated_args.size() + args.size();
	U_ASSERT( arg_count == function_type.args.size() );

	ArgsVector<llvm::Value*> llvm_args;
	ArgsVector<llvm::Constant*> constant_llvm_args;
	llvm_args.resize( arg_count, nullptr );

	std::vector< ReferencesGraphNodeHolder > locked_args_references;
	std::vector< ReferencesGraphNodeHolder > locked_args_inner_references;

	for( size_t i= 0u; i < arg_count; ++i )
	{
		const size_t j= evaluate_args_in_reverse_order ? arg_count - i - 1u : i;

		const Function::Arg& arg= function_type.args[j];

		Variable expr;
		FilePos file_pos;
		if( j < preevaluated_args.size() )
		{
			expr= preevaluated_args[j];
			file_pos= call_file_pos;
		}
		else
		{
			expr= BuildExpressionCodeEnsureVariable( *args[ j - preevaluated_args.size() ], names, function_context );
			file_pos= Synt::GetExpressionFilePos( *args[ j - preevaluated_args.size() ] );
		}

		if( expr.constexpr_value != nullptr && !( arg.is_reference && arg.is_mutable ) )
			constant_llvm_args.push_back( expr.constexpr_value );

		if( arg.is_reference )
		{
			if( !ReferenceIsConvertible( expr.type, arg.type, names.GetErrors(), call_file_pos ) &&
				GetConversionConstructor( expr.type, arg.type, names.GetErrors(), file_pos ) == nullptr )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), file_pos, arg.type, expr.type );
				return ErrorValue();
			}

			if( arg.is_mutable )
			{
				if( expr.value_type == ValueType::Value )
				{
					REPORT_ERROR( ExpectedReferenceValue, names.GetErrors(), file_pos );
					return ErrorValue();
				}
				if( expr.value_type == ValueType::ConstReference )
				{
					REPORT_ERROR( BindingConstReferenceToNonconstReference, names.GetErrors(), file_pos );
					return ErrorValue();
				}

				if( expr.type == arg.type )
					llvm_args[j]= expr.llvm_value;
				else
					llvm_args[j]= CreateReferenceCast( expr.llvm_value, expr.type, arg.type, function_context );

				// Lock references.
				locked_args_references.emplace_back(
					std::make_shared<ReferencesGraphNode>( ToProgramString( "reference_arg_" + std::to_string(i) ), ReferencesGraphNode::Kind::ReferenceMut ),
					function_context );
				const auto& arg_node= locked_args_references.back().Node();
				if( expr.node != nullptr )
				{
					if( function_context.variables_state.HaveOutgoingLinks( expr.node ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), file_pos, expr.node->name );
					else
						function_context.variables_state.AddLink( expr.node, arg_node );
				}
			}
			else
			{
				if( expr.value_type == ValueType::Value && expr.location == Variable::Location::LLVMRegister )
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
				{
					if( expr.type.ReferenceIsConvertibleTo( arg.type ) )
						llvm_args[j]= CreateReferenceCast( llvm_args[j], expr.type, arg.type, function_context );
					else
					{
						const auto conversion_constructor= GetConversionConstructor( expr.type, arg.type, names.GetErrors(), file_pos );
						U_ASSERT( conversion_constructor != nullptr );
						expr= ConvertVariable( expr, arg.type, *conversion_constructor, names, function_context, file_pos );
						llvm_args[j]= expr.llvm_value;
					}
				}

				// Lock references.
				locked_args_references.emplace_back(
					std::make_shared<ReferencesGraphNode>( ToProgramString( "reference_arg_" + std::to_string(i) ), ReferencesGraphNode::Kind::ReferenceImut ),
					function_context );
				const auto& arg_node= locked_args_references.back().Node();
				if( expr.node != nullptr )
				{
					if( function_context.variables_state.HaveOutgoingMutableNodes( expr.node ) )
						REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), file_pos, expr.node->name );
					else
						function_context.variables_state.AddLink( expr.node, arg_node );
				}
			}

			// Lock inner references.
			if( expr.node != nullptr )
			{
				const auto inner_references= function_context.variables_state.GetAllAccessibleInnerNodes_r( expr.node );
				if( !inner_references.empty() )
				{
					EnsureTypeCompleteness( arg.type, TypeCompleteness::ReferenceTagsComplete );
					if( arg.type.ReferencesTagsCount() > 0 )
					{
						bool is_mutable= false;
						for( const ReferencesGraphNodePtr& inner_reference : inner_references )
							is_mutable= is_mutable || inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

						locked_args_inner_references.emplace_back(
							std::make_shared<ReferencesGraphNode>(
								ToProgramString( "arg_lock_" + std::to_string(i) ),
								is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut ),
							function_context );
						for( const ReferencesGraphNodePtr& inner_reference : inner_references )
						{
							if( inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut && function_context.variables_state.HaveOutgoingLinks( inner_reference ) )
								REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), file_pos, inner_reference->name );
							else
								function_context.variables_state.AddLink( inner_reference, locked_args_inner_references.back().Node() );
						}
					}
				}
			}
		}
		else
		{
			locked_args_references.emplace_back(
				std::make_shared<ReferencesGraphNode>( ToProgramString( "value_arg_" + std::to_string(i) ), ReferencesGraphNode::Kind::Variable ),
				function_context );

			if( !ReferenceIsConvertible( expr.type, arg.type, names.GetErrors(), call_file_pos ) &&
				GetConversionConstructor( expr.type, arg.type, names.GetErrors(), file_pos ) == nullptr )
			{
				REPORT_ERROR( TypesMismatch, names.GetErrors(), file_pos, arg.type, expr.type );
				return ErrorValue();
			}

			if( expr.type != arg.type )
			{
				if( expr.type.ReferenceIsConvertibleTo( arg.type ) ){}
				else
				{
					const auto conversion_constructor= GetConversionConstructor( expr.type, arg.type, names.GetErrors(), file_pos );
					U_ASSERT( conversion_constructor != nullptr );
					expr= ConvertVariable( expr, arg.type, *conversion_constructor, names, function_context, file_pos );
				}
			}

			if( arg.type.GetFundamentalType() != nullptr || arg.type.GetEnumType() != nullptr || arg.type.GetFunctionPointerType() != nullptr )
				llvm_args[j]= CreateMoveToLLVMRegisterInstruction( expr, function_context );
			else if( arg.type.GetClassType() != nullptr || arg.type.GetTupleType() != nullptr )
			{
				// Lock inner references.
				// Do it only if arg type can contain any reference inside.
				// Do it before potential moving.
				EnsureTypeCompleteness( arg.type, TypeCompleteness::ReferenceTagsComplete ); // arg type for value arg must be already complete.
				if( expr.node != nullptr && arg.type.ReferencesTagsCount() > 0u )
				{
					const auto inner_references= function_context.variables_state.GetAllAccessibleInnerNodes_r( expr.node );
					bool is_mutable= false;
					for( const ReferencesGraphNodePtr& inner_reference : inner_references )
						is_mutable= is_mutable || inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut;

					const auto value_arg_inner_node= std::make_shared<ReferencesGraphNode>( ToProgramString( "value_arg_inner_reference_" + std::to_string(i) ), is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
					function_context.variables_state.SetNodeInnerReference( locked_args_references.back().Node(), value_arg_inner_node );

					for( const ReferencesGraphNodePtr inner_reference : inner_references )
					{
						if( inner_reference->kind == ReferencesGraphNode::Kind::ReferenceMut  && function_context.variables_state.HaveOutgoingLinks( inner_reference ) )
							REPORT_ERROR( ReferenceProtectionError, names.GetErrors(), file_pos, inner_reference->name );
						else
							function_context.variables_state.AddLink( inner_reference, value_arg_inner_node );
					}
				}

				if( expr.value_type == ValueType::Value && expr.type == arg.type )
				{
					// Do not call copy constructors - just move.
					if( expr.node != nullptr )
						function_context.variables_state.MoveNode( expr.node );
					llvm_args[j]= expr.llvm_value;
				}
				else
				{
					if( !arg.type.IsCopyConstructible() )
					{
						// Can not call function with value parameter, because for value parameter needs copy, but parameter type is not copyable.
						// TODO - print more reliable message.
						REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), file_pos, arg.type );
						continue;
					}

					// Create copy of class or tuple value. Call copy constructor.
					llvm::Value* const arg_copy= function_context.alloca_ir_builder.CreateAlloca( arg.type.GetLLVMType() );
					llvm_args[j]= arg_copy;
					BuildCopyConstructorPart(
						arg_copy,
						CreateReferenceCast( expr.llvm_value, expr.type, arg.type, function_context ),
						arg.type,
						function_context );
				}
			}
			else U_ASSERT( false );
		}

		// Destroy unused temporary variables after each argument evaluation.
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), call_file_pos );
	} // for args
	U_ASSERT( locked_args_references.size() == arg_count );
	if( evaluate_args_in_reverse_order )
		std::reverse( locked_args_references.begin(), locked_args_references.end() );

	const bool return_value_is_sret=
		!function_type.return_value_is_reference &&
		( function_type.return_type.GetClassType() != nullptr || function_type.return_type.GetTupleType() != nullptr );

	llvm::Value* s_ret_value= nullptr;
	if( return_value_is_sret )
	{
		s_ret_value= function_context.alloca_ir_builder.CreateAlloca( function_type.return_type.GetLLVMType() );
		llvm_args.insert( llvm_args.begin(), s_ret_value );
		constant_llvm_args.insert( constant_llvm_args.begin(), nullptr );
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
				constexpr_function_evaluator_.Evaluate( function_type, llvm::dyn_cast<llvm::Function>(function), constant_llvm_args, call_file_pos );

			names.GetErrors().insert( names.GetErrors().end(), evaluation_result.errors.begin(), evaluation_result.errors.end() );
			if( evaluation_result.errors.empty() && evaluation_result.result_constant != nullptr )
			{
				if( return_value_is_sret ) // We needs here block of memory with result constant struct.
					MoveConstantToMemory( s_ret_value, evaluation_result.result_constant, function_context );

				call_result= evaluation_result.result_constant;
				constant_call_result= evaluation_result.result_constant;
			}
		}
		if( call_result == nullptr )
			call_result= function_context.llvm_ir_builder.CreateCall( function, llvm_args );
	}
	else
		call_result= llvm::UndefValue::get( llvm::dyn_cast<llvm::FunctionType>(function->getType())->getReturnType() );

	if( return_value_is_sret )
	{
		U_ASSERT( s_ret_value != nullptr );
		call_result= s_ret_value;
	}

	Variable result;
	result.type= function_type.return_type;
	result.llvm_value= call_result;
	result.constexpr_value= constant_call_result;

	ReferencesGraphNodePtr result_node;
	if( function_type.return_value_is_reference )
	{
		result.location= Variable::Location::Pointer;
		result.value_type= function_type.return_value_is_mutable ? ValueType::Reference : ValueType::ConstReference;
		result_node= std::make_shared<ReferencesGraphNode>( "fn_result "_SpC + result.type.ToString(), function_type.return_value_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
	}
	else
	{
		if( function_type.return_type != void_type_ && !EnsureTypeCompleteness( function_type.return_type, TypeCompleteness::Complete ) )
			REPORT_ERROR( UsingIncompleteType, names.GetErrors(), call_file_pos, function_type.return_type );

		result.location= return_value_is_sret ? Variable::Location::Pointer : Variable::Location::LLVMRegister;
		result.value_type= ValueType::Value;
		result_node= std::make_shared<ReferencesGraphNode>( "fn_result "_SpC + result.type.ToString(), ReferencesGraphNode::Kind::Variable );
	}
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( result_node, result ) );
	result.node= result_node;

	// Prepare result references.
	if( function_type.return_value_is_reference )
	{
		for( const size_t arg_n : function_type.return_references.args_references )
		{
			U_ASSERT( arg_n < locked_args_references.size() );
			function_context.variables_state.AddLink( locked_args_references[arg_n].Node(), result_node );
		}
		for( const Function::ArgReference& arg_reference : function_type.return_references.inner_args_references )
		{
			for( const ReferencesGraphNodePtr& accesible_node : function_context.variables_state.GetAllAccessibleInnerNodes_r( locked_args_references[arg_reference.first].Node() ) )
				function_context.variables_state.AddLink( accesible_node, result_node );
		}
	}
	else if( function_type.return_type.ReferencesTagsCount() > 0u )
	{
		bool inner_reference_is_mutable= false;

		// First, know, what kind of reference we needs - mutable or immutable.
		for( const size_t arg_n : function_type.return_references.args_references )
		{
			U_ASSERT( arg_n < locked_args_references.size() );
			const auto node_kind= locked_args_references[arg_n].Node()->kind;

			if( node_kind == ReferencesGraphNode::Kind::Variable ||
				node_kind == ReferencesGraphNode::Kind::ReferenceMut )
				inner_reference_is_mutable= true;
			else if( node_kind == ReferencesGraphNode::Kind::ReferenceImut ) {}
			else U_ASSERT( false ); // Unexpected node kind.
		}
		for( const Function::ArgReference& arg_reference : function_type.return_references.inner_args_references )
		{
			for( const ReferencesGraphNodePtr& accesible_node : function_context.variables_state.GetAllAccessibleInnerNodes_r( locked_args_references[arg_reference.first].Node() ) )
			{
				if( accesible_node->kind == ReferencesGraphNode::Kind::Variable ||
					accesible_node->kind == ReferencesGraphNode::Kind::ReferenceMut )
					inner_reference_is_mutable= true;
				else if( accesible_node->kind == ReferencesGraphNode::Kind::ReferenceImut ) {}
				else U_ASSERT( false ); // Unexpected node kind.
			}
		}

		// Then, create inner node and link input nodes with it.
		const auto inner_reference_node=
			std::make_shared<ReferencesGraphNode>( "inner_node "_SpC + result_node->name, inner_reference_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
		function_context.variables_state.SetNodeInnerReference( result_node, inner_reference_node );

		for( const size_t arg_n : function_type.return_references.args_references )
		{
			U_ASSERT( arg_n < locked_args_references.size() );
			function_context.variables_state.AddLink( locked_args_references[arg_n].Node(), inner_reference_node );
		}
		for( const Function::ArgReference& arg_reference : function_type.return_references.inner_args_references )
		{
			for( const ReferencesGraphNodePtr& accesible_node : function_context.variables_state.GetAllAccessibleInnerNodes_r( locked_args_references[arg_reference.first].Node() ) )
				function_context.variables_state.AddLink( accesible_node, inner_reference_node );
		}
	}

	// Setup references after call.
	for( const Function::ReferencePollution& referene_pollution : function_type.references_pollution )
	{
		const size_t dst_arg= referene_pollution.dst.first;
		U_ASSERT( dst_arg < function_type.args.size() );
		U_ASSERT( function_type.args[ dst_arg ].type.ReferencesTagsCount() > 0u );

		bool src_variables_is_mutable= referene_pollution.src_is_mutable;
		std::unordered_set<ReferencesGraphNodePtr> src_nodes;
		if( referene_pollution.src.second == Function::c_arg_reference_tag_number )
		{
			// Reference-arg itself
			U_ASSERT( function_type.args[ referene_pollution.src.first ].is_reference );
			src_nodes.emplace( locked_args_references[ referene_pollution.src.first ].Node() );

			if( !function_type.args[ referene_pollution.src.first ].is_mutable )
				src_variables_is_mutable= false; // Even if reference-pollution is mutable, but if src vars is immutable, link as immutable.
		}
		else
		{
			// Variables, referenced by inner argument references.
			U_ASSERT( referene_pollution.src.second == 0u );// Currently we support one tag per struct.
			U_ASSERT( function_type.args[ referene_pollution.src.first ].type.ReferencesTagsCount() > 0u );

			bool all_in_references_is_imut= true;
			for( const ReferencesGraphNodePtr& inner_reference : function_context.variables_state.GetAllAccessibleInnerNodes_r( locked_args_references[ referene_pollution.src.first ].Node() ) )
			{
				src_nodes.insert( inner_reference );
				if( inner_reference->kind != ReferencesGraphNode::Kind::ReferenceImut )
					all_in_references_is_imut= false;
			}
			if( all_in_references_is_imut )
				src_variables_is_mutable= false; // Even if reference-pollution is mutable, but if src vars is immutable, link as immutable.
		}

		if( function_type.args[ dst_arg ].is_reference )
		{
			for( const ReferencesGraphNodePtr& dst_node : function_context.variables_state.GetAllAccessibleVariableNodes_r( locked_args_references[ dst_arg ].Node() ) )
			{
				ReferencesGraphNodePtr inner_reference= function_context.variables_state.GetNodeInnerReference( dst_node );
				if( inner_reference == nullptr )
				{
					inner_reference=
						std::make_shared<ReferencesGraphNode>(
							"arg"_SpC + ToProgramString(std::to_string(dst_arg)) + "_inner variable"_SpC,
							src_variables_is_mutable ? ReferencesGraphNode::Kind::ReferenceMut : ReferencesGraphNode::Kind::ReferenceImut );
					function_context.variables_state.SetNodeInnerReference( dst_node, inner_reference );
				}
				if( inner_reference->kind != ReferencesGraphNode::Kind::ReferenceMut && src_variables_is_mutable )
					REPORT_ERROR( NotImplemented, names.GetErrors(), call_file_pos, "changind inner node reference kind immutable to mutable" );

				for( const ReferencesGraphNodePtr& src_node : src_nodes )
					function_context.variables_state.AddLink( src_node, inner_reference );
			}
		}
		else
		{
			// Does it have sence, write references to value argument?
		}
	}

	locked_args_inner_references.clear();
	locked_args_references.clear();
	{ // Destroy unused temporary variables after each call.
		const ReferencesGraphNodeHolder call_result_lock(
			std::make_shared<ReferencesGraphNode>( "lock "_SpC + result_node->name, ReferencesGraphNode::Kind::ReferenceImut ),
			function_context );
		function_context.variables_state.AddLink( result_node, call_result_lock.Node() );
		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), call_file_pos );
	}

	return Value( std::move(result), call_file_pos );
}

Variable CodeBuilder::BuildTempVariableConstruction(
	const Type& type,
	const Synt::CallOperator& call_operator,
	NamesScope& names,
	FunctionContext& function_context )
{
	if( !EnsureTypeCompleteness( type, TypeCompleteness::Complete ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), call_operator.file_pos_, type );
		return Variable();
	}
	else if( type.IsAbstract() )
		REPORT_ERROR( ConstructingAbstractClassOrInterface, names.GetErrors(), call_operator.file_pos_, type );

	Variable variable;
	variable.type= type;
	variable.location= Variable::Location::Pointer;
	variable.value_type= ValueType::Reference;
	variable.llvm_value= function_context.alloca_ir_builder.CreateAlloca( type.GetLLVMType() );

	const ReferencesGraphNodePtr node= std::make_shared<ReferencesGraphNode>( "temp "_SpC + type.ToString(), ReferencesGraphNode::Kind::Variable );
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( node, variable ) );

	// Lock variable, for preventing of temporary destruction.
	const ReferencesGraphNodeHolder variable_lock(
		std::make_shared<ReferencesGraphNode>( type.ToString() + " temp variable lock"_SpC, ReferencesGraphNode::Kind::ReferenceMut ),
		function_context );
	function_context.variables_state.AddLink( node, variable_lock.Node() );
	variable.node= variable_lock.Node();

	variable.constexpr_value= ApplyConstructorInitializer( variable, call_operator, names, function_context );
	variable.value_type= ValueType::Value; // Make value after construction

	variable.node= node;

	return variable;
}

Variable CodeBuilder::ConvertVariable(
	const Variable& variable,
	const Type& dst_type,
	const FunctionVariable& conversion_constructor,
	NamesScope& names,
	FunctionContext& function_context,
	const FilePos& file_pos )
{
	if( !EnsureTypeCompleteness( dst_type, TypeCompleteness::Complete ) )
	{
		REPORT_ERROR( UsingIncompleteType, names.GetErrors(), file_pos, dst_type );
		return Variable();
	}

	Variable result;
	result.type= dst_type;
	result.location= Variable::Location::Pointer;
	result.value_type= ValueType::Reference;
	result.llvm_value= function_context.alloca_ir_builder.CreateAlloca( dst_type.GetLLVMType() );

	const ReferencesGraphNodePtr node= std::make_shared<ReferencesGraphNode>( "temp "_SpC + dst_type.ToString(), ReferencesGraphNode::Kind::Variable );
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( node, result ) );

	// Lock variables, for preventing of temporary destruction.
	const ReferencesGraphNodeHolder src_variable_lock(
		std::make_shared<ReferencesGraphNode>( variable.type.ToString() + " variable lock"_SpC, ReferencesGraphNode::Kind::ReferenceImut ),
		function_context );
	if( variable.node != nullptr )
		function_context.variables_state.AddLink( variable.node, src_variable_lock.Node() );

	const ReferencesGraphNodeHolder dst_variable_lock(
		std::make_shared<ReferencesGraphNode>( dst_type.ToString() + " variable lock"_SpC, ReferencesGraphNode::Kind::ReferenceMut ),
		function_context );
	function_context.variables_state.AddLink( node, dst_variable_lock.Node() );
	result.node= dst_variable_lock.Node();

	DoCallFunction(
		conversion_constructor.llvm_function,
		*conversion_constructor.type.GetFunctionType(),
		file_pos,
		{ result, variable },
		{},
		false,
		names,
		function_context,
		false );

	result.node= node;

	result.value_type= ValueType::Value; // Make value after construction
	return result;
}

Value CodeBuilder::BuildUnaryMinus(
	const Value& value,
	const Synt::UnaryMinus& unary_minus,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);
	if( value.GetVariable() == nullptr )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), unary_minus.file_pos_, value.GetKindName() );
		return ErrorValue();
	}
	const Variable& variable= *value.GetVariable();

	const FundamentalType* const fundamental_type= variable.type.GetFundamentalType();
	if( fundamental_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), unary_minus.file_pos_, value.GetKindName() );
		return ErrorValue();
	}

	const bool is_float= IsFloatingPoint( fundamental_type->fundamental_type );
	if( !( IsInteger( fundamental_type->fundamental_type ) || is_float ) )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), unary_minus.file_pos_, variable.type );
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

	const auto node= std::make_shared<ReferencesGraphNode>( OverloadedOperatorToString(OverloadedOperator::Sub), ReferencesGraphNode::Kind::Variable );
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( node, result ) );
	result.node= node;
	return Value( std::move(result), unary_minus.file_pos_ );
}

Value CodeBuilder::BuildLogicalNot(
	const Value& value,
	const Synt::LogicalNot& logical_not,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);
	if( value.GetVariable() == nullptr )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), logical_not.file_pos_, value.GetKindName() );
		return ErrorValue();
	}
	const Variable& variable= *value.GetVariable();

	if( variable.type != bool_type_ )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), logical_not.file_pos_, value.GetKindName() );
		return ErrorValue();
	}

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

	const auto node= std::make_shared<ReferencesGraphNode>( OverloadedOperatorToString(OverloadedOperator::LogicalNot), ReferencesGraphNode::Kind::Variable );
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( node, result ) );
	result.node= node;
	return Value( std::move(result), logical_not.file_pos_ );
}

Value CodeBuilder::BuildBitwiseNot(
	const Value& value,
	const Synt::BitwiseNot& bitwise_not,
	NamesScope& names,
	FunctionContext& function_context )
{
	CHECK_RETURN_ERROR_VALUE(value);
	if( value.GetVariable() == nullptr )
	{
		REPORT_ERROR( ExpectedVariable, names.GetErrors(), bitwise_not.file_pos_, value.GetKindName() );
		return ErrorValue();
	}
	const Variable& variable= *value.GetVariable();

	const FundamentalType* const fundamental_type= variable.type.GetFundamentalType();
	if( fundamental_type == nullptr )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), bitwise_not.file_pos_, value.GetKindName() );
		return ErrorValue();
	}
	if( !IsInteger( fundamental_type->fundamental_type ) )
	{
		REPORT_ERROR( OperationNotSupportedForThisType, names.GetErrors(), bitwise_not.file_pos_, value.GetKindName() );
		return ErrorValue();
	}

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

	const auto node= std::make_shared<ReferencesGraphNode>( OverloadedOperatorToString(OverloadedOperator::BitwiseNot), ReferencesGraphNode::Kind::Variable );
	function_context.stack_variables_stack.back()->RegisterVariable( std::make_pair( node, result ) );
	result.node= node;
	return Value( std::move(result), bitwise_not.file_pos_ );
}

} // namespace CodeBuilderPrivate

} // namespace U
