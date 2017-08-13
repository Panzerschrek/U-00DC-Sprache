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

static const size_t g_max_array_size_to_linear_initialization= 8u;

void CodeBuilder::ApplyInitializer(
	const Variable& variable,
	const IInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const ArrayInitializer* const array_initializer=
		dynamic_cast<const ArrayInitializer*>(&initializer) )
	{
		ApplyArrayInitializer( variable, *array_initializer, block_names, function_context );
	}
	else if( const StructNamedInitializer* const struct_named_initializer=
		dynamic_cast<const StructNamedInitializer*>(&initializer) )
	{
		ApplyStructNamedInitializer( variable, *struct_named_initializer, block_names, function_context );
	}
	else if( const ConstructorInitializer* const constructor_initializer=
		dynamic_cast<const ConstructorInitializer*>(&initializer) )
	{
		ApplyConstructorInitializer( variable, constructor_initializer->call_operator, block_names, function_context );
	}
	else if( const ExpressionInitializer* const expression_initializer=
		dynamic_cast<const ExpressionInitializer*>(&initializer) )
	{
		ApplyExpressionInitializer( variable, *expression_initializer, block_names, function_context );
	}
	else if( const ZeroInitializer* const zero_initializer=
		dynamic_cast<const ZeroInitializer*>(&initializer) )
	{
		ApplyZeroInitializer( variable, *zero_initializer, block_names, function_context );
	}
	else
	{
		U_ASSERT(false);
	}
}

void CodeBuilder::ApplyEmptyInitializer(
	const ProgramString& variable_name,
	const FilePos& file_pos,
	const Variable& variable,
	FunctionContext& function_context )
{
	if( !variable.type.IsDefaultConstructible() )
	{
		errors_.push_back( ReportExpectedInitializer( file_pos, variable_name ) );
		return;
	}

	if( const FundamentalType* const fundamental_type= variable.type.GetFundamentalType() )
	{
		// Fundamentals is not default-constructible, we should generate error about it before.
		U_UNUSED( fundamental_type );
		U_ASSERT( false );
	}
	else if( const Array* const array_type= variable.type.GetArrayType() )
	{
		Variable array_member= variable;
		array_member.type= array_type->type;
		array_member.location= Variable::Location::Pointer;

		GenerateLoop(
			array_type->size,
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
	else if( const ClassPtr class_type= variable.type.GetClassType() )
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

		const CallOperator call_operator( file_pos, std::vector<IExpressionComponentPtr>() );
		NamesScope dummy_names_scope( ProgramString(), nullptr );
		BuildCallOperator( this_overloaded_methods_set, call_operator, dummy_names_scope, function_context );
	}
	else
		return;
}

void CodeBuilder::ApplyArrayInitializer(
	const Variable& variable,
	const ArrayInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	const Array* const array_type= variable.type.GetArrayType();
	if( array_type == nullptr )
	{
		errors_.push_back( ReportArrayInitializerForNonArray( initializer.file_pos_ ) );
		return;
	}

	if( initializer.initializers.size() != array_type->size )
	{
		errors_.push_back(
			ReportArrayInitializersCountMismatch(
				initializer.file_pos_,
				array_type->size,
				initializer.initializers.size() ) );
		return;
		// SPRACHE_TODO - add array continious initializers.
	}

	Variable array_member= variable;
	array_member.type= array_type->type;
	array_member.location= Variable::Location::Pointer;

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

	for( size_t i= 0u; i < initializer.initializers.size(); i++ )
	{
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(i) ) );
		array_member.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

		U_ASSERT( initializer.initializers[i] != nullptr );
		ApplyInitializer( array_member, *initializer.initializers[i], block_names, function_context );
	}
}

