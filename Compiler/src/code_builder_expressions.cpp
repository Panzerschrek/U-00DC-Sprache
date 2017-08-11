#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include "pop_llvm_warnings.hpp"

#include "assert.hpp"
#include "keywords.hpp"
#include "lang_types.hpp"

#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

Value CodeBuilder::BuildExpressionCode(
	const IExpressionComponent& expression,
	const NamesScope& names,
	FunctionContext& function_context )
{
	const FilePos file_pos = expression.file_pos_;

	Value result;

	if( const BinaryOperator* const binary_operator=
		dynamic_cast<const BinaryOperator*>(&expression) )
	{
		const Value l_var_value=
			BuildExpressionCode(
				*binary_operator->left_,
				names,
				function_context );

		const Value r_var_value=
			BuildExpressionCode(
				*binary_operator->right_,
				names,
				function_context );

		const Variable* const l_var= l_var_value.GetVariable();
		const Variable* const r_var= r_var_value.GetVariable();

		if( l_var == nullptr )
			errors_.push_back( ReportExpectedVariableInBinaryOperator( file_pos, l_var_value.GetType().ToString() ) );
		if( r_var == nullptr )
			errors_.push_back( ReportExpectedVariableInBinaryOperator( file_pos, r_var_value.GetType().ToString() ) );
		if( l_var == nullptr || r_var == nullptr )
			throw ProgramError();

		return BuildBinaryOperator( *l_var, *r_var, binary_operator->operator_type_, file_pos, function_context );
	}
	else if( const NamedOperand* const named_operand=
		dynamic_cast<const NamedOperand*>(&expression) )
	{
		result= BuildNamedOperand( *named_operand, names, function_context );
	}
	else if( const NumericConstant* numeric_constant=
		dynamic_cast<const NumericConstant*>(&expression) )
	{
		result= BuildNumericConstant( *numeric_constant );
	}
	else if( const BooleanConstant* boolean_constant=
		dynamic_cast<const BooleanConstant*>(&expression) )
	{
		result= BuildBooleanConstant( *boolean_constant );
	}
	else if( const BracketExpression* bracket_expression=
		dynamic_cast<const BracketExpression*>(&expression) )
	{
		result= BuildExpressionCode( *bracket_expression->expression_, names, function_context );
	}
	else
	{
		// TODO
		U_ASSERT(false);
	}

	if( const ExpressionComponentWithUnaryOperators* const expression_with_unary_operators=
		dynamic_cast<const ExpressionComponentWithUnaryOperators*>( &expression ) )
	{
		for( const IUnaryPostfixOperatorPtr& postfix_operator : expression_with_unary_operators->postfix_operators_ )
		{
			if( const IndexationOperator* const indexation_operator=
				dynamic_cast<const IndexationOperator*>( postfix_operator.get() ) )
			{
				result= BuildIndexationOperator( result, *indexation_operator, names, function_context );
			}
			else if( const MemberAccessOperator* const member_access_operator=
				dynamic_cast<const MemberAccessOperator*>( postfix_operator.get() ) )
			{
				result= BuildMemberAccessOperator( result, *member_access_operator, function_context );
			}
			else if( const CallOperator* const call_operator=
				dynamic_cast<const CallOperator*>( postfix_operator.get() ) )
			{
				result= BuildCallOperator( result, *call_operator, names, function_context );
			}
			else
			{
				U_ASSERT(false);
			}
		} // for unary postfix operators

		for( const IUnaryPrefixOperatorPtr& prefix_operator : expression_with_unary_operators->prefix_operators_ )
		{
			if( const UnaryMinus* const unary_minus=
				dynamic_cast<const UnaryMinus*>( prefix_operator.get() ) )
			{
				result= BuildUnaryMinus( result, *unary_minus, function_context );
			}
			else if( const UnaryPlus* const unary_plus=
				dynamic_cast<const UnaryPlus*>( prefix_operator.get() ) )
			{
				(void)unary_plus;
				// DO NOTHING
			}
			// TODO
		} // for unary prefix operators

		return result;
	}
}

