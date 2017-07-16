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

Variable CodeBuilder::BuildExpressionCode(
	const BinaryOperatorsChain& expression,
	const NamesScope& names,
	FunctionContext& function_context )
{
	const InversePolishNotation ipn= ConvertToInversePolishNotation( expression );

	return
		BuildExpressionCode_r(
			ipn,
			ipn.size() - 1,
			names,
			function_context );
}

Variable CodeBuilder::BuildExpressionCode_r(
	const InversePolishNotation& ipn,
	unsigned int ipn_index,
	const NamesScope& names,
	FunctionContext& function_context )
{
	U_ASSERT( ipn_index < ipn.size() );
	const InversePolishNotationComponent& comp= ipn[ ipn_index ];

	const FilePos file_pos = ipn.front().operand->file_pos_;

	if( comp.operator_ != BinaryOperator::None )
	{
		Variable l_var=
			BuildExpressionCode_r(
				ipn, comp.l_index,
				names,
				function_context );

		Variable r_var=
			BuildExpressionCode_r(
				ipn, comp.r_index,
				names,
				function_context );

		Variable result;

		// TODO - add cast for some integers here.
		if( r_var.type != l_var.type )
		{
			// TODO - report types mismatch.
			throw ProgramError();
		}

		const Type& result_type= r_var.type;
		const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &result_type.one_of_type_kind );

		switch( comp.operator_ )
		{
		case BinaryOperator::Add:
		case BinaryOperator::Sub:
		case BinaryOperator::Div:
		case BinaryOperator::Mul:

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

				switch( comp.operator_ )
				{
				case BinaryOperator::Add:
					if( is_float )
						result_value=
							function_context.llvm_ir_builder.CreateFAdd( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateAdd( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Sub:
					if( is_float )
						result_value=
							function_context.llvm_ir_builder.CreateFSub( l_value_for_op, r_value_for_op );
					else
						result_value=
							function_context.llvm_ir_builder.CreateSub( l_value_for_op, r_value_for_op );
					break;

				case BinaryOperator::Div:
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

				case BinaryOperator::Mul:
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


		case BinaryOperator::Equal:
		case BinaryOperator::NotEqual:

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

			switch( comp.operator_ )
			{
			// TODO - select ordered/unordered comparision flags for floats.
			case BinaryOperator::Equal:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUEQ( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpEQ( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::NotEqual:
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

		case BinaryOperator::Less:
		case BinaryOperator::LessEqual:
		case BinaryOperator::Greater:
		case BinaryOperator::GreaterEqual:
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

			switch( comp.operator_ )
			{
			// TODO - select ordered/unordered comparision flags for floats.
			case BinaryOperator::Less:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpULT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSLT( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpULT( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::LessEqual:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpULE( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSLE( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpULE( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::Greater:
				if( if_float )
					result_value= function_context.llvm_ir_builder.CreateFCmpUGT( l_value_for_op, r_value_for_op );
				else if( is_signed )
					result_value= function_context.llvm_ir_builder.CreateICmpSGT( l_value_for_op, r_value_for_op );
				else
					result_value= function_context.llvm_ir_builder.CreateICmpUGT( l_value_for_op, r_value_for_op );
				break;

			case BinaryOperator::GreaterEqual:
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

		case BinaryOperator::And:
		case BinaryOperator::Or:
		case BinaryOperator::Xor:
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

			switch( comp.operator_ )
			{
			case BinaryOperator::And:
				result_value=
					function_context.llvm_ir_builder.CreateAnd( l_value_for_op, r_value_for_op );
				break;
			case BinaryOperator::Or:
				result_value=
					function_context.llvm_ir_builder.CreateOr( l_value_for_op, r_value_for_op );
				break;
			case BinaryOperator::Xor:
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

		case BinaryOperator::LazyLogicalAnd:
		case BinaryOperator::LazyLogicalOr:
		case BinaryOperator::None:
		case BinaryOperator::Last:
			U_ASSERT(false);
			break;
		};

		return result;
	}
	else
	{
		U_ASSERT( comp.operand );
		U_ASSERT( comp.r_index == InversePolishNotationComponent::c_no_parent );
		U_ASSERT( comp.l_index == InversePolishNotationComponent::c_no_parent );

		const IBinaryOperatorsChainComponent& operand= *comp.operand;

		Variable result;

		if( const NamedOperand* const named_operand=
			dynamic_cast<const NamedOperand*>(&operand) )
		{
			const NamesScope::InsertedName* name_entry=
				names.GetName( named_operand->name_ );
			if( !name_entry )
			{
				errors_.push_back( ReportNameNotFound( named_operand->file_pos_, named_operand->name_ ) );
				throw ProgramError();
			}

			if( const Variable* const variable= boost::get<Variable>( &name_entry->second ) )
			{
				result= *variable;
			}
			else if( const OverloadedFunctionsSet* const functins_set=
				boost::get<OverloadedFunctionsSet>( &name_entry->second ) )
			{
				result.type.one_of_type_kind= NontypeStub::OverloadedFunctionsSet;
				result.functions_set= *functins_set;
			}
			else
			{
				// TODO - set type name.
				result.type.one_of_type_kind= NontypeStub::ClassName;
			}
		}
		else if( const NumericConstant* numeric_constant=
			dynamic_cast<const NumericConstant*>(&operand) )
		{
			U_FundamentalType type= GetNumericConstantType( *numeric_constant );
			if( type == U_FundamentalType::InvalidType )
			{
				errors_.push_back( ReportUnknownNumericConstantType( numeric_constant->file_pos_, numeric_constant->type_suffix_ ) );
				throw ProgramError();
			}
			llvm::Type* const llvm_type= GetFundamentalLLVMType( type );

			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type.one_of_type_kind= FundamentalType( type, llvm_type );

			if( IsInteger( type ) )
				result.llvm_value=
					llvm::Constant::getIntegerValue( llvm_type, llvm::APInt( result.type.SizeOf() * 8u, uint64_t(numeric_constant->value_) ) );
			else if( IsFloatingPoint( type ) )
				result.llvm_value=
					llvm::ConstantFP::get( llvm_type, static_cast<double>( numeric_constant->value_) );
			else
			{
				U_ASSERT(false);
			}
		}
		else if( const BooleanConstant* boolean_constant=
			dynamic_cast<const BooleanConstant*>(&operand) )
		{
			result.location= Variable::Location::LLVMRegister;
			result.value_type= ValueType::Value;
			result.type.one_of_type_kind= FundamentalType( U_FundamentalType::Bool, fundamental_llvm_types_.bool_ );

			result.llvm_value=
				llvm::Constant::getIntegerValue(
					fundamental_llvm_types_.bool_ ,
					llvm::APInt( 1u, uint64_t(boolean_constant->value_) ) );
		}
		else if( const BracketExpression* bracket_expression=
			dynamic_cast<const BracketExpression*>(&operand) )
		{
			result= BuildExpressionCode( *bracket_expression->expression_, names, function_context );
		}
		else
		{
			// TODO
			U_ASSERT(false);
		}

		for( const IUnaryPostfixOperatorPtr& postfix_operator : comp.postfix_operand_operators )
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

		for( const IUnaryPrefixOperatorPtr& prefix_operator : comp.prefix_operand_operators )
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

Variable CodeBuilder::BuildIndexationOperator(
	const Variable& variable,
	const IndexationOperator& indexation_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	const ArrayPtr* const array_type= boost::get<ArrayPtr>( &variable.type.one_of_type_kind );
	if( array_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( indexation_operator.file_pos_, variable.type.ToString() ) );
		throw ProgramError();
	}
	U_ASSERT( *array_type != nullptr );

	Variable index=
		BuildExpressionCode(
			*indexation_operator.index_,
			names,
			function_context );

	const FundamentalType* const index_fundamental_type= boost::get<FundamentalType>( &index.type.one_of_type_kind );
	if( index_fundamental_type == nullptr ||
		!IsUnsignedInteger( index_fundamental_type->fundamental_type ) )
	{
		errors_.push_back( ReportTypesMismatch( indexation_operator.file_pos_, "any unsigned integer"_SpC, index.type.ToString() ) );
		throw ProgramError();
	}

	if( variable.location != Variable::Location::Pointer )
	{
		// TODO - Strange variable location.
		throw ProgramError();
	}

	Variable result;
	result.location= Variable::Location::Pointer;
	result.value_type= ValueType::Reference;
	result.type= (*array_type)->type;

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
	index_list[1]= CreateMoveToLLVMRegisterInstruction( index, function_context );

	result.llvm_value=
		function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef< llvm::Value*> ( index_list, 2u ) );

	return result;
}

Variable CodeBuilder::BuildMemberAccessOperator(
	const Variable& variable,
	const MemberAccessOperator& member_access_operator,
	FunctionContext& function_context )
{
	const ClassPtr* const class_type= boost::get<ClassPtr>( &variable.type.one_of_type_kind );
	if( class_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( member_access_operator.file_pos_, variable.type.ToString() ) );
		throw ProgramError();
	}
	U_ASSERT( *class_type != nullptr );

	const Class::Field* field= (*class_type)->GetField( member_access_operator.member_name_ );
	if( field == nullptr )
	{
		errors_.push_back( ReportNameNotFound( member_access_operator.file_pos_, member_access_operator.member_name_ ) );
		throw ProgramError();
	}

	U_ASSERT( variable.location == Variable::Location::Pointer );

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
	index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );

	Variable result;
	result.location= Variable::Location::Pointer;
	result.value_type= ValueType::Reference;
	result.type= field->type;
	result.llvm_value=
		function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, index_list );
	return result;
}

Variable CodeBuilder::BuildCallOperator(
	const Variable& function_variable,
	const CallOperator& call_operator,
	const NamesScope& names,
	FunctionContext& function_context )
{
	const NontypeStub* nontype_stub= boost::get<NontypeStub>( &function_variable.type.one_of_type_kind );
	if( nontype_stub == nullptr || *nontype_stub != NontypeStub::OverloadedFunctionsSet )
	{
		// TODO - Call of non-function.
		throw ProgramError();
	}
	const OverloadedFunctionsSet& functions_set= function_variable.functions_set;

	std::vector<Function::Arg> actual_args( call_operator.arguments_.size() );
	std::vector<Variable> actual_args_variables( call_operator.arguments_.size() );
	for( unsigned int i= 0u; i < actual_args.size(); i++ )
	{

		Variable expr= BuildExpressionCode( *call_operator.arguments_[i], names, function_context );
		actual_args[i].type= expr.type;
		actual_args[i].is_reference= expr.value_type != ValueType::Value;
		actual_args[i].is_mutable= expr.value_type == ValueType::Reference;

		actual_args_variables[i]= std::move(expr);
	}

	const Variable function= GetOverloadedFunction( functions_set, actual_args, call_operator.file_pos_ );
	const Function& function_type= *boost::get<FunctionPtr>( function.type.one_of_type_kind );

	if( function_type.args.size() != actual_args.size( ))
	{
		errors_.push_back( ReportFunctionSignatureMismatch( call_operator.file_pos_ ) );
		throw ProgramError();
	}

	std::vector<llvm::Value*> llvm_args( actual_args.size() );
	for( unsigned int i= 0u; i < actual_args.size(); i++ )
	{
		const Function::Arg& arg= function_type.args[i];
		const Variable& expr= actual_args_variables[i];

		if( expr.type != arg.type )
		{
			errors_.push_back( ReportFunctionSignatureMismatch( call_operator.arguments_[i]->file_pos_ ) );
			throw ProgramError();
		}

		if( arg.is_reference )
		{
			if( arg.is_mutable )
			{
				if( expr.value_type == ValueType::Value )
				{
					errors_.push_back( ReportExpectedReferenceValue( call_operator.arguments_[i]->file_pos_ ) );
					throw ProgramError();
				}
				if( expr.value_type == ValueType::ConstReference )
				{
					errors_.push_back( ReportBindingConstReferenceToNonconstReference( call_operator.arguments_[i]->file_pos_ ) );
					throw ProgramError();
				}

				llvm_args[i]= expr.llvm_value;
			}
			else
			{
				if( expr.value_type == ValueType::Value )
				{
					// Bind value to const reference.
					// TODO - support nonfundamental values.
					llvm::Value* temp_storage= function_context.llvm_ir_builder.CreateAlloca( expr.type.GetLLVMType() );
					function_context.llvm_ir_builder.CreateStore( expr.llvm_value, temp_storage );
					llvm_args[i]= temp_storage;
				}
				else
				{
					llvm_args[i]= expr.llvm_value;
				}
			}
		}
		else
		{
			// TODO - support nonfundamental value-parameters.
			llvm_args[i]= CreateMoveToLLVMRegisterInstruction( expr, function_context );
		}
	}

	llvm::Value* call_result=
		function_context.llvm_ir_builder.CreateCall(
			llvm::dyn_cast<llvm::Function>(function.llvm_value),
			llvm_args );

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
		result.location= Variable::Location::LLVMRegister;
		result.value_type= ValueType::Value;
	}
	result.type= function_type.return_type;
	result.llvm_value= call_result;

	return result;
}

Variable CodeBuilder::BuildUnaryMinus(
	const Variable& variable,
	const UnaryMinus& unary_minus,
	FunctionContext& function_context )
{
	const FundamentalType* const fundamental_type= boost::get<FundamentalType>( &variable.type.one_of_type_kind );
	if( fundamental_type == nullptr )
	{
		errors_.push_back( ReportOperationNotSupportedForThisType( unary_minus.file_pos_, variable.type.ToString() ) );
		throw ProgramError();
	}
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