void CodeBuilder::ApplyStructNamedInitializer(
	const Variable& variable,
	const StructNamedInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	const ClassPtr class_type= variable.type.GetClassType();
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

	for( const StructNamedInitializer::MemberInitializer& member_initializer : initializer.members_initializers )
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

		struct_member.type= field->type;
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
		struct_member.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

		U_ASSERT( member_initializer.initializer != nullptr );
		ApplyInitializer( struct_member, *member_initializer.initializer, block_names, function_context );
	}

	U_ASSERT( initialized_members_names.size() <= class_type->field_count );
	class_type->members.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& class_member )
		{
			if( const ClassField* const field = class_member.second.GetClassField() )
			{
				if( initialized_members_names.count( class_member.first ) == 0 )
				{
					struct_member.type= field->type;
					index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
					struct_member.llvm_value=
						function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );
					ApplyEmptyInitializer( class_member.first, initializer.file_pos_, struct_member, function_context );
				}
			}
		});
}

void CodeBuilder::ApplyConstructorInitializer(
	const Variable& variable,
	const CallOperator& call_operator,
	const NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const FundamentalType* const dst_type= variable.type.GetFundamentalType() )
	{
		if( call_operator.arguments_.size() != 1u )
		{
			errors_.push_back( ReportFundamentalTypesHaveConstructorsWithExactlyOneParameter( call_operator.file_pos_ ) );
			return;
		}

		const Value expression_result=
			BuildExpressionCode( *call_operator.arguments_.front(), block_names, function_context );
		const Type expression_type= expression_result.GetType();
		const FundamentalType* const src_type= expression_type.GetFundamentalType();

		if( src_type == nullptr )
		{
			errors_.push_back( ReportTypesMismatch( call_operator.file_pos_, variable.type.ToString(), expression_result.GetType().ToString() ) );
			return;
		}

		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( *expression_result.GetVariable(), function_context );

		if( dst_type->fundamental_type != src_type->fundamental_type )
		{
			// Perform fundamental types conversion.

			const size_t src_size= expression_type.SizeOf();
			const size_t dst_size= variable.type.SizeOf();
			if( IsInteger( dst_type->fundamental_type ) &&
				IsInteger( src_type->fundamental_type ) )
			{
				// int to int
				if( src_size < dst_size )
				{
					if( IsUnsignedInteger( dst_type->fundamental_type ) )
					{
						// We lost here some values in conversions, such i16 => u32, if src_type is signed.
						value_for_assignment= function_context.llvm_ir_builder.CreateZExt( value_for_assignment, dst_type->llvm_type );
					}
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateSExt( value_for_assignment, dst_type->llvm_type );
				}
				else if( src_size > dst_size )
					value_for_assignment= function_context.llvm_ir_builder.CreateTrunc( value_for_assignment, dst_type->llvm_type );
				else {} // Same size integers - do nothing.
			}
			else if( IsFloatingPoint( dst_type->fundamental_type ) &&
				IsFloatingPoint( src_type->fundamental_type ) )
			{
				// float to float
				if( src_size < dst_size )
					value_for_assignment= function_context.llvm_ir_builder.CreateFPExt( value_for_assignment, dst_type->llvm_type );
				else if( src_size > dst_size )
					value_for_assignment= function_context.llvm_ir_builder.CreateFPTrunc( value_for_assignment, dst_type->llvm_type );
				else{ U_ASSERT(false); }
			}
			else if( IsFloatingPoint( dst_type->fundamental_type ) &&
				IsInteger( src_type->fundamental_type ) )
			{
				// int to float
				if( IsSignedInteger( src_type->fundamental_type ) )
					value_for_assignment= function_context.llvm_ir_builder.CreateSIToFP( value_for_assignment, dst_type->llvm_type );
				else
					value_for_assignment= function_context.llvm_ir_builder.CreateUIToFP( value_for_assignment, dst_type->llvm_type );
			}
			else if( IsInteger( dst_type->fundamental_type ) &&
				IsFloatingPoint( src_type->fundamental_type ) )
			{
				// float to int
				if( IsSignedInteger( dst_type->fundamental_type ) )
					value_for_assignment= function_context.llvm_ir_builder.CreateFPToSI( value_for_assignment, dst_type->llvm_type );
				else
					value_for_assignment= function_context.llvm_ir_builder.CreateFPToUI( value_for_assignment, dst_type->llvm_type );
			}
			else
			{
				if( dst_type->fundamental_type == U_FundamentalType::Bool )
				{
					// TODO - error, bool have no constructors from other types
				}
				errors_.push_back( ReportTypesMismatch( call_operator.file_pos_, variable.type.ToString(), expression_result.GetType().ToString() ) );
				return;
			}
		} // If needs conversion

		function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );
	}
	else if( const ClassPtr class_type= variable.type.GetClassType() )
	{
		const NamesScope::InsertedName* constructor_name=
			class_type->members.GetThisScopeName( Keyword( Keywords::constructor_ ) );

		if( constructor_name == nullptr )
		{
			errors_.push_back( ReportClassHaveNoConstructors( call_operator.file_pos_ ) );
			return;
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
		return;
	}
}

