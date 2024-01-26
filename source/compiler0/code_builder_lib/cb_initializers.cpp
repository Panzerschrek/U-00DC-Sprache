#include <set>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constant.h>
#include <llvm/IR/LLVMContext.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

namespace U
{

llvm::Constant* CodeBuilder::ApplyInitializer(
	const VariablePtr& variable,
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::Initializer& initializer )
{
	return
		std::visit(
			[&]( const auto& t )
			{
				return ApplyInitializerImpl( variable, names, function_context, t );
			},
			initializer );
}

llvm::Constant* CodeBuilder::ApplyInitializerImpl(
	const VariablePtr&,
	NamesScope& names_scope,
	FunctionContext&,
	const Synt::EmptyVariant& )
{
	REPORT_ERROR( BuildFailed, names_scope.GetErrors(), SrcLoc( 0, 1, 0 ), "Reached empty variant initializer!" );
	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyInitializerImpl(
	const VariablePtr& variable,
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::SequenceInitializer& initializer )
{
	if( const ArrayType* const array_type= variable->type.GetArrayType() )
	{
		if( initializer.initializers.size() != array_type->element_count )
		{
			REPORT_ERROR( ArrayInitializersCountMismatch,
				names.GetErrors(),
				initializer.src_loc,
				array_type->element_count,
				initializer.initializers.size() );
			return nullptr;
			// SPRACHE_TODO - add array continious initializers.
		}

		const VariableMutPtr array_member=
			Variable::Create(
				array_type->element_type,
				ValueType::ReferenceMut,
				Variable::Location::Pointer,
				variable->name + "[]" );

		function_context.variables_state.AddNode( array_member );
		function_context.variables_state.TryAddLink( variable, array_member, names.GetErrors(), initializer.src_loc );
		function_context.variables_state.TryAddInnerLinks( variable, array_member, names.GetErrors(), initializer.src_loc );

		bool is_constant= array_type->element_type.CanBeConstexpr();
		llvm::SmallVector<llvm::Constant*, 16> members_constants;

		const bool requires_destruction= array_type->element_type.HaveDestructor();
		llvm::SmallVector<VariablePtr, 8> temp_initialized_variables;

		for( size_t i= 0u; i < initializer.initializers.size(); i++ )
		{
			array_member->llvm_value= CreateArrayElementGEP( function_context, *variable, i );

			llvm::Constant* const member_constant=
				ApplyInitializer( array_member, names, function_context, initializer.initializers[i] );

			if( is_constant && member_constant != nullptr )
				members_constants.push_back( member_constant );
			else
				is_constant= false;

			if( requires_destruction )
			{
				// Create temp variable for initialized member in order to call destructor in case of return or await during further array elements initialization.
				const VariableMutPtr temp_initialized_variable=
					Variable::Create(
						array_type->element_type,
						ValueType::Value,
						Variable::Location::Pointer,
						array_member->name,
						array_member->llvm_value );
				function_context.variables_state.AddNode( temp_initialized_variable );

				temp_initialized_variable->preserve_temporary= true;
				RegisterTemporaryVariable( function_context, temp_initialized_variable );
				temp_initialized_variables.push_back( temp_initialized_variable );
			}
		}

		function_context.variables_state.RemoveNode( array_member );

		for( const VariablePtr& temp_initialized_variable : temp_initialized_variables )
			function_context.variables_state.MoveNode( temp_initialized_variable );

		U_ASSERT( members_constants.size() == initializer.initializers.size() || !is_constant );

		if( is_constant )
			return llvm::ConstantArray::get( array_type->llvm_type, members_constants );
	}
	else if( const TupleType* const tuple_type= variable->type.GetTupleType() )
	{
		if( initializer.initializers.size() != tuple_type->element_types.size() )
		{
			REPORT_ERROR( TupleInitializersCountMismatch,
				names.GetErrors(),
				initializer.src_loc,
				tuple_type->element_types.size(),
				initializer.initializers.size() );
			return nullptr;
		}

		bool is_constant= variable->type.CanBeConstexpr();
		llvm::SmallVector<llvm::Constant*, 16> members_constants;

		llvm::SmallVector<VariablePtr, 8> temp_initialized_variables;

		for( size_t i= 0u; i < initializer.initializers.size(); ++i )
		{
			const Type& element_type= tuple_type->element_types[i];

			const VariablePtr tuple_element=
				Variable::Create(
					element_type,
					ValueType::ReferenceMut,
					Variable::Location::Pointer,
					variable->name + "[" + std::to_string(i) + "]",
					CreateTupleElementGEP( function_context, *variable, i ) );

			function_context.variables_state.AddNode( tuple_element );
			function_context.variables_state.TryAddLink( variable, tuple_element, names.GetErrors(), initializer.src_loc );
			function_context.variables_state.TryAddInnerLinksForTupleElement( variable, tuple_element, i, names.GetErrors(), initializer.src_loc );

			llvm::Constant* const member_constant=
				ApplyInitializer( tuple_element, names, function_context, initializer.initializers[i] );

			if( is_constant && member_constant != nullptr )
				members_constants.push_back( member_constant );
			else
				is_constant= false;

			function_context.variables_state.RemoveNode( tuple_element );

			if( element_type.HaveDestructor() )
			{
				// Create temp variable for initialized member in order to call destructor in case of return or await during further tuple elements initialization.
				const VariableMutPtr temp_initialized_variable=
					Variable::Create(
						element_type,
						ValueType::Value,
						Variable::Location::Pointer,
						tuple_element->name,
						tuple_element->llvm_value );
				function_context.variables_state.AddNode( temp_initialized_variable );

				temp_initialized_variable->preserve_temporary= true;
				RegisterTemporaryVariable( function_context, temp_initialized_variable );
				temp_initialized_variables.push_back( temp_initialized_variable );
			}
		}

		for( const VariablePtr& temp_initialized_variable : temp_initialized_variables )
			function_context.variables_state.MoveNode( temp_initialized_variable );

		U_ASSERT( members_constants.size() == initializer.initializers.size() || !is_constant );

		if( is_constant )
			return llvm::ConstantStruct::get( tuple_type->llvm_type, members_constants );
	}
	else
	{
		REPORT_ERROR( ArrayInitializerForNonArray, names.GetErrors(), initializer.src_loc );
		return nullptr;
	}

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyInitializerImpl(
	const VariablePtr& variable,
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::StructNamedInitializer& initializer )
{
	const Class* const class_type= variable->type.GetClassType();
	if( class_type == nullptr || class_type->kind != Class::Kind::Struct )
	{
		REPORT_ERROR( StructInitializerForNonStruct, names.GetErrors(), initializer.src_loc );
		return nullptr;
	}

	if( class_type->have_explicit_noncopy_constructors )
		REPORT_ERROR( InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors, names.GetErrors(), initializer.src_loc );

	ClassFieldsVector<bool> initialized_fields;
	initialized_fields.resize( class_type->llvm_type->getNumElements(), false );

	ClassFieldsVector<llvm::Constant*> constant_initializers;
	bool all_fields_are_constant= false;
	if( class_type->can_be_constexpr )
	{
		constant_initializers.resize( class_type->llvm_type->getNumElements(), nullptr );
		all_fields_are_constant= true;
	}

	llvm::SmallVector<VariablePtr, 8> temp_initialized_variables;

	for( const Synt::StructNamedInitializer::MemberInitializer& member_initializer : initializer.members_initializers )
	{
		if( member_initializer.completion_requested )
		{
			ComleteClassOwnFields( class_type, member_initializer.name );
			continue;
		}

		const NamesScopeValue* const class_member= class_type->members->GetThisScopeValue( member_initializer.name );
		if( class_member == nullptr )
		{
			REPORT_ERROR( NameNotFound, names.GetErrors(), initializer.src_loc, member_initializer.name );
			continue;
		}
		CollectDefinition( *class_member, member_initializer.src_loc );

		const ClassFieldPtr field= class_member->value.GetClassField();
		if( field == nullptr )
		{
			REPORT_ERROR( InitializerForNonfieldStructMember, names.GetErrors(), initializer.src_loc, member_initializer.name );
			continue;
		}
		if( field->class_ != variable->type )
		{
			REPORT_ERROR( InitializerForBaseClassField, names.GetErrors(), initializer.src_loc, member_initializer.name );
			continue;
		}

		if( field->index < initialized_fields.size() )
		{
			if( initialized_fields[field->index] )
			{
				REPORT_ERROR( DuplicatedStructMemberInitializer, names.GetErrors(), initializer.src_loc, member_initializer.name );
				continue;
			}
			initialized_fields[field->index]= true;
		}

		llvm::Constant* constant_initializer= nullptr;
		if( field->is_reference )
			constant_initializer=
				InitializeReferenceField( variable, *field, member_initializer.initializer, names, function_context );
		else
		{
			const VariablePtr struct_member=
				Variable::Create(
					field->type,
					ValueType::ReferenceMut,
					Variable::Location::Pointer,
					variable->name + "." + member_initializer.name,
					CreateClassFieldGEP( function_context, *variable, field->index ) );

			function_context.variables_state.AddNode( struct_member );
			function_context.variables_state.TryAddLink( variable, struct_member, names.GetErrors(), initializer.src_loc );
			function_context.variables_state.TryAddInnerLinksForClassField( variable, struct_member, *field, names.GetErrors(), initializer.src_loc );

			constant_initializer=
				ApplyInitializer( struct_member, names, function_context, member_initializer.initializer );

			function_context.variables_state.RemoveNode( struct_member );

			if( field->type.HaveDestructor() )
			{
				// Create temp variable for initialized member in order to call destructor in case of return or await during further struct elements initialization.
				const VariableMutPtr temp_initialized_variable=
					Variable::Create(
						field->type,
						ValueType::Value,
						Variable::Location::Pointer,
						struct_member->name,
						struct_member->llvm_value );
				function_context.variables_state.AddNode( temp_initialized_variable );

				temp_initialized_variable->preserve_temporary= true;
				RegisterTemporaryVariable( function_context, temp_initialized_variable );
				temp_initialized_variables.push_back( temp_initialized_variable );
			}
		}

		if( constant_initializer == nullptr )
			all_fields_are_constant= false;
		if( all_fields_are_constant )
			constant_initializers[field->index]= constant_initializer;
	}

	for( const VariablePtr& temp_initialized_variable : temp_initialized_variables )
		function_context.variables_state.MoveNode( temp_initialized_variable );

	for( const ClassFieldPtr& field : class_type->fields_order )
	{
		if( field == nullptr )
			continue;

		if( field->index < initialized_fields.size() && initialized_fields[ field->index ] )
			continue;

		llvm::Constant* constant_initializer= nullptr;
		if( field->is_reference )
		{
			if( field->syntax_element == nullptr || field->syntax_element->initializer == nullptr )
				REPORT_ERROR( ExpectedInitializer, names.GetErrors(), initializer.src_loc, field->GetName() ); // References is not default-constructible.
			else
				constant_initializer= InitializeReferenceClassFieldWithInClassIninitalizer( variable, *field, function_context );
		}
		else
		{
			const VariablePtr struct_member=
				Variable::Create(
					field->type,
					ValueType::ReferenceMut,
					Variable::Location::Pointer,
					variable->name + "." + field->GetName(),
					CreateClassFieldGEP( function_context, *variable, field->index ) );

			function_context.variables_state.AddNode( struct_member );
			function_context.variables_state.TryAddLink( variable, struct_member, names.GetErrors(), initializer.src_loc );
			function_context.variables_state.TryAddInnerLinksForClassField( variable, struct_member, *field, names.GetErrors(), initializer.src_loc );

			if( field->syntax_element != nullptr && field->syntax_element->initializer != nullptr )
				constant_initializer=
					InitializeClassFieldWithInClassIninitalizer( struct_member, *field, function_context );
			else
				constant_initializer=
					ApplyEmptyInitializer( field->GetName(), initializer.src_loc, struct_member, names, function_context );

			function_context.variables_state.RemoveNode( struct_member );

			// No need to register temporary variable for initialized struct member here, since it's not possible to return or await from default initializer.
		}

		if( constant_initializer == nullptr )
			all_fields_are_constant= false;
		if( all_fields_are_constant )
			constant_initializers[field->index]= constant_initializer;
	}

	if( all_fields_are_constant && constant_initializers.size() == class_type->field_count )
		return llvm::ConstantStruct::get( class_type->llvm_type, constant_initializers );

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyInitializerImpl(
	const VariablePtr& variable,
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::ConstructorInitializer& initializer )
{
	return ApplyConstructorInitializer( variable, initializer.arguments, initializer.src_loc, names, function_context );
}

llvm::Constant* CodeBuilder::ApplyInitializerImpl(
	const VariablePtr& variable,
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::ConstructorInitializerSignatureHelp& initializer )
{
	(void)names;
	(void)function_context;
	(void)initializer;

	// Perform signature help as for temp variable construction. Proper constructors should be suggested.
	PerformSignatureHelp( Value( variable->type ) );
	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyInitializerImpl(
	const VariablePtr& variable,
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::Expression& initializer )
{
	const SrcLoc src_loc= Synt::GetExpressionSrcLoc(initializer);

	if( variable->type.GetFundamentalType() != nullptr ||
		variable->type.GetRawPointerType() != nullptr ||
		variable->type.GetEnumType() != nullptr )
	{
		const VariablePtr expression_result= BuildExpressionCodeEnsureVariable( initializer, names, function_context );
		if( expression_result->type != variable->type )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, variable->type, expression_result->type );
			return nullptr;
		}

		llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( *expression_result, function_context );
		CreateTypedStore( function_context, variable->type, value_for_assignment, variable->llvm_value );

		DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), src_loc );

		if( llvm::Constant* const constexpr_value= expression_result->constexpr_value )
			return constexpr_value;
	}
	else if( variable->type.GetFunctionPointerType() != nullptr )
		return InitializeFunctionPointer( variable, initializer, names, function_context );
	else if( variable->type.GetArrayType() != nullptr || variable->type.GetTupleType() != nullptr )
	{
		const VariablePtr expression_result= BuildExpressionCodeEnsureVariable( initializer, names, function_context );
		if( expression_result->type != variable->type )
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, variable->type, expression_result->type );
			return nullptr;
		}

