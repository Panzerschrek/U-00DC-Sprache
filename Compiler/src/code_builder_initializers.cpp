#include <set>

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

llvm::Constant* CodeBuilder::ApplyInitializer(
	const Variable& variable,
	const Synt::IInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const auto array_initializer=
		dynamic_cast<const Synt::ArrayInitializer*>(&initializer) )
	{
		return ApplyArrayInitializer( variable, *array_initializer, block_names, function_context );
	}
	else if( const auto struct_named_initializer=
		dynamic_cast<const Synt::StructNamedInitializer*>(&initializer) )
	{
		ApplyStructNamedInitializer( variable, *struct_named_initializer, block_names, function_context );
	}
	else if( const auto constructor_initializer=
		dynamic_cast<const Synt::ConstructorInitializer*>(&initializer) )
	{
		return ApplyConstructorInitializer( variable, constructor_initializer->call_operator, block_names, function_context );
	}
	else if( const auto expression_initializer=
		dynamic_cast<const Synt::ExpressionInitializer*>(&initializer) )
	{
		return ApplyExpressionInitializer( variable, *expression_initializer, block_names, function_context );
	}
	else if( const auto zero_initializer=
		dynamic_cast<const Synt::ZeroInitializer*>(&initializer) )
	{
		return ApplyZeroInitializer( variable, *zero_initializer, block_names, function_context );
	}
	else
		U_ASSERT(false);

	return nullptr;
}

void CodeBuilder::ApplyEmptyInitializer(
	const ProgramString& variable_name,
	const FilePos& file_pos,
	const Variable& variable,
	FunctionContext& function_context )
{
	if( variable.type.GetTemplateDependentType() != nullptr )
		return;

	if( !variable.type.IsDefaultConstructible() )
	{
		errors_.push_back( ReportExpectedInitializer( file_pos, variable_name ) );
		return;
	}

	if( variable.type.GetFundamentalType() != nullptr )
	{
		// Fundamentals is not default-constructible, we should generate error about it before.
		U_ASSERT( false );
	}
	else if( const Array* const array_type= variable.type.GetArrayType() )
	{
		Variable array_member= variable;
		array_member.type= array_type->type;
		array_member.location= Variable::Location::Pointer;

		GenerateLoop(
			array_type->ArraySizeOrZero(),
			[&](llvm::Value* const counter_value)
			{
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= counter_value;
				array_member.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

				ApplyEmptyInitializer( variable_name, file_pos, array_member, function_context );
			},
			function_context);
	}
	else if( const Class* const class_type= variable.type.GetClassType() )
	{
		// If initializer for class variable is empty, try to call default constructor.

		const NamesScope::InsertedName* constructor_name=
			class_type->members.GetThisScopeName( Keyword( Keywords::constructor_ ) );
		U_ASSERT( constructor_name != nullptr );
		const OverloadedFunctionsSet* const constructors_set= constructor_name->second.GetFunctionsSet();
		U_ASSERT( constructors_set != nullptr );

		ThisOverloadedMethodsSet this_overloaded_methods_set;
		this_overloaded_methods_set.this_= variable;
		this_overloaded_methods_set.overloaded_methods_set= *constructors_set;

		const Synt::CallOperator call_operator( file_pos, std::vector<Synt::IExpressionComponentPtr>() );
		NamesScope dummy_names_scope( ProgramString(), nullptr );
		BuildCallOperator( this_overloaded_methods_set, call_operator, dummy_names_scope, function_context );
	}
	else
		U_ASSERT(false);
}

llvm::Constant* CodeBuilder::ApplyArrayInitializer(
	const Variable& variable,
	const Synt::ArrayInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( variable.type.GetTemplateDependentType() != nullptr )
	{
		for( const Synt::IInitializerPtr& sub_initializer : initializer.initializers )
			ApplyInitializer( variable, *sub_initializer, block_names, function_context );
		return nullptr;
	}

	const Array* const array_type= variable.type.GetArrayType();
	if( array_type == nullptr )
	{
		errors_.push_back( ReportArrayInitializerForNonArray( initializer.file_pos_ ) );
		return nullptr;
	}

	if( array_type->size != Array::c_undefined_size && initializer.initializers.size() != array_type->size )
	{
		errors_.push_back(
			ReportArrayInitializersCountMismatch(
				initializer.file_pos_,
				array_type->size,
				initializer.initializers.size() ) );
		return nullptr;
		// SPRACHE_TODO - add array continious initializers.
	}

	Variable array_member= variable;
	array_member.type= array_type->type;
	array_member.location= Variable::Location::Pointer;

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

	bool is_constant= array_type->type.CanBeConstexpr();
	std::vector<llvm::Constant*> members_constants;

	for( SizeType i= 0u; i < initializer.initializers.size(); i++ )
	{
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(i) ) );
		array_member.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

		U_ASSERT( initializer.initializers[i] != nullptr );
		llvm::Constant* const member_constant=
			ApplyInitializer( array_member, *initializer.initializers[i], block_names, function_context );

		if( is_constant && member_constant != nullptr )
			members_constants.push_back( member_constant );
		else
			is_constant= false;
	}

	U_ASSERT( members_constants.size() == initializer.initializers.size() || !is_constant );

	if( is_constant )
	{
		if( array_type->size == Array::c_undefined_size )
			return llvm::UndefValue::get( array_type->llvm_type );
		else
			return llvm::ConstantArray::get( array_type->llvm_type, members_constants );
	}

	return nullptr;
}

