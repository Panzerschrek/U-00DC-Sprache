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
	const StoredVariablePtr& variable_storage,
	const Synt::IInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const auto array_initializer= dynamic_cast<const Synt::ArrayInitializer*>(&initializer) )
		return ApplyArrayInitializer( variable, variable_storage, *array_initializer, block_names, function_context );
	else if( const auto struct_named_initializer= dynamic_cast<const Synt::StructNamedInitializer*>(&initializer) )
		return ApplyStructNamedInitializer( variable, variable_storage, *struct_named_initializer, block_names, function_context );
	else if( const auto constructor_initializer= dynamic_cast<const Synt::ConstructorInitializer*>(&initializer) )
		return ApplyConstructorInitializer( variable, variable_storage, constructor_initializer->call_operator, block_names, function_context );
	else if( const auto expression_initializer= dynamic_cast<const Synt::ExpressionInitializer*>(&initializer) )
		return ApplyExpressionInitializer( variable, variable_storage, *expression_initializer, block_names, function_context );
	else if( const auto zero_initializer= dynamic_cast<const Synt::ZeroInitializer*>(&initializer) )
		return ApplyZeroInitializer( variable, *zero_initializer, function_context );
	else if( const auto uninitialized_initializer= dynamic_cast<const Synt::UninitializedInitializer*>(&initializer) )
		return ApplyUninitializedInitializer( variable, *uninitialized_initializer, function_context );
	else U_ASSERT(false);

	return nullptr;
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

	if( variable.type.GetFundamentalType() != nullptr || variable.type.GetEnumType() != nullptr )
	{
		// Fundamentals and enums are not default-constructible, we should generate error about it before.
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

		// TODO - fix this.
		// "CallOperator" pointer used as key in overloading resolution cache. Passing stack ovject is not safe.
		const Synt::CallOperator call_operator( file_pos, std::vector<Synt::IExpressionComponentPtr>() );
		NamesScope dummy_names_scope( ProgramString(), nullptr );
		BuildCallOperator( this_overloaded_methods_set, call_operator, dummy_names_scope, function_context );
	}
	else U_ASSERT(false);
}

llvm::Constant* CodeBuilder::ApplyArrayInitializer(
	const Variable& variable,
	const StoredVariablePtr& variable_storage,
	const Synt::ArrayInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
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

	for( size_t i= 0u; i < initializer.initializers.size(); i++ )
	{
		index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(i) ) );
		array_member.llvm_value=
			function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

		U_ASSERT( initializer.initializers[i] != nullptr );
		llvm::Constant* const member_constant=
			ApplyInitializer( array_member, variable_storage, *initializer.initializers[i], block_names, function_context );

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