		SetupReferencesInCopyOrMove( function_context, variable, expression_result, names.GetErrors(), src_loc );

		// Move or try call copy constructor.
		if( expression_result->value_type == ValueType::Value && expression_result->type == variable->type )
		{
			function_context.variables_state.MoveNode( expression_result );

			U_ASSERT( expression_result->location == Variable::Location::Pointer );
			if( !function_context.is_functionless_context )
			{
				CopyBytes( variable->llvm_value, expression_result->llvm_value, variable->type, function_context );
				CreateLifetimeEnd( function_context, expression_result->llvm_value );
			}

			DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), src_loc );
		}
		else
		{
			if( !variable->type.IsCopyConstructible() )
			{
				REPORT_ERROR( CopyConstructValueOfNoncopyableType, names.GetErrors(), src_loc, variable->type );
				return nullptr;
			}

			if( !function_context.is_functionless_context )
			{
				BuildCopyConstructorPart(
					variable->llvm_value,
					expression_result->llvm_value,
					variable->type,
					function_context );
			}
		}

		// Copy constructor for constexpr type is trivial, so, we can just take constexpr value of source.
		return expression_result->constexpr_value;
	}
	else if( variable->type.GetClassType() != nullptr )
	{
		// Currently we support "=" initializer for copying and moving of structs.

		VariablePtr expression_result= BuildExpressionCodeEnsureVariable( initializer, names, function_context );
		if( expression_result->type == variable->type )
		{} // Ok, same types.
		else if( ReferenceIsConvertible( expression_result->type, variable->type, names.GetErrors(), src_loc ) )
		{} // Ok, can do reference conversion.
		else if( const FunctionVariable* const conversion_constructor= GetConversionConstructor( expression_result->type, variable->type, names.GetErrors(), src_loc ) )
		{
			// Type conversion required.
			expression_result= ConvertVariable( expression_result, variable->type, *conversion_constructor, names, function_context, src_loc );
		}
		else
		{
			REPORT_ERROR( TypesMismatch, names.GetErrors(), src_loc, variable->type, expression_result->type );
			return nullptr;
		}

		SetupReferencesInCopyOrMove( function_context, variable, expression_result, names.GetErrors(), src_loc );

		// Move or try call copy constructor.
		// TODO - produce constant initializer for generated copy constructor, if source is constant.
		if( expression_result->value_type == ValueType::Value && expression_result->type == variable->type )
		{
			function_context.variables_state.MoveNode( expression_result );

			U_ASSERT( expression_result->location == Variable::Location::Pointer );
			if( !function_context.is_functionless_context )
			{
				CopyBytes( variable->llvm_value, expression_result->llvm_value, variable->type, function_context );
				CreateLifetimeEnd( function_context, expression_result->llvm_value );
			}

			DestroyUnusedTemporaryVariables( function_context, names.GetErrors(), src_loc );

			return expression_result->constexpr_value; // Move can preserve constexpr.
		}
		else
		{
			llvm::Value* const value_for_copy=
				CreateReferenceCast( expression_result->llvm_value, expression_result->type, variable->type, function_context );
			TryCallCopyConstructor(
				names.GetErrors(), src_loc, variable->llvm_value, value_for_copy, variable->type.GetClassType(), function_context );
		}
	}
	else U_ASSERT( false );

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyInitializerImpl(
	const VariablePtr& variable,
	NamesScope& names,
	FunctionContext& function_context,
	const Synt::ZeroInitializer& initializer )
{
	if( variable->type.GetFundamentalType() != nullptr ||
		variable->type.GetEnumType() != nullptr ||
		variable->type.GetRawPointerType() != nullptr ||
		variable->type.GetFunctionPointerType() != nullptr )
	{
		// "0" for numbers, "false" for boolean type, first element for enums, "nullptr" for function pointers.
		const auto zero_value= llvm::Constant::getNullValue( variable->type.GetLLVMType() );
		CreateTypedStore( function_context, variable->type, zero_value, variable->llvm_value );
		return zero_value;
	}
	else if( const ArrayType* const array_type= variable->type.GetArrayType() )
	{
		const VariableMutPtr array_member=
			Variable::Create(
				array_type->element_type,
				ValueType::ReferenceMut,
				Variable::Location::Pointer,
				variable->name + "[]" );

		function_context.variables_state.AddNode( array_member );
		function_context.variables_state.TryAddLink( variable, array_member, names.GetErrors(), initializer.src_loc );
		function_context.variables_state.TryAddInnerLinks( variable, array_member, names.GetErrors(), initializer.src_loc );

		GenerateLoop(
			array_type->element_count,
			[&](llvm::Value* const counter_value)
			{
				array_member->llvm_value= CreateArrayElementGEP( function_context, *variable, counter_value );
				ApplyInitializer( array_member, names, function_context, initializer );
			},
			function_context);

			function_context.variables_state.RemoveNode( array_member );

		if( array_type->element_type.CanBeConstexpr() )
			return llvm::Constant::getNullValue( array_type->llvm_type );
		else
			return nullptr;
	}
	else if( const TupleType* const tuple_type= variable->type.GetTupleType() )
	{
		for( const Type& element_type : tuple_type->element_types )
		{
			const size_t i= size_t( &element_type - tuple_type->element_types.data() );
			const VariablePtr tuple_element=
				Variable::Create(
					tuple_type->element_types[i],
					ValueType::ReferenceMut,
					Variable::Location::Pointer,
					variable->name + "[" + std::to_string(i) + "]",
					CreateTupleElementGEP( function_context, *variable, i ) );

			function_context.variables_state.AddNode( tuple_element );
			function_context.variables_state.TryAddLink( variable, tuple_element, names.GetErrors(), initializer.src_loc );
			function_context.variables_state.TryAddInnerLinksForTupleElement( variable, tuple_element, i, names.GetErrors(), initializer.src_loc );

			ApplyInitializer( tuple_element, names, function_context, initializer );

			function_context.variables_state.RemoveNode( tuple_element );
		}

		if( variable->type.CanBeConstexpr() )
			return llvm::Constant::getNullValue( tuple_type->llvm_type );
		else
			return nullptr;
	}
	else if( const Class* const class_type= variable->type.GetClassType() )
	{
		if( class_type->have_explicit_noncopy_constructors )
			REPORT_ERROR( InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors, names.GetErrors(), initializer.src_loc );
		if( class_type->kind != Class::Kind::Struct )
			REPORT_ERROR( ZeroInitializerForClass, names.GetErrors(), initializer.src_loc );

		bool all_fields_are_constant= variable->type.CanBeConstexpr();
		for( const ClassFieldPtr& field : class_type->fields_order )
		{
			if( field == nullptr )
				continue;

			if( field->is_reference )
			{
				all_fields_are_constant= false;
				REPORT_ERROR( UnsupportedInitializerForReference, names.GetErrors(), initializer.src_loc );
				continue;
			}

			const VariablePtr struct_member=
				Variable::Create(
					field->type,
					ValueType::ReferenceMut,
					Variable::Location::Pointer,
					variable->name + "." + field->GetName(),
					CreateClassFieldGEP( function_context, *variable, field->index ) );

			function_context.variables_state.AddNode( struct_member );
			function_context.variables_state.TryAddLink( variable, struct_member, names.GetErrors(), initializer.src_loc );
			function_context.variables_state.TryAddInnerLinksForClassField( variable, struct_member, *field, names.GetErrors(), initializer.src_loc );

			ApplyInitializer( struct_member, names, function_context, initializer );

			function_context.variables_state.RemoveNode( struct_member );
		}

		if( all_fields_are_constant )
			return llvm::Constant::getNullValue( class_type->llvm_type );
		else
			return nullptr;
	}
	else U_ASSERT( false );

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyInitializerImpl(
	const VariablePtr&,
	NamesScope& block_names,
	FunctionContext& function_context,
	const Synt::UninitializedInitializer& initializer )
{
	if( !function_context.is_in_unsafe_block )
		REPORT_ERROR( UninitializedInitializerOutsideUnsafeBlock, block_names.GetErrors(), initializer.src_loc );

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyEmptyInitializer(
	const std::string_view variable_name,
	const SrcLoc& src_loc,
	const VariablePtr variable,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( !variable->type.IsDefaultConstructible() )
	{
		REPORT_ERROR( ExpectedInitializer, block_names.GetErrors(), src_loc, variable_name );
		return nullptr;
	}

	if( variable->type.GetFundamentalType() != nullptr )
	{
		U_ASSERT( variable->type == void_type_ ); // "void" is only default-constructible fundamental type.
		return llvm::Constant::getNullValue( fundamental_llvm_types_.void_ );
	}
	else if( variable->type.GetEnumType() != nullptr || variable->type.GetRawPointerType() != nullptr || variable->type.GetFunctionPointerType() != nullptr )
	{
		// This type is not default-constructible, we should generate error about it before.
		U_ASSERT( false );
		return nullptr;
	}
	else if( const ArrayType* const array_type= variable->type.GetArrayType() )
	{
		const VariableMutPtr array_member=
			Variable::Create(
				array_type->element_type,
				ValueType::ReferenceMut,
				Variable::Location::Pointer,
				variable->name + "[]" );

		function_context.variables_state.AddNode( array_member );
		function_context.variables_state.TryAddLink( variable, array_member, block_names.GetErrors(), src_loc );
		function_context.variables_state.TryAddInnerLinks( variable, array_member, block_names.GetErrors(), src_loc );

		llvm::Constant* constant_initializer= nullptr;

		GenerateLoop(
			array_type->element_count,
			[&](llvm::Value* const counter_value)
			{
				array_member->llvm_value= CreateArrayElementGEP( function_context, *variable, counter_value );
				constant_initializer= ApplyEmptyInitializer( variable_name, src_loc, array_member, block_names, function_context );
			},
			function_context );

			function_context.variables_state.RemoveNode( array_member );

		if( constant_initializer != nullptr )
		{
			llvm::SmallVector<llvm::Constant*, 16> array_initializers;
			array_initializers.resize( size_t(array_type->element_count), constant_initializer );
			return llvm::ConstantArray::get( array_type->llvm_type, array_initializers );
		}
		return nullptr;
	}
	else if( const TupleType* const tuple_type= variable->type.GetTupleType() )
	{
		llvm::SmallVector<llvm::Constant*, 16> constant_initializers;

		for( const Type& element_type : tuple_type->element_types )
		{
			const size_t i= size_t( &element_type - tuple_type->element_types.data() );
			const VariablePtr tuple_element=
				Variable::Create(
					tuple_type->element_types[i],
					ValueType::ReferenceMut,
					Variable::Location::Pointer,
					variable->name + "[" + std::to_string(i) + "]",
					CreateTupleElementGEP( function_context, *variable, i ) );

			function_context.variables_state.AddNode( tuple_element );
			function_context.variables_state.TryAddLink( variable, tuple_element, block_names.GetErrors(), src_loc );
			function_context.variables_state.TryAddInnerLinksForTupleElement( variable, tuple_element, i, block_names.GetErrors(), src_loc );

			llvm::Constant* const constant_initializer=
				ApplyEmptyInitializer( variable_name, src_loc, tuple_element, block_names, function_context );

			if( constant_initializer != nullptr )
				constant_initializers.push_back( constant_initializer );

			function_context.variables_state.RemoveNode( tuple_element );
		}

		if( constant_initializers.size() == tuple_type->element_types.size() )
			return llvm::ConstantStruct::get( tuple_type->llvm_type, constant_initializers );
		return nullptr;
	}
	else if( const Class* const class_type= variable->type.GetClassType() )
	{
		// If initializer for class variable is empty, try to call default constructor.

		const NamesScopeValue* constructor_value=
			class_type->members->GetThisScopeValue( Keyword( Keywords::constructor_ ) );
		U_ASSERT( constructor_value != nullptr );
		const OverloadedFunctionsSetPtr constructors_set= constructor_value->value.GetFunctionsSet();
		U_ASSERT( constructors_set != nullptr );
		PrepareFunctionsSetAndBuildConstexprBodies( *class_type->members, *constructors_set );

		ThisOverloadedMethodsSet this_overloaded_methods_set;
		this_overloaded_methods_set.this_= variable;
		this_overloaded_methods_set.overloaded_methods_set= constructors_set;

		CallFunctionValue( std::move(this_overloaded_methods_set), {}, src_loc, std::nullopt, block_names, function_context );

		return nullptr;
	}
	else U_ASSERT(false);

	return nullptr;
}

llvm::Constant* CodeBuilder::ApplyConstructorInitializer(
	const VariablePtr& variable,
	const llvm::ArrayRef<Synt::Expression> synt_args,
	const SrcLoc& src_loc,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	if( const FundamentalType* const dst_type= variable->type.GetFundamentalType() )
	{
		if( dst_type->fundamental_type == U_FundamentalType::void_ && synt_args.empty() )
			return llvm::Constant::getNullValue( dst_type->llvm_type );

		if( synt_args.size() != 1u )
		{
			REPORT_ERROR( FundamentalTypesHaveConstructorsWithExactlyOneParameter, block_names.GetErrors(), src_loc );
			return nullptr;
		}

		const VariablePtr src_var= BuildExpressionCodeEnsureVariable( synt_args.front(), block_names, function_context );

		const FundamentalType* src_type= src_var->type.GetFundamentalType();
		if( src_type == nullptr )
		{
			// Allow explicit conversions of enums to ints.
			if( const Enum* const enum_type= src_var->type.GetEnumType () )
				src_type= &enum_type->underlying_type;
		}

		if( src_type == nullptr )
		{
			REPORT_ERROR( TypesMismatch, block_names.GetErrors(), src_loc, variable->type, src_var->type );
			return nullptr;
		}

		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( *src_var, function_context );
		DestroyUnusedTemporaryVariables( function_context, block_names.GetErrors(), src_loc );

		if( value_for_assignment != nullptr )
		{
			if( dst_type->fundamental_type != src_type->fundamental_type )
			{
				// Perform fundamental types conversion.

				const uint64_t src_size= src_type->GetSize();
				const uint64_t dst_size= dst_type->GetSize();
				if( IsInteger( dst_type->fundamental_type ) && IsInteger( src_type->fundamental_type ) )
				{
					// int to int
					if( src_size < dst_size )
					{
						// We lost here some values in conversions, such i16 => u32, if src_type is signed.
						if( IsUnsignedInteger( dst_type->fundamental_type ) )
							value_for_assignment= function_context.llvm_ir_builder.CreateZExt( value_for_assignment, dst_type->llvm_type );
						else
							value_for_assignment= function_context.llvm_ir_builder.CreateSExt( value_for_assignment, dst_type->llvm_type );
					}
					else if( src_size > dst_size )
						value_for_assignment= function_context.llvm_ir_builder.CreateTrunc( value_for_assignment, dst_type->llvm_type );
				}
				else if( IsFloatingPoint( dst_type->fundamental_type ) && IsFloatingPoint( src_type->fundamental_type ) )
				{
					// float to float
					if( src_size < dst_size )
						value_for_assignment= function_context.llvm_ir_builder.CreateFPExt( value_for_assignment, dst_type->llvm_type );
					else if( src_size > dst_size )
						value_for_assignment= function_context.llvm_ir_builder.CreateFPTrunc( value_for_assignment, dst_type->llvm_type );
					else U_ASSERT(false);
				}
				else if( IsFloatingPoint( dst_type->fundamental_type ) && IsInteger( src_type->fundamental_type ) )
				{
					// int to float
					if( IsSignedInteger( src_type->fundamental_type ) )
						value_for_assignment= function_context.llvm_ir_builder.CreateSIToFP( value_for_assignment, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateUIToFP( value_for_assignment, dst_type->llvm_type );
				}
				else if( IsInteger( dst_type->fundamental_type ) && IsFloatingPoint( src_type->fundamental_type ) )
				{
					// float to int
					// TODO - fix this. Use something like "llvm.fptosi.sat" to avoid undefined behaviour in cases where result can't fit into destination.
					if( IsSignedInteger( dst_type->fundamental_type ) )
						value_for_assignment= function_context.llvm_ir_builder.CreateFPToSI( value_for_assignment, dst_type->llvm_type );
					else
						value_for_assignment= function_context.llvm_ir_builder.CreateFPToUI( value_for_assignment, dst_type->llvm_type );
				}
				else if( IsChar( dst_type->fundamental_type ) && ( IsInteger( src_type->fundamental_type ) || IsChar( src_type->fundamental_type ) ) )
				{
					// int to char or char to char
					if( src_size < dst_size )
						value_for_assignment= function_context.llvm_ir_builder.CreateZExt( value_for_assignment, dst_type->llvm_type );
					else if( src_size > dst_size )
						value_for_assignment= function_context.llvm_ir_builder.CreateTrunc( value_for_assignment, dst_type->llvm_type );
				}
				else if( IsInteger( dst_type->fundamental_type ) && IsChar( src_type->fundamental_type ) )
				{
					// char to int
					if( src_size < dst_size )
					{
						// We lost here some values in conversions, such i16 => u32, if src_type is signed.
						if( IsUnsignedInteger( dst_type->fundamental_type ) )
							value_for_assignment= function_context.llvm_ir_builder.CreateZExt( value_for_assignment, dst_type->llvm_type );
						else
							value_for_assignment= function_context.llvm_ir_builder.CreateSExt( value_for_assignment, dst_type->llvm_type );
					}
					else if( src_size > dst_size )
						value_for_assignment= function_context.llvm_ir_builder.CreateTrunc( value_for_assignment, dst_type->llvm_type );
				}
				else if( src_size == dst_size && (
					( IsByte( dst_type->fundamental_type ) && IsInteger( src_type->fundamental_type ) ) ||
					( IsInteger( dst_type->fundamental_type ) && IsByte( src_type->fundamental_type ) ) ) )
				{
					// Perform int -> bytes or bytes -> int conversion.
					// Do nothing, because internally bytes and int of same size is same type.
				}
				else if( src_size == dst_size && (
					( IsByte( dst_type->fundamental_type ) && IsChar( src_type->fundamental_type ) ) ||
					( IsChar( dst_type->fundamental_type ) && IsByte( src_type->fundamental_type ) ) ) )
				{
					// Perform char -> bytes or bytes -> char conversion.
					// Do nothing, because internally bytes and char of same size is same type.
				}
				else if( src_size == dst_size && (
					( IsByte( dst_type->fundamental_type ) && IsFloatingPoint( src_type->fundamental_type ) ) ||
					( IsFloatingPoint( dst_type->fundamental_type ) && IsByte( src_type->fundamental_type ) ) ) )
				{
					// Perfrom float -> bytes or bytes->float conversion.
					value_for_assignment= function_context.llvm_ir_builder.CreateBitCast( value_for_assignment, dst_type->llvm_type );
				}
				else
				{
					if( dst_type->fundamental_type == U_FundamentalType::bool_ )
					{
						// TODO - error, bool have no constructors from other types
					}
					REPORT_ERROR( TypesMismatch, block_names.GetErrors(), src_loc, variable->type, src_var->type );
					return nullptr;
				}
			} // If needs conversion

			CreateTypedStore( function_context, variable->type, value_for_assignment, variable->llvm_value );
			return llvm::dyn_cast<llvm::Constant>(value_for_assignment);
		}

		return nullptr;
	}
	else if( variable->type.GetEnumType() != nullptr || variable->type.GetRawPointerType() != nullptr )
	{
		if( synt_args.size() != 1u )
		{
			// TODO - generate separate error for enums.
			REPORT_ERROR( FundamentalTypesHaveConstructorsWithExactlyOneParameter, block_names.GetErrors(), src_loc );
			return nullptr;
		}

		const VariablePtr expression_result= BuildExpressionCodeEnsureVariable( synt_args.front(), block_names, function_context );
		if( expression_result->type != variable->type )
		{
			REPORT_ERROR( TypesMismatch, block_names.GetErrors(), src_loc, variable->type, expression_result->type );
			return nullptr;
		}

		CreateTypedStore(
			function_context,
			variable->type,
			CreateMoveToLLVMRegisterInstruction( *expression_result, function_context ),
			variable->llvm_value );

		DestroyUnusedTemporaryVariables( function_context, block_names.GetErrors(), src_loc );

		return expression_result->constexpr_value;
	}
	else if( variable->type.GetFunctionPointerType() != nullptr )
	{
		if( synt_args.size() != 1u )
		{
			// TODO - generate separate error for function pointers.
			REPORT_ERROR( FundamentalTypesHaveConstructorsWithExactlyOneParameter, block_names.GetErrors(), src_loc );
			return nullptr;
		}

		return InitializeFunctionPointer( variable, synt_args.front(), block_names, function_context );
	}
	else if( variable->type.GetArrayType() != nullptr || variable->type.GetTupleType() != nullptr )
	{
		if( synt_args.size() != 1u )
		{
			REPORT_ERROR( ConstructorInitializerForUnsupportedType, block_names.GetErrors(), src_loc );
			return nullptr;
		}

		const VariablePtr expression_result= BuildExpressionCodeEnsureVariable( synt_args.front(), block_names, function_context );
		if( expression_result->type != variable->type )
		{
			REPORT_ERROR( TypesMismatch, block_names.GetErrors(), src_loc, variable->type, expression_result->type );
			return nullptr;
		}

		SetupReferencesInCopyOrMove( function_context, variable, expression_result, block_names.GetErrors(), src_loc );

		// Copy/move initialize array/tuple.
		if( expression_result->value_type == ValueType::Value )
		{
			function_context.variables_state.MoveNode( expression_result );

			U_ASSERT( expression_result->location == Variable::Location::Pointer );
			if( !function_context.is_functionless_context )
			{
				CopyBytes( variable->llvm_value, expression_result->llvm_value, variable->type, function_context );
				CreateLifetimeEnd( function_context, expression_result->llvm_value );
			}
		}
		else
		{
			if( !variable->type.IsCopyConstructible() )
			{
				REPORT_ERROR( CopyConstructValueOfNoncopyableType, block_names.GetErrors(), src_loc, variable->type );
				return nullptr;
			}

			if( !function_context.is_functionless_context )
			{
				BuildCopyConstructorPart(
					variable->llvm_value,
					expression_result->llvm_value,
					variable->type,
					function_context );
			}
		}

		// Copy constructor for constexpr type is trivial, so, we can just take constexpr value of source.
		return expression_result->constexpr_value;
	}
	else if( const Class* const class_type= variable->type.GetClassType() )
	{
		// Try do move-construct.
		bool needs_move_constuct= false;
		if( synt_args.size() == 1u )
		{
			const bool prev_is_functionless_context= function_context.is_functionless_context;
			function_context.is_functionless_context= true;
			const auto state= SaveFunctionContextState( function_context );
			{
				const StackVariablesStorage dummy_stack_variables_storage( function_context );

				const VariablePtr initializer_value= BuildExpressionCodeEnsureVariable( synt_args.front(), block_names, function_context );
				needs_move_constuct= initializer_value->type == variable->type && initializer_value->value_type == ValueType::Value;
			}

			RestoreFunctionContextState( function_context, state );
			function_context.is_functionless_context= prev_is_functionless_context;
		}
		if( needs_move_constuct )
		{
			const VariablePtr initializer_variable= BuildExpressionCodeEnsureVariable( synt_args.front(), block_names, function_context );

			SetupReferencesInCopyOrMove( function_context, variable, initializer_variable, block_names.GetErrors(), src_loc );

			function_context.variables_state.MoveNode( initializer_variable );

			U_ASSERT( initializer_variable->location == Variable::Location::Pointer );
			if( !function_context.is_functionless_context )
			{
				CopyBytes( variable->llvm_value, initializer_variable->llvm_value, variable->type, function_context );
				CreateLifetimeEnd( function_context, initializer_variable->llvm_value );
			}

			return initializer_variable->constexpr_value; // Move can preserve constexpr.
		}

		const NamesScopeValue* constructor_value=
			class_type->members->GetThisScopeValue( Keyword( Keywords::constructor_ ) );
		if( constructor_value == nullptr )
		{
			REPORT_ERROR( ClassHaveNoConstructors, block_names.GetErrors(), src_loc );
			return nullptr;
		}

		const OverloadedFunctionsSetPtr constructors_set= constructor_value->value.GetFunctionsSet();
		U_ASSERT( constructors_set != nullptr );
		PrepareFunctionsSetAndBuildConstexprBodies( *class_type->members, *constructors_set );

		ThisOverloadedMethodsSet this_overloaded_methods_set;
		this_overloaded_methods_set.this_= variable;
		this_overloaded_methods_set.overloaded_methods_set= constructors_set;

		CallFunctionValue( std::move(this_overloaded_methods_set), synt_args, src_loc, std::nullopt, block_names, function_context );
	}
	else U_ASSERT( false );

	return nullptr;
}

void CodeBuilder::BuildConstructorInitialization(
	const VariablePtr& this_,
	const Class& base_class,
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::StructNamedInitializer& constructor_initialization_list )
{
	ClassFieldsVector<bool> initialized_fields;
	initialized_fields.resize( base_class.llvm_type->getNumElements(), false );

	// Check for errors, build list of initialized fields.
	bool have_fields_errors= false;
	bool base_initialized= false;
	for( const Synt::StructNamedInitializer::MemberInitializer& field_initializer : constructor_initialization_list.members_initializers )
	{
		if( field_initializer.completion_requested )
			ComleteClassOwnFields( &base_class, field_initializer.name );

		if( field_initializer.name == Keywords::base_ )
		{
			if( base_class.base_class == nullptr )
			{
				have_fields_errors= true;
				REPORT_ERROR( BaseUnavailable, names_scope.GetErrors(), constructor_initialization_list.src_loc );
				continue;
			}
			if( base_initialized )
			{
				have_fields_errors= true;
				REPORT_ERROR( DuplicatedStructMemberInitializer, names_scope.GetErrors(), constructor_initialization_list.src_loc, field_initializer.name );
				continue;
			}
			base_initialized= true;
			function_context.base_initialized= false;
			continue;
		}

		const NamesScopeValue* const class_member= base_class.members->GetThisScopeValue( field_initializer.name );
		if( class_member == nullptr )
		{
			have_fields_errors= true;
			REPORT_ERROR( NameNotFound, names_scope.GetErrors(), constructor_initialization_list.src_loc, field_initializer.name );
			continue;
		}
		CollectDefinition( *class_member, field_initializer.src_loc );

		const ClassFieldPtr field= class_member->value.GetClassField();
		if( field == nullptr )
		{
			have_fields_errors= true;
			REPORT_ERROR( InitializerForNonfieldStructMember, names_scope.GetErrors(), constructor_initialization_list.src_loc, field_initializer.name );
			continue;
		}
		if( field->class_ != &base_class )
		{
			have_fields_errors= true;
			REPORT_ERROR( InitializerForBaseClassField, names_scope.GetErrors(), constructor_initialization_list.src_loc, field_initializer.name );
			continue;
		}

		if( field->index < initialized_fields.size() )
		{
			if( initialized_fields[ field->index ] )
			{
				have_fields_errors= true;
				REPORT_ERROR( DuplicatedStructMemberInitializer, names_scope.GetErrors(), constructor_initialization_list.src_loc, field_initializer.name );
				continue;
			}

			initialized_fields[ field->index ]= true;
		}
	} // for fields initializers

	if( have_fields_errors )
		return;

	const StackVariablesStorage temp_variables_storage( function_context );

	function_context.whole_this_is_unavailable= true;
	function_context.initialized_this_fields.resize( base_class.llvm_type->getNumElements(), false );

	// Initialize fields, missing in initializer list.
	for( const ClassFieldPtr& field : base_class.fields_order )
	{
		if( field == nullptr || ( field->index < initialized_fields.size() && initialized_fields[field->index] ) )
			continue;

		if( field->is_reference )
		{
			if( field->syntax_element == nullptr || field->syntax_element->initializer == nullptr )
			{
				REPORT_ERROR( ExpectedInitializer, names_scope.GetErrors(), constructor_initialization_list.src_loc, field->GetName() );
				continue;
			}
			InitializeReferenceClassFieldWithInClassIninitalizer( this_, *field, function_context );
		}
		else if( !field->is_mutable )
		{
			// HACK! Can't use "AccessClassField" here, since it returns immtable reference.
			// So, just create derived reference field, not a child node for the field.
			const VariablePtr field_variable=
				Variable::Create(
					field->type,
					ValueType::ReferenceMut,
					Variable::Location::Pointer,
					this_->name + "." + field->GetName(),
					CreateClassFieldGEP( function_context, *this_, field->index ) );

			function_context.variables_state.AddNode( field_variable );
			function_context.variables_state.TryAddLink( this_, field_variable, names_scope.GetErrors(), constructor_initialization_list.src_loc );
			function_context.variables_state.TryAddInnerLinksForClassField( this_, field_variable, *field, names_scope.GetErrors(), constructor_initialization_list.src_loc );

			if( field->syntax_element != nullptr && field->syntax_element->initializer != nullptr )
				InitializeClassFieldWithInClassIninitalizer( field_variable, *field, function_context );
			else
				ApplyEmptyInitializer( field->GetName(), constructor_initialization_list.src_loc, field_variable, names_scope, function_context );

			function_context.variables_state.RemoveNode( field_variable );
		}
		else
		{
			const VariablePtr field_variable=
				AccessClassField( names_scope, function_context, this_, *field, field->GetName(), constructor_initialization_list.src_loc ).GetVariable();
			U_ASSERT( field_variable != nullptr );

			if( field->syntax_element != nullptr && field->syntax_element->initializer != nullptr )
				InitializeClassFieldWithInClassIninitalizer( field_variable, *field, function_context );
			else
				ApplyEmptyInitializer( field->GetName(), constructor_initialization_list.src_loc, field_variable, names_scope, function_context );
		}

		function_context.initialized_this_fields[ field->index ]= true;
	}

	// Initialize base (if it is not listed).
	if( !base_initialized && base_class.base_class != nullptr )
	{
		// Apply default initializer for base class.

		// It is safe to access "base" as child node here since it is possible to call only constructor but not any virtual method.
		const VariablePtr base_variable= AccessClassBase( this_, function_context );

		ApplyEmptyInitializer( base_class.base_class->members->GetThisNamespaceName(), constructor_initialization_list.src_loc, base_variable, names_scope, function_context );
		function_context.base_initialized= true;
	}

	// Initialize fields listed in the initializer.
	for( const Synt::StructNamedInitializer::MemberInitializer& field_initializer : constructor_initialization_list.members_initializers )
	{
		if( field_initializer.name == Keywords::base_ )
		{
			// It is safe to access "base" as child node here since it is possible to call only constructor but not any virtual method.
			const VariablePtr base_variable= AccessClassBase( this_, function_context );

			ApplyInitializer( base_variable, names_scope, function_context, field_initializer.initializer );
			function_context.base_initialized= true;
			continue;
		}

		const NamesScopeValue* const class_member=
			base_class.members->GetThisScopeValue( field_initializer.name );
		U_ASSERT( class_member != nullptr );
		const ClassFieldPtr field= class_member->value.GetClassField();
		U_ASSERT( field != nullptr );

		if( field->is_reference )
			InitializeReferenceField( this_, *field, field_initializer.initializer, names_scope, function_context );
		else if( !field->is_mutable )
		{
			// HACK! Can't use "AccessClassField" here, since it returns immtable reference.
			// So, just create derived reference field, not a child node for the field.
			const VariablePtr field_variable=
				Variable::Create(
					field->type,
					ValueType::ReferenceMut,
					Variable::Location::Pointer,
					this_->name + "." + field_initializer.name,
					CreateClassFieldGEP( function_context, *this_, field->index ) );

			function_context.variables_state.AddNode( field_variable );
			function_context.variables_state.TryAddLink( this_, field_variable, names_scope.GetErrors(), Synt::GetInitializerSrcLoc(field_initializer.initializer) );
			function_context.variables_state.TryAddInnerLinksForClassField( this_, field_variable, *field, names_scope.GetErrors(), constructor_initialization_list.src_loc );

			ApplyInitializer( field_variable, names_scope, function_context, field_initializer.initializer );

			function_context.variables_state.RemoveNode( field_variable );
		}
		else
		{
			const VariablePtr field_variable=
				AccessClassField( names_scope, function_context, this_, *field, field_initializer.name, constructor_initialization_list.src_loc ).GetVariable();
			U_ASSERT( field_variable != nullptr );

			ApplyInitializer( field_variable, names_scope, function_context, field_initializer.initializer );
		}

		function_context.initialized_this_fields[ field->index ]= true;
	} // for fields initializers

	function_context.whole_this_is_unavailable= false;
	function_context.initialized_this_fields.clear();

	CallDestructors( temp_variables_storage, names_scope, function_context, constructor_initialization_list.src_loc );
	SetupVirtualTablePointers( this_->llvm_value, base_class, function_context );
}

llvm::Constant* CodeBuilder::InitializeReferenceField(
	const VariablePtr& variable,
	const ClassField& field,
	const Synt::Initializer& initializer,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	U_ASSERT( variable->type.GetClassType() != nullptr );
	U_ASSERT( variable->type.GetClassType() == field.class_ );

	const SrcLoc initializer_src_loc= Synt::GetInitializerSrcLoc( initializer );
	const Synt::Expression* initializer_expression= nullptr;
	if( const auto expression_initializer= std::get_if<Synt::Expression>( &initializer ) )
		initializer_expression= expression_initializer;
	else if( const auto constructor_initializer= std::get_if<Synt::ConstructorInitializer>( &initializer ) )
	{
		if( constructor_initializer->arguments.size() != 1u )
		{
			REPORT_ERROR( ReferencesHaveConstructorsWithExactlyOneParameter, block_names.GetErrors(), constructor_initializer->src_loc );
			return nullptr;
		}
		initializer_expression= &constructor_initializer->arguments.front();
	}
	else
	{
		REPORT_ERROR( UnsupportedInitializerForReference, block_names.GetErrors(), initializer_src_loc );
		return nullptr;
	}

	const VariablePtr initializer_variable= BuildExpressionCodeEnsureVariable( *initializer_expression, block_names, function_context );

	const SrcLoc initializer_expression_src_loc= Synt::GetExpressionSrcLoc( *initializer_expression );
	if( !ReferenceIsConvertible( initializer_variable->type, field.type, block_names.GetErrors(), initializer_expression_src_loc ) )
	{
		REPORT_ERROR( TypesMismatch, block_names.GetErrors(), initializer_expression_src_loc, field.type, initializer_variable->type );
		return nullptr;
	}
	if( initializer_variable->value_type == ValueType::Value )
	{
		REPORT_ERROR( ExpectedReferenceValue, block_names.GetErrors(), initializer_expression_src_loc );
		return nullptr;
	}
	U_ASSERT( initializer_variable->location == Variable::Location::Pointer );

	if( field.is_mutable && initializer_variable->value_type == ValueType::ReferenceImut )
	{
		REPORT_ERROR( BindingConstReferenceToNonconstReference, block_names.GetErrors(), initializer_expression_src_loc );
		return nullptr;
	}

	// Link references.
	U_ASSERT( field.reference_tag < variable->inner_reference_nodes.size() );
	function_context.variables_state.TryAddLinkToAllAccessibleVariableNodesInnerReferences(
		initializer_variable,
		variable->inner_reference_nodes[field.reference_tag],
		block_names.GetErrors(),
		initializer_src_loc );

	llvm::Value* const address_of_reference= CreateClassFieldGEP( function_context, *variable, field.index );

	llvm::Value* const ref_to_store= CreateReferenceCast( initializer_variable->llvm_value, initializer_variable->type, field.type, function_context );
	CreateTypedReferenceStore( function_context, field.type, ref_to_store, address_of_reference );

	if( initializer_variable->constexpr_value != nullptr )
	{
		// We needs to store constant somewhere. Create global variable for it.
		llvm::Constant* constant_stored= CreateGlobalConstantVariable( initializer_variable->type, "_temp_const", initializer_variable->constexpr_value );

		if( field.type != initializer_variable->type )
			constant_stored=
				llvm::dyn_cast<llvm::Constant>( CreateReferenceCast( constant_stored, initializer_variable->type, field.type, function_context ) );

		return constant_stored;
	}

	return nullptr;
}

llvm::Constant* CodeBuilder::InitializeFunctionPointer(
	const VariablePtr& variable,
	const Synt::Expression& initializer_expression,
	NamesScope& block_names,
	FunctionContext& function_context )
{
	U_ASSERT( variable->type.GetFunctionPointerType() != nullptr );

	const SrcLoc initializer_expression_src_loc= Synt::GetExpressionSrcLoc( initializer_expression );
	const FunctionPointerType& function_pointer_type= *variable->type.GetFunctionPointerType();

	const Value initializer_value= BuildExpressionCode( initializer_expression, block_names, function_context );

	if( const VariablePtr initializer_variable= initializer_value.GetVariable() )
	{
		const FunctionPointerType* const intitializer_type= initializer_variable->type.GetFunctionPointerType();
		if( intitializer_type == nullptr ||
			!intitializer_type->function_type.PointerCanBeConvertedTo( function_pointer_type.function_type ) )
		{
			REPORT_ERROR( TypesMismatch, block_names.GetErrors(), initializer_expression_src_loc, variable->type, initializer_variable->type );
			return nullptr;
		}
		U_ASSERT( initializer_variable->type.GetFunctionPointerType() != nullptr );

		llvm::Value* value_for_assignment= CreateMoveToLLVMRegisterInstruction( *initializer_variable, function_context );
		if( initializer_variable->type != variable->type )
			value_for_assignment= function_context.llvm_ir_builder.CreatePointerCast( value_for_assignment, variable->type.GetLLVMType() );

		CreateTypedStore( function_context, variable->type, value_for_assignment, variable->llvm_value );
		return initializer_variable->constexpr_value;
	}

	OverloadedFunctionsSetConstPtr candidate_functions;
	if( const OverloadedFunctionsSetConstPtr overloaded_functions_set= initializer_value.GetFunctionsSet() )
		candidate_functions= overloaded_functions_set;
	else if( const ThisOverloadedMethodsSet* const overloaded_methods_set= initializer_value.GetThisOverloadedMethodsSet() )
		candidate_functions= overloaded_methods_set->overloaded_methods_set;
	else
	{
		// TODO - generate separate error
		REPORT_ERROR( ExpectedVariable, block_names.GetErrors(), initializer_expression_src_loc, initializer_value.GetKindName() );
		return nullptr;
	}

	// Try select one of overloaded functions.
	// Select function with same with pointer type, if it exists.
	// If there is no function with same type, select function, convertible to pointer type, but only if exists only one convertible function.
	const FunctionVariable* exact_match_function_variable= nullptr;
	llvm::SmallVector<const FunctionVariable*, 4> convertible_function_variables;

	for( const FunctionVariable& func : candidate_functions->functions )
	{
		if( func.type == function_pointer_type.function_type )
			exact_match_function_variable= &func;
		else if( func.type.PointerCanBeConvertedTo( function_pointer_type.function_type ) )
			convertible_function_variables.push_back(&func);
	}
	// Try also select template functions with zero template parameters and template functions with all template parameters known.
	for( const FunctionTemplatePtr& function_template : candidate_functions->template_functions )
	{
		if( const auto func= FinishTemplateFunctionParameterization( block_names.GetErrors(), initializer_expression_src_loc, function_template ) )
		{
			if( func->type == function_pointer_type.function_type )
			{
				if( exact_match_function_variable != nullptr )
				{
					// Error, exists more,then one non-exact match function.
					// TODO - maybe generate separate error?
					REPORT_ERROR( TooManySuitableOverloadedFunctions, block_names.GetErrors(), initializer_expression_src_loc, FunctionParamsToString(function_pointer_type.function_type.params) );
					return nullptr;
				}
				exact_match_function_variable= func;
			}
			else if( func->type.PointerCanBeConvertedTo( function_pointer_type.function_type ) )
				convertible_function_variables.push_back(func);
		}
	}

	const FunctionVariable* function_variable= exact_match_function_variable;
	if( function_variable == nullptr )
	{
		if( convertible_function_variables.size() > 1u )
		{
			// Error, exist more, then one non-exact match function.
			// TODO - maybe generate separate error?
			REPORT_ERROR( TooManySuitableOverloadedFunctions, block_names.GetErrors(), initializer_expression_src_loc, FunctionParamsToString(function_pointer_type.function_type.params) );
			return nullptr;
		}
		else if( !convertible_function_variables.empty() )
			function_variable= convertible_function_variables.front();
	}
	if( function_variable == nullptr )
	{
		REPORT_ERROR( CouldNotSelectOverloadedFunction, block_names.GetErrors(), initializer_expression_src_loc, FunctionParamsToString(function_pointer_type.function_type.params) );
		return nullptr;
	}
	if( function_variable->is_deleted )
		REPORT_ERROR( AccessingDeletedMethod, block_names.GetErrors(), initializer_expression_src_loc );

	{
		SrcLoc value_src_loc;
		if( const auto template_parameterization= std::get_if< std::unique_ptr< const Synt::TemplateParameterization > >( &initializer_expression ) )
			value_src_loc= Synt::GetComplexNameSrcLoc( (*template_parameterization)->base );
		else
			value_src_loc= initializer_expression_src_loc;
		CollectFunctionDefinition( *function_variable, value_src_loc );
	}

	llvm::Value* function_value= EnsureLLVMFunctionCreated( *function_variable );
	if( function_variable->type != function_pointer_type.function_type )
		function_value= function_context.llvm_ir_builder.CreatePointerCast( function_value, variable->type.GetLLVMType() );

	function_variable->referenced= true;

	CreateTypedStore( function_context, variable->type, function_value, variable->llvm_value );
	return EnsureLLVMFunctionCreated( *function_variable );
}

llvm::Constant* CodeBuilder::InitializeClassFieldWithInClassIninitalizer(
	const VariablePtr& field_variable,
	const ClassField& class_field,
	FunctionContext& function_context )
{
	U_ASSERT( class_field.syntax_element->initializer != nullptr );
	U_ASSERT( !class_field.is_reference );

	// Reset "this" for function context.
	// TODO - maybe reset also other function context fields?
	VariablePtr prev_this= function_context.this_;
	function_context.this_= nullptr;

	llvm::Constant* const result=
		ApplyInitializer(
			field_variable,
			*class_field.class_->members_initial, // Use initial class members names scope.
			function_context,
			*class_field.syntax_element->initializer );

	function_context.this_= std::move(prev_this);

	return result;
}

llvm::Constant* CodeBuilder::InitializeReferenceClassFieldWithInClassIninitalizer(
	const VariablePtr variable,
	const ClassField& class_field,
	FunctionContext& function_context )
{
	U_ASSERT( class_field.syntax_element->initializer != nullptr );
	U_ASSERT( class_field.is_reference );

	// Reset "this" for function context.
	// TODO - maybe reset also other function context fields?
	VariablePtr prev_this= function_context.this_;
	function_context.this_= nullptr;

	llvm::Constant* const result=
		InitializeReferenceField(
			variable,
			class_field,
			*class_field.syntax_element->initializer,
			*class_field.class_->members_initial, // Use initial class members names scope.
			function_context );

	function_context.this_= std::move(prev_this);

	return result;
}

void CodeBuilder::CheckClassFieldsInitializers( const ClassPtr class_type )
{
	// Run code generation for initializers.
	// We must check it, becauseinitializers may not be executed later.

	const Class& class_= *class_type;
	U_ASSERT( class_.is_complete );

	FunctionContext& function_context= *global_function_context_;
	const StackVariablesStorage dummy_stack_variables_storage( function_context );

	for( const ClassFieldPtr& field : class_.fields_order )
	{
		if( field == nullptr )
			continue;

		if( field->syntax_element == nullptr || field->syntax_element->initializer == nullptr )
			continue;

		if( field->is_reference )
		{
			const VariablePtr this_variable=
				Variable::Create(
					class_type,
					ValueType::ReferenceMut,
					Variable::Location::Pointer,
					field->GetName() );
			function_context.variables_state.AddNode( this_variable );

			InitializeReferenceClassFieldWithInClassIninitalizer( this_variable, *field, function_context );
			function_context.variables_state.RemoveNode( this_variable );
		}
		else
		{
			const VariablePtr field_variable=
				Variable::Create(
					field->type,
					ValueType::ReferenceMut,
					Variable::Location::Pointer,
					field->GetName() );
			function_context.variables_state.AddNode( field_variable );
			InitializeClassFieldWithInClassIninitalizer( field_variable, *field, function_context );
			function_context.variables_state.RemoveNode( field_variable );
		}
	}
}

} // namespace U