void CodeBuilder::ApplyExpressionInitializer(
	const Variable& variable,
	const ExpressionInitializer& initializer,
	const NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const FundamentalType* const fundamental_type= variable.type.GetFundamentalType() )
	{
		U_UNUSED(fundamental_type);

		const Value expression_result=
			BuildExpressionCode( *initializer.expression, block_names, function_context );
		if( expression_result.GetType() != variable.type )
		{
			errors_.push_back( ReportTypesMismatch( initializer.file_pos_, variable.type.ToString(), expression_result.GetType().ToString() ) );
			return;
		}

		llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( *expression_result.GetVariable(), function_context );
		function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );
	}
	else
	{
		errors_.push_back( ReportNotImplemented( initializer.file_pos_, "expression initialization for nonfundamental types" ) );
		return;
	}
}

void CodeBuilder::ApplyZeroInitializer(
	const Variable& variable,
	const ZeroInitializer& initializer,
	const NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const FundamentalType* const fundamental_type= variable.type.GetFundamentalType() )
	{
		llvm::Value* zero_value= nullptr;
		switch( fundamental_type->fundamental_type )
		{
		case U_FundamentalType::Bool:
			zero_value=
				llvm::Constant::getIntegerValue(
					fundamental_llvm_types_.bool_,
					llvm::APInt( 1u, uint64_t(0) ) );
			break;

		case U_FundamentalType::i8:
		case U_FundamentalType::u8:
		case U_FundamentalType::i16:
		case U_FundamentalType::u16:
		case U_FundamentalType::i32:
		case U_FundamentalType::u32:
		case U_FundamentalType::i64:
		case U_FundamentalType::u64:
			zero_value=
				llvm::Constant::getIntegerValue(
					GetFundamentalLLVMType( fundamental_type->fundamental_type ),
					llvm::APInt( variable.type.SizeOf() * 8u, uint64_t(0) ) );
			break;

		case U_FundamentalType::f32:
			zero_value= llvm::ConstantFP::get( fundamental_llvm_types_.f32, 0.0 );
			break;
		case U_FundamentalType::f64:
			zero_value= llvm::ConstantFP::get( fundamental_llvm_types_.f64, 0.0 );
			break;

		case U_FundamentalType::Void:
		case U_FundamentalType::InvalidType:
		case U_FundamentalType::LastType:
			U_ASSERT(false);
			break;
		};

		function_context.llvm_ir_builder.CreateStore( zero_value, variable.llvm_value );
	}
	else if( const Array* const array_type= variable.type.GetArrayType() )
	{
		Variable array_member= variable;
		array_member.type= array_type->type;
		array_member.location= Variable::Location::Pointer;

		GenerateLoop(
			array_type->size,
			[&](llvm::Value* const counter_value)
			{
				llvm::Value* index_list[2];
				index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
				index_list[1]= counter_value;
				array_member.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*>( index_list, 2u ) );
				ApplyZeroInitializer( array_member, initializer, block_names, function_context );
			},
			function_context);
	}
	else if( const ClassPtr class_type= variable.type.GetClassType() )
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
	{
		// REPORT unsupported type for zero initializer
		return;
	}
}

} // namespace CodeBuilderPrivate

} // namespace U