llvm::Constant* CodeBuilder::ApplyStructNamedInitializer(
	const Variable& variable,
	const StoredVariablePtr& variable_storage,
	const Synt::StructNamedInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	const Class* const class_type= variable.type.GetClassType();
	if( class_type == nullptr || class_type->kind != Class::Kind::Struct )
	{
		errors_.push_back( ReportStructInitializerForNonStruct( initializer.file_pos_ ) );
		return nullptr;
	}

	if( class_type->have_explicit_noncopy_constructors )
		errors_.push_back( ReportInitializerDisabledBecauseClassHaveExplicitNoncopyConstructors( initializer.file_pos_ ) );

	std::set<ProgramString> initialized_members_names;

	std::vector<llvm::Constant*> constant_initializers;
	bool all_fields_are_constant= false;
	if( class_type->can_be_constexpr )
	{
		constant_initializers.resize( class_type->llvm_type->getNumElements(), nullptr );
		all_fields_are_constant= true;
	}

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
		if( field->class_.lock() != variable.type )
		{
			errors_.push_back( ReportInitializerForBaseClassField( initializer.file_pos_, member_initializer.name ) );
			continue;
		}

		initialized_members_names.insert( member_initializer.name );

		llvm::Constant* constant_initializer= nullptr;
		if( field->is_reference )
			constant_initializer=
				InitializeReferenceField( variable, variable_storage, *field, *member_initializer.initializer, block_names, function_context );
		else
		{
			struct_member.type= field->type;
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
			struct_member.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

			U_ASSERT( member_initializer.initializer != nullptr );
			constant_initializer=
				ApplyInitializer( struct_member, variable_storage, *member_initializer.initializer, block_names, function_context );
		}

		if( constant_initializer == nullptr )
			all_fields_are_constant= false;
		if( all_fields_are_constant )
			constant_initializers[field->index]= constant_initializer;
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

	if( all_fields_are_constant && initialized_members_names.size() == class_type->field_count )
		return llvm::ConstantStruct::get( class_type->llvm_type, constant_initializers );

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyConstructorInitializer(
	const Variable& variable,
	const StoredVariablePtr& variable_storage,
	const Synt::CallOperator& call_operator,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const FundamentalType* const dst_type= variable.type.GetFundamentalType() )
	{
		if( call_operator.arguments_.size() != 1u )
		{
			errors_.push_back( ReportFundamentalTypesHaveConstructorsWithExactlyOneParameter( call_operator.file_pos_ ) );
			return nullptr;
		}

		const Variable src_var= BuildExpressionCodeEnsureVariable( *call_operator.arguments_.front(), block_names, function_context );

		const FundamentalType* src_type= src_var.type.GetFundamentalType();
		if( src_type == nullptr )
		{
			// Allow explicit conversions of enums to ints.
			if( const Enum* const enum_type= src_var.type.GetEnumType () )
				src_type= &enum_type->underlaying_type;
		}

		if( src_type == nullptr )
		{
			errors_.push_back( ReportTypesMismatch( call_operator.file_pos_, variable.type.ToString(), src_var.type.ToString() ) );
			return nullptr;
		}

		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( src_var, function_context );

		llvm::Constant* constant_value= nullptr;
		const bool src_is_constant= src_var.constexpr_value != nullptr;

		if( dst_type->fundamental_type != src_type->fundamental_type )
		{
			// Perform fundamental types conversion.

			const SizeType src_size= src_var.type.SizeOf();
			const SizeType dst_size= variable.type.SizeOf();
			if( IsInteger( dst_type->fundamental_type ) && IsInteger( src_type->fundamental_type ) )
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
			else if( IsFloatingPoint( dst_type->fundamental_type ) && IsFloatingPoint( src_type->fundamental_type ) )
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
			else if( IsFloatingPoint( dst_type->fundamental_type ) && IsInteger( src_type->fundamental_type ) )
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
			else if( IsInteger( dst_type->fundamental_type ) && IsFloatingPoint( src_type->fundamental_type ) )
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
			else if( IsChar( dst_type->fundamental_type ) && ( IsInteger( src_type->fundamental_type ) || IsChar( src_type->fundamental_type ) ) )
			{
				// int to char or char to char
				if( src_size < dst_size )
				{
					if( src_is_constant )
						constant_value= llvm::ConstantExpr::getZExt( src_var.constexpr_value, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateZExt( value_for_assignment, dst_type->llvm_type );
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
				}
			}
			else if( IsInteger( dst_type->fundamental_type ) && IsChar( src_type->fundamental_type ) )
			{
				// char to int
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
				}
			}
			else
			{
				if( dst_type->fundamental_type == U_FundamentalType::Bool )
				{
					// TODO - error, bool have no constructors from other types
				}
				errors_.push_back( ReportTypesMismatch( call_operator.file_pos_, variable.type.ToString(), src_var.type.ToString() ) );
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

		const Variable expression_result= BuildExpressionCodeEnsureVariable( *call_operator.arguments_.front(), block_names, function_context );
		if( expression_result.type != variable.type )
		{
			errors_.push_back( ReportTypesMismatch( call_operator.file_pos_, variable.type.ToString(), expression_result.type.ToString() ) );
			return nullptr;
		}

		function_context.llvm_ir_builder.CreateStore(
			CreateMoveToLLVMRegisterInstruction( expression_result, function_context ),
			variable.llvm_value );

		return expression_result.constexpr_value;
	}
	else if( variable.type.GetFunctionPointerType() != nullptr )
	{
		if( call_operator.arguments_.size() != 1u )
		{
			// TODO - generate separate error for function pointers.
			errors_.push_back( ReportFundamentalTypesHaveConstructorsWithExactlyOneParameter( call_operator.file_pos_ ) );
			return nullptr;
		}

		return InitializeFunctionPointer( variable, *call_operator.arguments_.front(), block_names, function_context );
	}
	else if( const Class* const class_type= variable.type.GetClassType() )
	{
		// Try do move-construct.
		bool needs_move_constuct= false;
		if( call_operator.arguments_.size() == 1u )
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
			dummy_function_context.is_in_unsafe_block= function_context.is_in_unsafe_block;
			dummy_function_context.variables_state= function_context.variables_state;
			function_context.variables_state.DeactivateLocks();

			const Variable initializer_value= BuildExpressionCodeEnsureVariable( *call_operator.arguments_.front(), block_names, dummy_function_context );
			needs_move_constuct= initializer_value.type == variable.type && initializer_value.value_type == ValueType::Value ;

			function_context.variables_state.ActivateLocks();

			function_context.overloading_resolutin_cache.insert(
				dummy_function_context.overloading_resolutin_cache.begin(),
				dummy_function_context.overloading_resolutin_cache.end() );
		}
		if( needs_move_constuct )
		{
			const Variable initializer_variable= BuildExpressionCodeEnsureVariable( *call_operator.arguments_.front(), block_names, function_context );
			CopyBytes( initializer_variable.llvm_value, variable.llvm_value, variable.type, function_context );

			// Lock references and move.
			U_ASSERT( initializer_variable.referenced_variables.size() == 1u );
			for( const auto& inner_variable : function_context.variables_state.GetVariableReferences( *initializer_variable.referenced_variables.begin() ) )
			{
				const bool ok= function_context.variables_state.AddPollution( variable_storage, inner_variable.first, inner_variable.second.IsMutable() );
				if( !ok )
					errors_.push_back( ReportReferenceProtectionError( call_operator.file_pos_, inner_variable.first->name ) );
			}

			function_context.variables_state.Move( *initializer_variable.referenced_variables.begin() );

			return initializer_variable.constexpr_value; // Move can preserve constexpr.
		}

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
	const StoredVariablePtr& variable_storage,
	const Synt::ExpressionInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{

	if( variable.type.GetFundamentalType() != nullptr || variable.type.GetEnumType() != nullptr )
	{
		const Variable expression_result= BuildExpressionCodeEnsureVariable( *initializer.expression, block_names, function_context );
		if( expression_result.type != variable.type )
		{
			errors_.push_back( ReportTypesMismatch( initializer.file_pos_, variable.type.ToString(), expression_result.type.ToString() ) );
			return nullptr;
		}

		llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( expression_result, function_context );
		function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );

		if( llvm::Constant* const constexpr_value= expression_result.constexpr_value )
			return constexpr_value;
	}
	else if( variable.type.GetFunctionPointerType() != nullptr )
		return InitializeFunctionPointer( variable, *initializer.expression, block_names, function_context );
	else if( variable.type.GetClassType() != nullptr )
	{
		// Currently we support "=" initializer for copying and moving of structs.

		const Variable expression_result= BuildExpressionCodeEnsureVariable( *initializer.expression, block_names, function_context );
		if( !ReferenceIsConvertible( expression_result.type, variable.type, initializer.file_pos_ ) )
		{
			errors_.push_back( ReportTypesMismatch( initializer.file_pos_, variable.type.ToString(), expression_result.type.ToString() ) );
			return nullptr;
		}

		// Lock references.
		if( variable.type.ReferencesTagsCount() > 0u )
		{
			for( const StoredVariablePtr& referenced_variable : expression_result.referenced_variables )
			{
				for( const auto& inner_variable : function_context.variables_state.GetVariableReferences( referenced_variable ) )
				{
					const bool ok= function_context.variables_state.AddPollution( variable_storage, inner_variable.first, inner_variable.second.IsMutable() );
					if( !ok )
						errors_.push_back( ReportReferenceProtectionError( initializer.file_pos_, inner_variable.first->name ) );
				}
			}
		}

		// Move or try call copy constructor.
		// TODO - produce constant initializer for generated copy constructor, if source is constant.
		if( expression_result.value_type == ValueType::Value && expression_result.type == variable.type )
		{
			U_ASSERT( expression_result.referenced_variables.size() == 1u );
			function_context.variables_state.Move( *expression_result.referenced_variables.begin() );
			CopyBytes( expression_result.llvm_value, variable.llvm_value, variable.type, function_context );
			return expression_result.constexpr_value; // Move can preserve constexpr.
		}
		else
		{
			llvm::Value* value_for_copy= expression_result.llvm_value;
			if( expression_result.type != variable.type )
				value_for_copy= CreateReferenceCast( value_for_copy, expression_result.type, variable.type, function_context );
			TryCallCopyConstructor(
				initializer.file_pos_, variable.llvm_value, value_for_copy, variable.type.GetClassTypeProxy(), function_context );
		}
	}
	else
	{
		errors_.push_back( ReportNotImplemented( initializer.file_pos_, "expression initialization for arrays" ) );
		return nullptr;
	}

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyZeroInitializer(
	const Variable& variable,
	const Synt::ZeroInitializer& initializer,
	FunctionContext& function_context )
{
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
		case U_FundamentalType::char8 :
		case U_FundamentalType::char16:
		case U_FundamentalType::char32:
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
	else if( const FunctionPointer* const function_pointer_type= variable.type.GetFunctionPointerType() )
	{
		// Really? Allow zero function pointers?

		llvm::Constant* const null_value=
			llvm::Constant::getNullValue( function_pointer_type->llvm_function_pointer_type );

		function_context.llvm_ir_builder.CreateStore( null_value, variable.llvm_value );
		return null_value;
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
				const_value= ApplyZeroInitializer( array_member, initializer, function_context );
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
						std::vector<llvm::Constant*>( size_t(array_type->size), const_value ) );
		}
	}
	else if( const Class* const class_type= variable.type.GetClassType() )
	{
		if( class_type->have_explicit_noncopy_constructors )
			errors_.push_back( ReportInitializerDisabledBecauseClassHaveExplicitNoncopyConstructors( initializer.file_pos_ ) );
		if( class_type->kind != Class::Kind::Struct )
			errors_.push_back( ReportZeroInitializerForClass( initializer.file_pos_ ) );

		std::vector<llvm::Constant*> constant_initializers;
		bool all_fields_are_constant= false;
		if( class_type->can_be_constexpr )
		{
			constant_initializers.resize( class_type->llvm_type->getNumElements(), nullptr );
			all_fields_are_constant= true;
		}

		Variable struct_member= variable;
		struct_member.location= Variable::Location::Pointer;
		// Make first index = 0 for array to pointer conversion.
		llvm::Value* index_list[2];
		index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );

		class_type->members.ForEachInThisScope(
			[&]( const NamesScope::InsertedName& member )
			{
				const ClassField* const field= member.second.GetClassField();
				if( field == nullptr || field->class_.lock() != variable.type )
					return;
				if( field->is_reference )
				{
					all_fields_are_constant= false;
					errors_.push_back( ReportUnsupportedInitializerForReference( initializer.file_pos_ ) );
					return;
				}

				struct_member.type= field->type;
				index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field->index) ) );
				struct_member.llvm_value=
					function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

				llvm::Constant* const constant_initializer=
					ApplyZeroInitializer( struct_member, initializer, function_context );

				if( constant_initializer == nullptr )
					all_fields_are_constant= false;
				if( all_fields_are_constant )
					constant_initializers[field->index]= constant_initializer;
			});

		if( all_fields_are_constant )
			return llvm::ConstantStruct::get( class_type->llvm_type, constant_initializers );
	}
	else U_ASSERT( false );

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyUninitializedInitializer(
	const Variable& variable,
	const Synt::UninitializedInitializer& initializer,
	FunctionContext& function_context )
{
	if( !function_context.is_in_unsafe_block )
		errors_.push_back( ReportUninitializedInitializerOutsideUnsafeBlock( initializer.file_pos_ ) );

	if( variable.type.CanBeConstexpr() )
		return llvm::UndefValue::get( variable.type.GetLLVMType() );
	else
		return nullptr;
}