void CodeBuilder::ApplyStructNamedInitializer(
	const Variable& variable,
	const Synt::StructNamedInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( variable.type.GetTemplateDependentType() != nullptr )
	{
		for( const Synt::StructNamedInitializer::MemberInitializer& member_initializer : initializer.members_initializers )
			ApplyInitializer( variable, *member_initializer.initializer, block_names, function_context );
		return;
	}

	const Class* const class_type= variable.type.GetClassType();
	if( class_type == nullptr )
	{
		errors_.push_back( ReportStructInitializerForNonStruct( initializer.file_pos_ ) );
		return;
	}

	if( class_type->have_explicit_noncopy_constructors )
		errors_.push_back( ReportInitializerDisabledBecauseClassHaveExplicitNoncopyConstructors( initializer.file_pos_ ) );

	std::set<ProgramString> initialized_members_names;

	Variable struct_member= variable;
	struct_member.location= Variable::Location::Pointer;
	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

	for( const Synt::StructNamedInitializer::MemberInitializer& member_initializer : initializer.members_initializers )
	{
		if( initialized_members_names.count( member_initializer.name ) != 0 )
		{
			errors_.push_back( ReportDuplicatedStructMemberInitializer( initializer.file_pos_, member_initializer.name ) );
			continue;
		}

		const NamesScope::InsertedName* const class_member= class_type->members.GetThisScopeName( member_initializer.name );
		if( class_member == nullptr )
		{
			errors_.push_back( ReportNameNotFound( initializer.file_pos_, member_initializer.name ) );
			continue;
		}
		const ClassField* const field= class_member->second.GetClassField();
		if( field == nullptr )
		{
			errors_.push_back( ReportInitializerForNonfieldStructMember( initializer.file_pos_, member_initializer.name ) );
			continue;
		}

		initialized_members_names.insert( member_initializer.name );

		if( field->is_reference )
		{
			const Synt::IExpressionComponent* initializer_expression= nullptr;
			if( const auto expression_initializer=
				dynamic_cast<const Synt::ExpressionInitializer*>( member_initializer.initializer.get() ) )
			{
				initializer_expression= expression_initializer->expression.get();
			}
			else if( const auto constructor_initializer=
				dynamic_cast<const Synt::ConstructorInitializer*>( member_initializer.initializer.get() ) )
			{
				if( constructor_initializer->call_operator.arguments_.size() != 1u )
				{
					errors_.push_back( ReportReferencesHaveConstructorsWithExactlyOneParameter( constructor_initializer->file_pos_ ) );
					continue;
				}
				initializer_expression= constructor_initializer->call_operator.arguments_.front().get();
			}
			else
			{
				errors_.push_back( ReportUnsupportedInitializerForReference( member_initializer.initializer->GetFilePos() ) );
				continue;
			}

			// SPRACHE_TODO - maybe we need save temporaries of this expression?
			const Value initializer_value= BuildExpressionCodeAndDestroyTemporaries( *initializer_expression, block_names, function_context );
			if( initializer_value.GetTemplateDependentValue() != nullptr )
				continue;

			const Variable* const initializer_variable= initializer_value.GetVariable();
			if( initializer_variable == nullptr )
			{
				errors_.push_back( ReportExpectedVariable( initializer_expression->GetFilePos(), initializer_value.GetType().ToString() ) );
				continue;
			}

			if( field->type.GetTemplateDependentType() != nullptr )
				continue;
			if( initializer_variable->value_type == ValueType::Value )
			{
				errors_.push_back( ReportExpectedReferenceValue( initializer_expression->GetFilePos() ) );
				continue;
			}
			U_ASSERT( initializer_variable->location == Variable::Location::Pointer );

			// TODO - check mutability correctness
			// TODO - collect referenced variables

			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			llvm::Value* const address_of_reference=
				function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			function_context.llvm_ir_builder.CreateStore( initializer_variable->llvm_value, address_of_reference );
		}
		else
		{
			struct_member.type= field->type;
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			struct_member.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			U_ASSERT( member_initializer.initializer != nullptr );
			ApplyInitializer( struct_member, *member_initializer.initializer, block_names, function_context );
		}
	}

	U_ASSERT( initialized_members_names.size() <= class_type->field_count );
	class_type->members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& class_member )
		{
			if( const ClassField* const field = class_member.second.GetClassField() )
			{
				if( initialized_members_names.count( class_member.first ) == 0 )
				{
					if( field->is_reference )
						errors_.push_back( ReportExpectedInitializer( class_member.second.GetFilePos(), class_member.first ) ); // References is not default-constructible.
					else
					{
						struct_member.type= field->type;
						index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
						struct_member.llvm_value=
							function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );
						ApplyEmptyInitializer( class_member.first, initializer.file_pos_, struct_member, function_context );
					}
				}
			}
		});
}