Variable CodeBuilder::BuildBinaryOperator(
	const Variable& l_var,
	const Variable& r_var,
	const BinaryOperatorType binary_operator,
	const FilePos& file_pos,
	FunctionContext& function_context )
{
	Variable result;

	// SPRACHE_-TODO - add cast for some integers here.
	if( r_var.type != l_var.type )
	{
		errors_.push_back( ReportNoMatchBinaryOperatorForGivenTypes( file_pos, r_var.type.ToString(), l_var.type.ToString(), BinaryOperatorToString( binary_operator ) ) );
		throw ProgramError();
	}

	const Type& result_type= r_var.type;
	const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &result_type.one_of_type_kind );

	switch( binary_operator )
	{
	case BinaryOperatorType::Add:
	case BinaryOperatorType::Sub:
	case BinaryOperatorType::Div:
	case BinaryOperatorType::Mul:

		if( fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
			throw ProgramError();
		}
		else
		{
			if( result_type.SizeOf() < 4u )
			{
				// Operation supported only for 32 and 64bit operands
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}
			const bool is_float= IsFloatingPoint( fundamental_type->fundamental_type );
			if( !( IsInteger( fundamental_type->fundamental_type ) || is_float ) )
			{
				// this operations allowed only for integer and floating point operands.
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}

			const bool is_signed= IsSignedInteger( fundamental_type->fundamental_type );

			llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			llvm::Value* result_value;

			switch( binary_operator )
			{
			case BinaryOperatorType::Add:
				if( is_float )
					result_value=
						function_context.llvm_ir_builder.CreateFAdd( l_value_for_op, r_value_for_op );
				else
					result_value=
						function_context.llvm_ir_builder.CreateAdd( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Sub:
				if( is_float )
					result_value=
						function_context.llvm_ir_builder.CreateFSub( l_value_for_op, r_value_for_op );
				else
					result_value=
						function_context.llvm_ir_builder.CreateSub( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Div:
				if( is_float )
					result_value=
						function_context.llvm_ir_builder.CreateFDiv( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value=
						function_context.llvm_ir_builder.CreateSDiv( l_value_for_op, r_value_for_op );
				else
					result_value=
						function_context.llvm_ir_builder.CreateUDiv( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Mul:
				if( is_float )
					result_value=
						function_context.llvm_ir_builder.CreateFMul( l_value_for_op, r_value_for_op );
				else
					result_value=
						function_context.llvm_ir_builder.CreateMul( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type= r_var.type;
			result.llvm_value= result_value;
		}
		break;


	case BinaryOperatorType::Equal:
	case BinaryOperatorType::NotEqual:

		if( fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
			throw ProgramError();
		}
		else
		{
			const bool if_float= IsFloatingPoint( fundamental_type->fundamental_type );
			if( !( IsInteger( fundamental_type->fundamental_type ) || if_float || fundamental_type->fundamental_type == U_FundamentalType::Bool ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			llvm::Value* result_value;

			switch( binary_operator )
			{
			// TODO - select ordered/unordered comparision flags for floats.
			case BinaryOperatorType::Equal:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUEQ( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpEQ( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::NotEqual:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUNE( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpNE( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type.one_of_type_kind= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );
			result.llvm_value= result_value;
		}
		break;

	case BinaryOperatorType::Less:
	case BinaryOperatorType::LessEqual:
	case BinaryOperatorType::Greater:
	case BinaryOperatorType::GreaterEqual:

		if( fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
			throw ProgramError();
		}
		else
		{
			const bool if_float= IsFloatingPoint( fundamental_type->fundamental_type );
			const bool is_signed= IsSignedInteger( fundamental_type->fundamental_type );
			if( !( IsInteger( fundamental_type->fundamental_type ) || if_float ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			llvm::Value* result_value;

			switch( binary_operator )
			{
			// TODO - select ordered/unordered comparision flags for floats.
			case BinaryOperatorType::Less:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpULT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSLT( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpULT( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::LessEqual:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpULE( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSLE( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpULE( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::Greater:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUGT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSGT( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpUGT( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperatorType::GreaterEqual:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUGE( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSGE( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpUGE( l_value_for_op, r_value_for_op );
				break;

			default: U_ASSERT( false ); break;
			};

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type.one_of_type_kind= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );
			result.llvm_value= result_value;
		}
		break;

	case BinaryOperatorType::And:
	case BinaryOperatorType::Or:
	case BinaryOperatorType::Xor:

		if( fundamental_type == nullptr )
		{
			errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
			throw ProgramError();
		}
		else
		{
			if( !( IsInteger( fundamental_type->fundamental_type ) || fundamental_type->fundamental_type == U_FundamentalType::Bool ) )
			{
				errors_.push_back( ReportOperationNotSupportedForThisType( file_pos, result_type.ToString() ) );
				throw ProgramError();
			}

			llvm::Value* l_value_for_op= CreateMoveToLLVMRegisterInstruction( l_var, function_context );
			llvm::Value* r_value_for_op= CreateMoveToLLVMRegisterInstruction( r_var, function_context );
			llvm::Value* result_value;

			switch( binary_operator )
			{
			case BinaryOperatorType::And:
				result_value=
					function_context.llvm_ir_builder.CreateAnd( l_value_for_op, r_value_for_op );
				break;
			case BinaryOperatorType::Or:
				result_value=
					function_context.llvm_ir_builder.CreateOr( l_value_for_op, r_value_for_op );
				break;
			case BinaryOperatorType::Xor:
				result_value=
					function_context.llvm_ir_builder.CreateXor( l_value_for_op, r_value_for_op );
				break;
			default: U_ASSERT( false ); break;
			};

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type= result_type;
			result.llvm_value= result_value;
		}
		break;

	case BinaryOperatorType::LazyLogicalAnd:
	case BinaryOperatorType::LazyLogicalOr:
	case BinaryOperatorType::Last:
		break;
	};

	return result;
}

Value CodeBuilder::BuildNamedOperand(
	const NamedOperand& named_operand,
	const NamesScope& names,
	FunctionContext& function_context )
{
	if( named_operand.name_.components.size() == 1u &&
		named_operand.name_.components.back() == Keywords::this_ )
	{
		if( function_context.this_ == nullptr || function_context.is_constructor_initializer_list_now )
		{
			errors_.push_back( ReportThisUnavailable( named_operand.file_pos_ ) );
			throw ProgramError();
		}
		return *function_context.this_;
	}

	const NamesScope::InsertedName* name_entry=
		names.ResolveName( named_operand.name_ );
	if( !name_entry )
	{
		errors_.push_back( ReportNameNotFound( named_operand.file_pos_, named_operand.name_ ) );
		throw ProgramError();
	}

	if( const ClassField* const field= name_entry->second.GetClassField() )
	{
		if( function_context.this_ == nullptr )
		{
			errors_.push_back( ReportClassFiledAccessInStaticMethod( named_operand.file_pos_, named_operand.name_.components.back() ) );
			throw ProgramError();
		}

		const ClassPtr class_= field->class_.lock();
		U_ASSERT( class_ != nullptr  && "Class is dead? WTF?" );

		// SPRACHE_TODO - allow access to parents fields here.
		Type class_type;
		class_type.one_of_type_kind= class_;
		if( class_type != function_context.this_->type )
		{
			// SPRACHE_TODO - accessing field of non-this class.
			errors_.push_back( ReportNotImplemented( named_operand.file_pos_, "classes inside other classes" ) );
			throw ProgramError();
		}

		if( function_context.is_constructor_initializer_list_now &&
			function_context.uninitialized_this_fields.find( field ) != function_context.uninitialized_this_fields.end() )
		{
			errors_.push_back( ReportFieldIsNotInitializedYet( named_operand.file_pos_, named_operand.name_.components.back() ) );
			throw ProgramError();
		}

		Variable field_variable;
		field_variable.type= field->type;
		field_variable.location= Variable::Location::Pointer;
		field_variable.value_type= function_context.this_->value_type;

		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
		field_variable.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( function_context.this_->llvm_value, index_list );

		return field_variable;
	}
	else if( const OverloadedFunctionsSet* const overloaded_functions_set=
		name_entry->second.GetFunctionsSet() )
	{
		if( function_context.this_ != nullptr )
		{
			if( function_context.is_constructor_initializer_list_now )
			{
				// SPRACHE_TODO - allow call of static methods and parents methods.
				errors_.push_back( ReportMethodsCallInConstructorInitializerListIsForbidden( named_operand.file_pos_, named_operand.name_.components.back() ) );
				throw ProgramError();
			}

			// Trying add "this" to functions set.
			const ClassPtr class_= boost::get<ClassPtr>( function_context.this_->type.one_of_type_kind );

			const NamesScope::InsertedName* const same_set_in_class=
				class_->members.GetThisScopeName( named_operand.name_.components.back() );
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

	return name_entry->second;
}

Variable CodeBuilder::BuildNumericConstant( const NumericConstant& numeric_constant )
{
	U_FundamentalType type= GetNumericConstantType( numeric_constant );
	if( type == U_FundamentalType::InvalidType )
	{
		errors_.push_back( ReportUnknownNumericConstantType( numeric_constant.file_pos_, numeric_constant.type_suffix_ ) );
		throw ProgramError();
	}
	llvm::Type* const llvm_type= GetFundamentalLLVMType( type );

	Variable result;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;
	result.type.one_of_type_kind= FundamentalType( type, llvm_type );

	if( IsInteger( type ) )
		result.llvm_value=
			llvm::Constant::getIntegerValue( llvm_type, llvm::APInt( result.type.SizeOf() * 8u, uint64_t(numeric_constant.value_) ) );
	else if( IsFloatingPoint( type ) )
		result.llvm_value=
			llvm::ConstantFP::get( llvm_type, static_cast<double>( numeric_constant.value_) );
	else
	{
		U_ASSERT(false);
	}

	return result;
}

Variable CodeBuilder::BuildBooleanConstant( const BooleanConstant& boolean_constant )
{
	Variable result;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;
	result.type.one_of_type_kind= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );

	result.llvm_value=
		llvm::Constant::getIntegerValue(
			fundamental_llvm_types_.bool_ ,
			llvm::APInt( 1u, uint64_t(boolean_constant.value_) ) );

	return result;
}

Variable CodeBuilder::BuildIndexationOperator(
	const Value& value,
	const IndexationOperator& indexation_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	const ArrayPtr* const array_type= boost::get<ArrayPtr>( &value.GetType().one_of_type_kind );
	if( array_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( indexation_operator.file_pos_, value.GetType().ToString() ) );
		throw ProgramError();
	}
	U_ASSERT( *array_type != nullptr );
	const Variable& variable= *value.GetVariable();

	const Value index_value=
		BuildExpressionCode(
			*indexation_operator.index_,
			names,
			function_context );

	const FundamentalType* const index_fundamental_type= boost::get<FundamentalType>( & index_value.GetType().one_of_type_kind );
	if( index_fundamental_type == nullptr ||
		!IsUnsignedInteger( index_fundamental_type->fundamental_type ) )
	{
		errors_.push_back( ReportTypesMismatch( indexation_operator.file_pos_, "any unsigned integer"_SpC, index_value.GetType().ToString() ) );
		throw ProgramError();
	}
	const Variable& index= *index_value.GetVariable();

	if( variable.location != Variable::Location::Pointer )
	{
		// TODO - Strange variable location.
		throw ProgramError();
	}

	Variable result;
	result.location= Variable::Location::Pointer;
	result.value_type= variable.value_type;
	result.type= (*array_type)->type;

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
	index_list[1]= CreateMoveToLLVMRegisterInstruction( index, function_context );

	result.llvm_value=
		function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef< llvm::Value*> ( index_list, 2u ) );

	return result;
}

Value CodeBuilder::BuildMemberAccessOperator(
	const Value& value,
	const MemberAccessOperator& member_access_operator,
	FunctionContext& function_context )
{
	const ClassPtr* const class_type= boost::get<ClassPtr>( &value.GetType().one_of_type_kind );
	if( class_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( member_access_operator.file_pos_, value.GetType().ToString() ) );
		throw ProgramError();
	}
	U_ASSERT( *class_type != nullptr );

	if( (*class_type)->is_incomplete )
	{
		errors_.push_back( ReportUsingIncompleteType( member_access_operator.file_pos_, value.GetType().ToString() ) );
		throw ProgramError();
	}

	const Variable& variable= *value.GetVariable();

	const NamesScope::InsertedName* const class_member= (*class_type)->members.GetThisScopeName( member_access_operator.member_name_ );
	if( class_member == nullptr )
	{
		errors_.push_back( ReportNameNotFound( member_access_operator.file_pos_, member_access_operator.member_name_ ) );
		throw ProgramError();
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
		throw ProgramError();
	}

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
	index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );

	Variable result;
	result.location= Variable::Location::Pointer;
	result.value_type= variable.value_type;
	result.type= field->type;
	result.llvm_value=
		function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, index_list );
	return result;
}

Variable CodeBuilder::BuildCallOperator(
	const Value& function_value,
	const CallOperator& call_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
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
		throw ProgramError();
	}

	size_t this_count= this_ == nullptr ? 0u : 1u;
	size_t total_args= this_count + call_operator.arguments_.size();
	std::vector<Function::Arg> actual_args;
	std::vector<Variable> actual_args_variables;
	actual_args.reserve( total_args );
	actual_args_variables.reserve( total_args );

	// Push "this" argument.
	if( this_ != nullptr )
	{
		actual_args.emplace_back();
		actual_args.back().type= this_->type;
		actual_args.back().is_reference= true;
		actual_args.back().is_mutable= this_->value_type == ValueType::Reference;

		actual_args_variables.push_back( *this_ );
	}
	// Push arguments from call operator.
	for( const IExpressionComponentPtr& arg_expression : call_operator.arguments_ )
	{
		U_ASSERT( arg_expression != nullptr );
		const Value expr_value= BuildExpressionCode( *arg_expression, names, function_context );
		const Variable* const expr= expr_value.GetVariable();
		if( expr == nullptr )
		{
			errors_.push_back( ReportExpectedVariableAsArgument( arg_expression->file_pos_, expr_value.GetType().ToString() ) );
			throw ProgramError();
		}

		actual_args.emplace_back();
		actual_args.back().type= expr->type;
		actual_args.back().is_reference= expr->value_type != ValueType::Value;
		actual_args.back().is_mutable= expr->value_type == ValueType::Reference;

		actual_args_variables.push_back( std::move(*expr) );
	}

	// SPRACHE_TODO - try get function with "this" parameter in signature and without it.
	// We must support static functions call using "this".
	const FunctionVariable& function=
		GetOverloadedFunction( *functions_set, actual_args, this_ != nullptr, call_operator.file_pos_ );
	const Function& function_type= *boost::get<FunctionPtr>( function.type.one_of_type_kind );

	if( this_ != nullptr && !function.is_this_call )
	{
		// Static function call via "this".
		// Just dump first "this" arg.
		this_count--;
		total_args--;
		actual_args.erase( actual_args.begin() );
		actual_args_variables.erase( actual_args_variables.begin() );
	}

	if( this_ == nullptr && function.is_this_call )
	{
		errors_.push_back( ReportCallOfThiscallFunctionUsingNonthisArgument( call_operator.file_pos_ ) );
		throw ProgramError();
	}

	if( function_type.args.size() != actual_args.size() )
	{
		errors_.push_back( ReportFunctionSignatureMismatch( call_operator.file_pos_ ) );
		throw ProgramError();
	}

	std::vector<llvm::Value*> llvm_args;

	llvm::Value* s_ret_value= nullptr;
	if( function.return_value_is_sret )
	{
		s_ret_value= function_context.alloca_ir_builder.CreateAlloca( function_type.return_type.GetLLVMType() );
		llvm_args.push_back( s_ret_value );
	}

	for( unsigned int i= 0u; i < actual_args.size(); i++ )
	{
		const Function::Arg& arg= function_type.args[i];
		const Variable& expr= actual_args_variables[i];

		const FilePos& file_pos=
			( this_count != 0u && i == 0u )
				? call_operator.file_pos_
				: call_operator.arguments_[i - this_count]->file_pos_;

		if( expr.type != arg.type )
		{
			errors_.push_back( ReportFunctionSignatureMismatch( file_pos ) );
			throw ProgramError();
		}

		if( arg.is_reference )
		{
			if( arg.is_mutable )
			{
				if( expr.value_type == ValueType::Value )
				{
					errors_.push_back( ReportExpectedReferenceValue( file_pos ) );
					throw ProgramError();
				}
				if( expr.value_type == ValueType::ConstReference )
				{
					errors_.push_back( ReportBindingConstReferenceToNonconstReference( file_pos ) );
					throw ProgramError();
				}

				llvm_args.push_back(expr.llvm_value);
			}
			else
			{
				if( expr.value_type == ValueType::Value )
				{
					// Bind value to const reference.
					// TODO - support nonfundamental values.
					if( expr.location == Variable::Location::LLVMRegister )
					{
						llvm::Value* temp_storage= function_context.alloca_ir_builder.CreateAlloca( expr.type.GetLLVMType() );
						function_context.llvm_ir_builder.CreateStore( expr.llvm_value, temp_storage );
						llvm_args.push_back( temp_storage );
					}
					else
						llvm_args.push_back( expr.llvm_value );
				}
				else
				{
					llvm_args.push_back( expr.llvm_value );
				}
			}
		}
		else
		{
			if( const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &arg.type.one_of_type_kind ) )
			{
				U_UNUSED(fundamental_type);
				llvm_args.push_back( CreateMoveToLLVMRegisterInstruction( expr, function_context ) );
			}
			else if( const ClassPtr* const class_type_ptr= boost::get<ClassPtr>( &arg.type.one_of_type_kind ) )
			{
				U_ASSERT( *class_type_ptr != nullptr );
				const Class& class_type= **class_type_ptr;

				if( !arg.type.IsCopyConstructible() )
				{
					// Can not call function with value parameter, because for value parameter needs copy, but parameter type is not copyable.
					// TODO - print more reliable message.
					errors_.push_back( ReportOperationNotSupportedForThisType( call_operator.file_pos_, arg.type.ToString() ) );
					continue;
				}

				// Create copy of class value. Call copy constructor.
				llvm::Value* const arg_copy= function_context.alloca_ir_builder.CreateAlloca( arg.type.GetLLVMType() );

				TryCallCopyConstructor( call_operator.file_pos_, arg_copy, expr.llvm_value, *class_type_ptr, function_context );
				llvm_args.push_back( arg_copy );
			}
			else
			{ U_ASSERT( false );}
		}
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
	}
	result.type= function_type.return_type;
	result.llvm_value= call_result;

	return result;
}

Variable CodeBuilder::BuildUnaryMinus(
	const Value& value,
	const UnaryMinus& unary_minus,
	FunctionContext& function_context )
{
	const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &value.GetType().one_of_type_kind );
	if( fundamental_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( unary_minus.file_pos_, value.GetType().ToString() ) );
		throw ProgramError();
	}
	const Variable variable= *value.GetVariable();

	const bool is_float= IsFloatingPoint( fundamental_type->fundamental_type );
	if( !( IsInteger( fundamental_type->fundamental_type ) || is_float ) )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( unary_minus.file_pos_, variable.type.ToString() ) );
		throw ProgramError();
	}
	// TODO - maybe not support unary minus for 8 and 16 bot integer types?

	Variable result;
	result.type= variable.type;
	result.location= Variable::Location::LLVMRegister;
	result.value_type= ValueType::Value;

	llvm::Value* value_for_neg= CreateMoveToLLVMRegisterInstruction( variable, function_context );
	if( is_float )
		result.llvm_value= function_context.llvm_ir_builder.CreateFNeg( value_for_neg );
	else
		result.llvm_value= function_context.llvm_ir_builder.CreateNeg( value_for_neg );

	return result;
}

} // namespace CodeBuilderPrivate

} // namespace U