llvm::Constant* CodeBuilder::InitializeReferenceField(
	const Variable& variable,
	const StoredVariablePtr& variable_storage,
	const ClassField& field,
	const Synt::IInitializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	U_ASSERT( variable.type.GetClassType() != nullptr );
	U_ASSERT( variable.type.GetClassTypeProxy() == field.class_.lock() );

	const Synt::IExpressionComponent* initializer_expression= nullptr;
	if( const auto expression_initializer=
		dynamic_cast<const Synt::ExpressionInitializer*>( &initializer ) )
	{
		initializer_expression= expression_initializer->expression.get();
	}
	else if( const auto constructor_initializer=
		dynamic_cast<const Synt::ConstructorInitializer*>( &initializer ) )
	{
		if( constructor_initializer->call_operator.arguments_.size() != 1u )
		{
			errors_.push_back( ReportReferencesHaveConstructorsWithExactlyOneParameter( constructor_initializer->file_pos_ ) );
			return nullptr;
		}
		initializer_expression= constructor_initializer->call_operator.arguments_.front().get();
	}
	else
	{
		errors_.push_back( ReportUnsupportedInitializerForReference( initializer.GetFilePos() ) );
		return nullptr;
	}

	const Variable initializer_variable= BuildExpressionCodeEnsureVariable( *initializer_expression, block_names, function_context );

	if( !ReferenceIsConvertible( initializer_variable.type, field.type, initializer_expression->GetFilePos() ) )
	{
		errors_.push_back( ReportTypesMismatch( initializer_expression->GetFilePos(), field.type.ToString(), initializer_variable.type.ToString() ) );
		return nullptr;
	}
	if( initializer_variable.value_type == ValueType::Value )
	{
		errors_.push_back( ReportExpectedReferenceValue( initializer_expression->GetFilePos() ) );
		return nullptr;
	}
	U_ASSERT( initializer_variable.location == Variable::Location::Pointer );

	if( field.is_mutable && initializer_variable.value_type == ValueType::ConstReference )
	{
		errors_.push_back( ReportBindingConstReferenceToNonconstReference( initializer_expression->GetFilePos() ) );
		return nullptr;
	}

	for( const StoredVariablePtr& referenced_variable : initializer_variable.referenced_variables )
	{
		const bool ok= function_context.variables_state.AddPollution( variable_storage, referenced_variable, field.is_mutable );
		if( !ok )
			errors_.push_back( ReportReferenceProtectionError( initializer.GetFilePos(), referenced_variable->name ) );
	}
	CheckReferencedVariables( initializer_variable, initializer.GetFilePos() );

	// Make first index = 0 for array to pointer conversion.
	llvm::Value* index_list[2];
	index_list[0]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(0u) ) );
	index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, uint64_t(field.index) ) );
	llvm::Value* const address_of_reference=
		function_context.llvm_ir_builder.CreateGEP( variable.llvm_value, llvm::ArrayRef<llvm::Value*> ( index_list, 2u ) );

	llvm::Value* ref_to_store= initializer_variable.llvm_value;
	if( field.type != initializer_variable.type )
		ref_to_store= CreateReferenceCast( ref_to_store, initializer_variable.type, field.type, function_context );
	function_context.llvm_ir_builder.CreateStore( ref_to_store, address_of_reference );

	if( initializer_variable.constexpr_value != nullptr )
	{
		// We needs to store constant somewhere. Create global variable for it.
		llvm::Constant* constant_stored= CreateGlobalConstantVariable( initializer_variable.type, "_temp_const", initializer_variable.constexpr_value );

		if( field.type != initializer_variable.type )
			constant_stored=
				llvm::dyn_cast<llvm::Constant>( CreateReferenceCast( constant_stored, initializer_variable.type, field.type, function_context ) );

		return constant_stored;
	}

	return nullptr;
}