llvm::Constant* CodeBuilder::ApplyConstructorInitializer(
	const Variable& variable,
	const Synt::CallOperator& call_operator,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( variable.type.GetTemplateDependentType() != nullptr )
	{
		for( const Synt::IExpressionComponentPtr& arg : call_operator.arguments_ )
			BuildExpressionCode( *arg, block_names, function_context );
		return nullptr;
	}

	if( const FundamentalType* const dst_type= variable.type.GetFundamentalType() )
	{
		if( call_operator.arguments_.size() != 1u )
		{
			errors_.push_back( ReportFundamentalTypesHaveConstructorsWithExactlyOneParameter( call_operator.file_pos_ ) );
			return nullptr;
		}

		// SPRACHE_TODO - maybe we need save temporaries of this expression?
		const Value expression_result=
			BuildExpressionCodeAndDestroyTemporaries( *call_operator.arguments_.front(), block_names, function_context );
		if( expression_result.GetType() == NontypeStub::TemplateDependentValue ||
			expression_result.GetType().GetTemplateDependentType() != nullptr )
			return llvm::UndefValue::get( dst_type->llvm_type );

		const Type expression_type= expression_result.GetType();
		const FundamentalType* src_type= expression_type.GetFundamentalType();
		if( src_type == nullptr )
		{
			// Allow explicit conversions of enums to ints.
			if( const Enum* const enum_type= expression_type.GetEnumType () )
				src_type= &enum_type->underlaying_type;
		}

		if( src_type == nullptr )
		{
			errors_.push_back( ReportTypesMismatch( call_operator.file_pos_, variable.type.ToString(), expression_result.GetType().ToString() ) );
			return nullptr;
		}

		const Variable& src_var= *expression_result.GetVariable();
		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( src_var, function_context );

		llvm::Constant* constant_value= nullptr;
		const bool src_is_constant= src_var.constexpr_value != nullptr;

		if( dst_type->fundamental_type != src_type->fundamental_type )
		{
			// Perform fundamental types conversion.

			const SizeType src_size= expression_type.SizeOf();
			const SizeType dst_size= variable.type.SizeOf();
			if( IsInteger( dst_type->fundamental_type ) &&
				IsInteger( src_type->fundamental_type ) )
			{
				// int to int
				if( src_size < dst_size )
				{
					if( IsUnsignedInteger( dst_type->fundamental_type ) )
					{
						// We lost here some values in conversions, such i16 => u32, if src_type is signed.
						if( src_is_constant )
							constant_value= llvm::ConstantExpr::getZExt( src_var.constexpr_value, dst_type->llvm_type );
						else
							value_for_assignment= function_context.llvm_ir_builder.CreateZExt( value_for_assignment, dst_type->llvm_type );
					}
					else
					{
						if( src_is_constant )
							constant_value= llvm::ConstantExpr::getSExt( src_var.constexpr_value, dst_type->llvm_type );
						else
							value_for_assignment= function_context.llvm_ir_builder.CreateSExt( value_for_assignment, dst_type->llvm_type );
					}
				}
				else if( src_size > dst_size )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getTrunc( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateTrunc( value_for_assignment, dst_type->llvm_type );
				}
				else
				{
					if( src_is_constant )
						constant_value= src_var.constexpr_value;
					// Same size integers - do nothing.
				}
			}
			else if( IsFloatingPoint( dst_type->fundamental_type ) &&
				IsFloatingPoint( src_type->fundamental_type ) )
			{
				// float to float
				if( src_size < dst_size )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getFPExtend( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateFPExt( value_for_assignment, dst_type->llvm_type );
				}
				else if( src_size > dst_size )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getFPTrunc( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateFPTrunc( value_for_assignment, dst_type->llvm_type );
				}
				else U_ASSERT(false);
			}
			else if( IsFloatingPoint( dst_type->fundamental_type ) &&
				IsInteger( src_type->fundamental_type ) )
			{
				// int to float
				if( IsSignedInteger( src_type->fundamental_type ) )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getSIToFP( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateSIToFP( value_for_assignment, dst_type->llvm_type );
				}
				else
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getUIToFP( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateUIToFP( value_for_assignment, dst_type->llvm_type );
				}
			}
			else if( IsInteger( dst_type->fundamental_type ) &&
				IsFloatingPoint( src_type->fundamental_type ) )
			{
				// float to int
				if( IsSignedInteger( dst_type->fundamental_type ) )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getFPToSI( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateFPToSI( value_for_assignment, dst_type->llvm_type );
				}
				else
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getFPToUI( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateFPToUI( value_for_assignment, dst_type->llvm_type );
				}
			}
			else
			{
				if( dst_type->fundamental_type == U_FundamentalType::Bool )
				{
					// TODO - error, bool have no constructors from other types
				}
				errors_.push_back( ReportTypesMismatch( call_operator.file_pos_, variable.type.ToString(), expression_result.GetType().ToString() ) );
				return nullptr;
			}
		} // If needs conversion
		else
		{
			if( src_is_constant )
				constant_value= src_var.constexpr_value;
		}

		if( src_is_constant )
		{
			U_ASSERT( constant_value != nullptr );
			value_for_assignment= constant_value;
		}

		function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );
		return constant_value;
	}
	else if( variable.type.GetEnumType() != nullptr )
	{
		if( call_operator.arguments_.size() != 1u )
		{
			// TODO - generate separate error for enums.
			errors_.push_back( ReportFundamentalTypesHaveConstructorsWithExactlyOneParameter( call_operator.file_pos_ ) );
			return nullptr;
		}

		// SPRACHE_TODO - maybe we need save temporaries of this expression?
		const Value expression_result=
			BuildExpressionCodeAndDestroyTemporaries( *call_operator.arguments_.front(), block_names, function_context );
		if( expression_result.GetType() == NontypeStub::TemplateDependentValue ||
			expression_result.GetType().GetTemplateDependentType() != nullptr )
			return llvm::UndefValue::get( dst_type->llvm_type );

		if( expression_result.GetType() != variable.type )
		{
			errors_.push_back( ReportTypesMismatch( call_operator.file_pos_, variable.type.ToString(), expression_result.GetType().ToString() ) );
			return nullptr;
		}

		const Variable& expression_result_variable= *expression_result.GetVariable();

		function_context.llvm_ir_builder.CreateStore(
			CreateMoveToLLVMRegisterInstruction( expression_result_variable, function_context ),
			variable.llvm_value );

		return expression_result_variable.constexpr_value;
	}
	else if( const Class* const class_type= variable.type.GetClassType() )
	{
		const NamesScope::InsertedName* constructor_name=
			class_type->members.GetThisScopeName( Keyword( Keywords::constructor_ ) );

		if( constructor_name == nullptr )
		{
			errors_.push_back( ReportClassHaveNoConstructors( call_operator.file_pos_ ) );
			return nullptr;
		}

		const OverloadedFunctionsSet* const constructors_set= constructor_name->second.GetFunctionsSet();
		U_ASSERT( constructors_set != nullptr );

		ThisOverloadedMethodsSet this_overloaded_methods_set;
		this_overloaded_methods_set.this_= variable;
		this_overloaded_methods_set.overloaded_methods_set= *constructors_set;

		// TODO - disallow explicit constructors calls.
		BuildCallOperator( this_overloaded_methods_set, call_operator, block_names, function_context );
	}
	else
	{
		errors_.push_back( ReportConstructorInitializerForUnsupportedType( call_operator.file_pos_ ) );
		return nullptr;
	}

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyExpressionInitializer(
	const Variable& variable,
	const Synt::ExpressionInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( variable.type.GetTemplateDependentType() != nullptr )
	{
		BuildExpressionCode( *initializer.expression, block_names, function_context );
		return nullptr;
	}

	if( variable.type.GetFundamentalType() != nullptr ||
		variable.type.GetEnumType() != nullptr )
	{
		// SPRACHE_TODO - maybe we need save temporaries of this expression?
		const Value expression_result=
			BuildExpressionCodeAndDestroyTemporaries( *initializer.expression, block_names, function_context );
		if( expression_result.GetType() == NontypeStub::TemplateDependentValue ||
			expression_result.GetType().GetTemplateDependentType() != nullptr )
			return llvm::UndefValue::get( variable.type.GetLLVMType() );

		if( expression_result.GetType() != variable.type )
		{
			errors_.push_back( ReportTypesMismatch( initializer.file_pos_, variable.type.ToString(), expression_result.GetType().ToString() ) );
			return nullptr;
		}

		llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( *expression_result.GetVariable(), function_context );
		function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );

		if( llvm::Constant* const constexpr_value= expression_result.GetVariable()->constexpr_value )
			return constexpr_value;
	}
	else if( variable.type.GetTemplateDependentType() != nullptr )
	{}
	else
	{
		errors_.push_back( ReportNotImplemented( initializer.file_pos_, "expression initialization for nonfundamental types" ) );
		return nullptr;
	}

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyZeroInitializer(
	const Variable& variable,
	const Synt::ZeroInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( variable.type.GetTemplateDependentType() != nullptr )
		return nullptr;

	if( const FundamentalType* const fundamental_type= variable.type.GetFundamentalType() )
	{
		llvm::Constant* zero_value= nullptr;
		switch( fundamental_type->fundamental_type )
		{
		case U_FundamentalType::Bool:
		case U_FundamentalType::i8:
		case U_FundamentalType::u8:
		case U_FundamentalType::i16:
		case U_FundamentalType::u16:
		case U_FundamentalType::i32:
		case U_FundamentalType::u32:
		case U_FundamentalType::i64:
		case U_FundamentalType::u64:
		case U_FundamentalType::InvalidType:
			zero_value=
				llvm::Constant::getIntegerValue(
					fundamental_type->llvm_type,
					llvm::APInt( fundamental_type->llvm_type->getIntegerBitWidth(), uint64_t(0) ) );
			break;

		case U_FundamentalType::f32:
		case U_FundamentalType::f64:
			zero_value= llvm::ConstantFP::get( fundamental_type->llvm_type, 0.0 );
			break;

		case U_FundamentalType::Void:
		case U_FundamentalType::LastType:
			U_ASSERT(false);
			break;
		};

		function_context.llvm_ir_builder.CreateStore( zero_value, variable.llvm_value );
		return zero_value;
	}
	else if( const Enum* const enum_type= variable.type.GetEnumType() )
	{
		// Currently, first member of enum have zero value.

		llvm::Constant* const zero_value=
			llvm::Constant::getIntegerValue(
				enum_type->underlaying_type.llvm_type,
				llvm::APInt( enum_type->underlaying_type.llvm_type->getIntegerBitWidth(), uint64_t(0) ) );

		function_context.llvm_ir_builder.CreateStore( zero_value, variable.llvm_value );
		return zero_value;
	}
	else if( const Array* const array_type= variable.type.GetArrayType() )
	{
		Variable array_member= variable;
		array_member.type= array_type->type;
		array_member.location= Variable::Location::Pointer;

		llvm::Constant* const_value= nullptr;

		GenerateLoop(
			array_type->ArraySizeOrZero(),
			[&](llvm::Value* const counter_value)
			{
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= counter_value;
				array_member.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*>( index_list, 2u ) );
				const_value= ApplyZeroInitializer( array_member, initializer, block_names, function_context );
			},
			function_context);

		if( const_value != nullptr && array_type->type.CanBeConstexpr() )
		{
			if( array_type->size == Array::c_undefined_size )
				return llvm::UndefValue::get( array_type->llvm_type );
			else
				return
					llvm::ConstantArray::get(
						array_type->llvm_type,
						std::vector<llvm::Constant*>( array_type->size, const_value ) );
		}
	}
	else if( const Class* const class_type= variable.type.GetClassType() )
	{
		if( class_type->have_explicit_noncopy_constructors )
			errors_.push_back( ReportInitializerDisabledBecauseClassHaveExplicitNoncopyConstructors( initializer.file_pos_ ) );

		Variable struct_member= variable;
		struct_member.location= Variable::Location::Pointer;
		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

		class_type->members.ForEachInThisScope(
			[&]( const NamesScope::InsertedName& member )
			{
				const ClassField* const field= member.second.GetClassField();
				if( field == nullptr )
					return;

				struct_member.type= field->type;
				index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
				struct_member.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

				ApplyZeroInitializer( struct_member, initializer, block_names, function_context );
			});
	}
	else
		U_ASSERT( false );

	return nullptr;
}

} // namespace CodeBuilderPrivate

} // namespace U