llvm::Constant* CodeBuilder::InitializeFunctionPointer(
	const Variable& variable,
	const Synt::IExpressionComponent& initializer_expression,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	U_ASSERT( variable.type.GetFunctionPointerType() != nullptr );

	const FunctionPointer& function_pointer_type= *variable.type.GetFunctionPointerType();

	const Value initializer_value= BuildExpressionCode( initializer_expression, block_names, function_context );

	if( const Variable* const initializer_variable= initializer_value.GetVariable() )
	{
		const FunctionPointer* const intitializer_type= initializer_variable->type.GetFunctionPointerType();
		if( intitializer_type == nullptr ||
			!intitializer_type->function.PointerCanBeConvertedTo( function_pointer_type.function ) )
		{
			errors_.push_back( ReportTypesMismatch( initializer_expression.GetFilePos(), variable.type.ToString(), initializer_variable->type.ToString() ) );
			return nullptr;
		}
		U_ASSERT( initializer_variable->type.GetFunctionPointerType() != nullptr );

		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( *initializer_variable, function_context );
		if( initializer_variable->type != variable.type )
			value_for_assignment= function_context.llvm_ir_builder.CreatePointerCast( value_for_assignment, variable.type.GetLLVMType() );

		function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );
		return initializer_variable->constexpr_value;
	}

	const OverloadedFunctionsSet* candidate_functions= nullptr;
	if( const OverloadedFunctionsSet* const overloaded_functions_set= initializer_value.GetFunctionsSet() )
		candidate_functions= overloaded_functions_set;
	else if( const ThisOverloadedMethodsSet* const overloaded_methods_set= initializer_value.GetThisOverloadedMethodsSet() )
		candidate_functions= &overloaded_methods_set->overloaded_methods_set;
	else
	{
		// TODO - generate separate error
		errors_.push_back( ReportExpectedVariable( initializer_expression.GetFilePos(), initializer_value.GetKindName() ) );
		return nullptr;
	}

	// Try select one of overloaded functions.
	// Select function with same with pointer type, if it exists.
	// If there is no function with same type, select function, convertible to pointer type, but only if exists only one convertible function.
	const FunctionVariable* exact_match_function_variable= nullptr;
	std::vector<const FunctionVariable*> convertible_function_variables;

	for( const FunctionVariable& func : candidate_functions->functions )
	{
		if( *func.type.GetFunctionType() == function_pointer_type.function )
			exact_match_function_variable= &func;
		else if( func.type.GetFunctionType()->PointerCanBeConvertedTo( function_pointer_type.function ) )
			convertible_function_variables.push_back(&func);
	}
	// Try also select template functions with zero template parameters and template functions with all template parameters known.
	for( const FunctionTemplatePtr& function_template : candidate_functions->template_functions )
	{
		if( function_template->template_parameters.empty() )
		{
			const FunctionVariable* const func=
				GenTemplateFunction(
					initializer_expression.GetFilePos(),
					function_template,
					std::vector<Function::Arg>(), false, true );
			if( func != nullptr )
			{
				if( func->type == function_pointer_type.function )
				{
					if( exact_match_function_variable != nullptr )
					{
						// Error, exist more, then one non-exact match function.
						// TODO - maybe generate separate error?
						errors_.push_back( ReportTooManySuitableOverloadedFunctions( initializer_expression.GetFilePos() ) );
						return nullptr;
					}
					exact_match_function_variable= func;
				}
				else if( func->type.GetFunctionType()->PointerCanBeConvertedTo( function_pointer_type.function ) )
					convertible_function_variables.push_back(func);
			}
		}
	}

	const FunctionVariable* function_variable= exact_match_function_variable;
	if( function_variable == nullptr )
	{
		if( convertible_function_variables.size() > 1u )
		{
			// Error, exist more, then one non-exact match function.
			// TODO - maybe generate separate error?
			errors_.push_back( ReportTooManySuitableOverloadedFunctions( initializer_expression.GetFilePos() ) );
			return nullptr;
		}
		else if( !convertible_function_variables.empty() )
			function_variable= convertible_function_variables.front();
	}
	if( function_variable == nullptr )
	{
		errors_.push_back( ReportCouldNotSelectOverloadedFunction( initializer_expression.GetFilePos() ) );
		return nullptr;
	}
	if( function_variable->is_deleted )
		errors_.push_back( ReportAccessingDeletedMethod( initializer_expression.GetFilePos() ) );

	llvm::Value* function_value= function_variable->llvm_function;
	if( function_variable->type != function_pointer_type.function )
		function_value= function_context.llvm_ir_builder.CreatePointerCast( function_value, variable.type.GetLLVMType() );

	function_context.llvm_ir_builder.CreateStore( function_value, variable.llvm_value );
	return function_variable->llvm_function;
}

} // namespace CodeBuilderPrivate

} // namespace U
