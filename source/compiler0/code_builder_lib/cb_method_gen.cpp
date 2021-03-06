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

void CodeBuilder::TryGenerateDefaultConstructor( const ClassPtr& class_type )
{
	Class& the_class= *class_type;

	// Search for explicit default constructor.
	FunctionVariable* constructor_variable= nullptr;
	if( Value* const constructors_value=
		the_class.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
	{
		OverloadedFunctionsSet* const constructors= constructors_value->GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( FunctionVariable& constructor : constructors->functions )
		{
			if( IsDefaultConstructor( *constructor.type.GetFunctionType(), class_type ) )
			{
				if( constructor.is_generated )
				{
					U_ASSERT(!constructor.have_body);
					constructor_variable= &constructor;
				}
				else
				{
					if( !constructor.is_deleted )
						the_class.is_default_constructible= true;
					return;
				}
			}
		};
	}

	// Generating of default constructor disabled, if class have other explicit constructors, except copy constructors.
	if( the_class.have_explicit_noncopy_constructors && constructor_variable == nullptr )
		return;

	// Generate default constructor, if all fields is default constructible.
	bool all_fields_is_default_constructible= true;

	for( const std::string& field_name : the_class.fields_order )
	{
		if( field_name.empty() )
			continue;

		const ClassField& field= *the_class.members->GetThisScopeValue( field_name )->GetClassField();

		if( field.syntax_element->initializer == nullptr &&
			( field.is_reference || !field.type.IsDefaultConstructible() ) )
			all_fields_is_default_constructible= false;
	}

	if( the_class.base_class != nullptr && !the_class.base_class->is_default_constructible )
		all_fields_is_default_constructible= false;

	if( !all_fields_is_default_constructible )
	{
		if( constructor_variable != nullptr )
			REPORT_ERROR( MethodBodyGenerationFailed, the_class.members->GetErrors(), constructor_variable->prototype_src_loc );
		return;
	}

	if( constructor_variable == nullptr )
	{
		// Generate function
		FunctionType constructor_type;
		constructor_type.return_type= void_type_;
		constructor_type.params.emplace_back();
		constructor_type.params.back().type= class_type;
		constructor_type.params.back().is_mutable= true;
		constructor_type.params.back().is_reference= true;

		constructor_type.llvm_type=
			llvm::FunctionType::get(
				fundamental_llvm_types_.void_for_ret,
				{ the_class.llvm_type->getPointerTo() },
				false );

		llvm::Function* const llvm_constructor_function=
			llvm::Function::Create(
				constructor_type.llvm_type,
				llvm::Function::LinkageTypes::ExternalLinkage,
				mangler_.MangleFunction( *the_class.members, Keyword( Keywords::constructor_ ), constructor_type ),
				module_.get() );

		FunctionVariable new_constructor_variable;
		new_constructor_variable.type= std::move( constructor_type );
		new_constructor_variable.llvm_function= llvm_constructor_function;

		// Add generated constructor
		if( Value* const constructors_value= the_class.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
		{
			OverloadedFunctionsSet* const constructors= constructors_value->GetFunctionsSet();
			U_ASSERT( constructors != nullptr );
			constructors->functions.push_back( std::move( new_constructor_variable ) );
			constructor_variable= &constructors->functions.back();
		}
		else
		{
			OverloadedFunctionsSet constructors;
			constructors.functions.push_back( std::move( new_constructor_variable ) );
			Value* const inserted_value= the_class.members->AddName( Keyword( Keywords::constructor_ ), std::move( constructors ) );
			constructor_variable= &inserted_value->GetFunctionsSet()->functions.back();

		}
	}
	SetupGeneratedFunctionAttributes( *constructor_variable->llvm_function );
	constructor_variable->llvm_function->addAttribute( 1u, llvm::Attribute::NonNull );
	constructor_variable->llvm_function->addAttribute( 1u, llvm::Attribute::NoAlias );

	constructor_variable->have_body= true;
	constructor_variable->is_this_call= true;
	constructor_variable->is_generated= true;
	constructor_variable->is_constructor= true;
	constructor_variable->constexpr_kind= the_class.can_be_constexpr ? FunctionVariable::ConstexprKind::ConstexprComplete : FunctionVariable::ConstexprKind::NonConstexpr;

	FunctionContext function_context(
		*constructor_variable->type.GetFunctionType(),
		void_type_,
		llvm_context_,
		constructor_variable->llvm_function );
	StackVariablesStorage function_variables_storage( function_context );
	llvm::Value* const this_llvm_value= constructor_variable->llvm_function->args().begin();
	this_llvm_value->setName( Keyword( Keywords::this_ ) );

	if( the_class.base_class != nullptr )
	{
		Variable base_variable;
		base_variable.type= the_class.base_class;
		base_variable.value_type= ValueType::Reference;

		base_variable.llvm_value=
			function_context.llvm_ir_builder.CreateGEP(
				this_llvm_value,
				{ GetZeroGEPIndex(), GetFieldGEPIndex( 0u /*base class is allways first field */ ) } );

		ApplyEmptyInitializer( Keyword( Keywords::base_ ), SrcLoc()/*TODO*/, base_variable, *the_class.members, function_context );
	}

	for( const std::string& field_name : the_class.fields_order )
	{
		if( field_name.empty() )
			continue;

		const ClassField& field= *the_class.members->GetThisScopeValue( field_name )->GetClassField();

		if( field.is_reference )
		{
			U_ASSERT( field.syntax_element->initializer != nullptr ); // Can initialize reference field only with class field initializer.
			Variable variable;
			variable.type= class_type;
			variable.value_type= ValueType::Reference;
			variable.llvm_value= this_llvm_value;
			InitializeReferenceClassFieldWithInClassIninitalizer( variable, field, function_context );
		}
		else
		{
			Variable field_variable;
			field_variable.type= field.type;
			field_variable.value_type= ValueType::Reference;

			field_variable.llvm_value=
				function_context.llvm_ir_builder.CreateGEP( this_llvm_value, { GetZeroGEPIndex(), GetFieldGEPIndex( field.index ) } );

			if( field.syntax_element->initializer != nullptr )
				InitializeClassFieldWithInClassIninitalizer( field_variable, field, function_context );
			else
				ApplyEmptyInitializer( field_name, SrcLoc()/*TODO*/, field_variable, *the_class.members, function_context );
		}
	}

	SetupVirtualTablePointers( this_llvm_value, the_class, function_context );

	function_context.llvm_ir_builder.CreateRetVoid();
	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	// After default constructor generation, class is default-constructible.
	the_class.is_default_constructible= true;
}

void CodeBuilder::TryGenerateCopyConstructor( const ClassPtr& class_type )
{
	Class& the_class= *class_type;

	// Search for explicit copy constructor.
	FunctionVariable* constructor_variable= nullptr;
	if( Value* const constructors_value= the_class.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
	{
		OverloadedFunctionsSet* const constructors= constructors_value->GetFunctionsSet();
		U_ASSERT( constructors != nullptr );
		for( FunctionVariable& constructor : constructors->functions )
		{
			if( IsCopyConstructor( *constructor.type.GetFunctionType(), class_type ) )
			{
				if( constructor.is_generated )
				{
					U_ASSERT(!constructor.have_body);
					constructor_variable= &constructor;
				}
				else
				{
					if( !constructor.is_deleted )
						the_class.is_copy_constructible= true;
					return;
				}
			}
		}
	}

	if( constructor_variable == nullptr && the_class.kind != Class::Kind::Struct )
		return; // Do not generate copy-constructor for classes. Generate it only if "=default" explicitly specified for this method.

	bool all_fields_is_copy_constructible= true;

	for( const std::string& field_name : the_class.fields_order )
	{
		if( field_name.empty() )
			continue;

		const ClassField& field= *the_class.members->GetThisScopeValue( field_name )->GetClassField();

		if( !field.is_reference && !field.type.IsCopyConstructible() )
			all_fields_is_copy_constructible= false;
	}

	if( the_class.base_class != nullptr && !the_class.base_class->is_copy_constructible )
		all_fields_is_copy_constructible= false;

	if( !all_fields_is_copy_constructible )
	{
		if( constructor_variable != nullptr )
			REPORT_ERROR( MethodBodyGenerationFailed, the_class.members->GetErrors(), constructor_variable->prototype_src_loc );
		return;
	}

	if( constructor_variable == nullptr )
	{
		// Generate copy-constructor
		FunctionType constructor_type;
		constructor_type.return_type= void_type_;
		constructor_type.params.resize(2u);
		constructor_type.params[0].type= class_type;
		constructor_type.params[0].is_mutable= true;
		constructor_type.params[0].is_reference= true;
		constructor_type.params[1].type= class_type;
		constructor_type.params[1].is_mutable= false;
		constructor_type.params[1].is_reference= true;

		// Generate default reference pollution for copying.
		for( size_t i= 0u; i < Type(class_type).ReferencesTagsCount(); ++i )
		{
			FunctionType::ReferencePollution pollution;
			pollution.dst.first= 0u;
			pollution.dst.second= i;
			pollution.src.first= 1u;
			pollution.src.second= i;
			constructor_type.references_pollution.emplace(pollution);
		}

		constructor_type.llvm_type=
			llvm::FunctionType::get(
				fundamental_llvm_types_.void_for_ret,
				{ the_class.llvm_type->getPointerTo(), the_class.llvm_type->getPointerTo() },
				false );

		llvm::Function* const llvm_constructor_function=
			llvm::Function::Create(
				constructor_type.llvm_type,
				llvm::Function::LinkageTypes::ExternalLinkage,
				mangler_.MangleFunction( *the_class.members, Keyword( Keywords::constructor_ ), constructor_type ),
				module_.get() );

		// Add generated constructor
		FunctionVariable new_constructor_variable;
		new_constructor_variable.type= std::move( constructor_type );
		new_constructor_variable.llvm_function= llvm_constructor_function;

		if( Value* const constructors_value= the_class.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) ) )
		{
			OverloadedFunctionsSet* const constructors= constructors_value->GetFunctionsSet();
			U_ASSERT( constructors != nullptr );
			constructors->functions.push_back( std::move( new_constructor_variable ) );
			constructor_variable= &constructors->functions.back();
		}
		else
		{
			OverloadedFunctionsSet constructors;
			constructors.functions.push_back( std::move( new_constructor_variable ) );
			Value* const inserted_value= the_class.members->AddName( Keyword( Keywords::constructor_ ), std::move( constructors ) );
			constructor_variable= &inserted_value->GetFunctionsSet()->functions.back();
		}
	}
	SetupGeneratedFunctionAttributes( *constructor_variable->llvm_function );
	// Both args are "nonnull", "this" is "noalias".
	constructor_variable->llvm_function->addAttribute( 1u, llvm::Attribute::NonNull );
	constructor_variable->llvm_function->addAttribute( 1u, llvm::Attribute::NoAlias );
	constructor_variable->llvm_function->addAttribute( 2u, llvm::Attribute::NonNull );
	constructor_variable->llvm_function->addAttribute( 2u, llvm::Attribute::ReadOnly );

	constructor_variable->have_body= true;
	constructor_variable->is_this_call= true;
	constructor_variable->is_generated= true;
	constructor_variable->is_constructor= true;
	constructor_variable->constexpr_kind= the_class.can_be_constexpr ? FunctionVariable::ConstexprKind::ConstexprComplete : FunctionVariable::ConstexprKind::NonConstexpr;

	FunctionContext function_context(
		*constructor_variable->type.GetFunctionType(),
		void_type_,
		llvm_context_,
		constructor_variable->llvm_function );

	llvm::Value* const this_llvm_value= &*constructor_variable->llvm_function->args().begin();
	this_llvm_value->setName( Keyword( Keywords::this_ ) );
	llvm::Value* const src_llvm_value= &*std::next(constructor_variable->llvm_function->args().begin());
	src_llvm_value->setName( "src" );

	if( the_class.base_class != nullptr )
	{
		llvm::Value* const index_list[2] { GetZeroGEPIndex(),  GetFieldGEPIndex(  0u /*base class is allways first field */ ) };
		BuildCopyConstructorPart(
			function_context.llvm_ir_builder.CreateGEP( this_llvm_value, index_list ),
			function_context.llvm_ir_builder.CreateGEP( src_llvm_value , index_list ),
			the_class.base_class,
			function_context );
	}

	for( const std::string& field_name : the_class.fields_order )
	{
		if( field_name.empty() )
			continue;

		const ClassField& field= *the_class.members->GetThisScopeValue( field_name )->GetClassField();

		llvm::Value* const index_list[2] { GetZeroGEPIndex(), GetFieldGEPIndex( field.index ) };
		llvm::Value* const dst= function_context.llvm_ir_builder.CreateGEP( this_llvm_value, index_list );
		llvm::Value* const src= function_context.llvm_ir_builder.CreateGEP( src_llvm_value , index_list );

		if( field.is_reference )
		{
			// Create simple load-store for references.
			llvm::Value* const val= function_context.llvm_ir_builder.CreateLoad( src );
			function_context.llvm_ir_builder.CreateStore( val, dst );
		}
		else
		{
			U_ASSERT( field.type.IsCopyConstructible() );
			BuildCopyConstructorPart( dst, src, field.type, function_context );
		}
	}

	SetupVirtualTablePointers( this_llvm_value, the_class, function_context );

	function_context.llvm_ir_builder.CreateRetVoid();
	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );

	// After default constructor generation, class is copy-constructible.
	the_class.is_copy_constructible= true;
}

FunctionVariable CodeBuilder::GenerateDestructorPrototype( const ClassPtr& class_type )
{
	Class& the_class= *class_type;

	FunctionType destructor_type;
	destructor_type.return_type= void_type_;
	destructor_type.params.resize(1u);
	destructor_type.params[0].type= class_type;
	destructor_type.params[0].is_mutable= true;
	destructor_type.params[0].is_reference= true;

	llvm::Type* const this_llvm_type= the_class.llvm_type->getPointerTo();
	destructor_type.llvm_type=
		llvm::FunctionType::get(
			fundamental_llvm_types_.void_for_ret,
			llvm::ArrayRef<llvm::Type*>( &this_llvm_type, 1u ),
			false );

	FunctionVariable destructor_function;
	destructor_function.type= destructor_type;
	destructor_function.is_generated= true;
	destructor_function.is_this_call= true;
	destructor_function.have_body= false;

	destructor_function.llvm_function=
		llvm::Function::Create(
			destructor_type.llvm_type,
			llvm::Function::LinkageTypes::ExternalLinkage,
			mangler_.MangleFunction( *the_class.members, Keyword( Keywords::destructor_ ), destructor_type ),
			module_.get() );

	destructor_function.llvm_function->addAttribute( 1u, llvm::Attribute::NonNull );
	destructor_function.llvm_function->addAttribute( 1u, llvm::Attribute::NoAlias );

	return destructor_function;
}

void CodeBuilder::GenerateDestructorBody( const ClassPtr& class_type, FunctionVariable& destructor_function )
{
	Class& the_class= *class_type;
	const FunctionType& destructor_type= *destructor_function .type.GetFunctionType();

	llvm::Value* const this_llvm_value= &*destructor_function .llvm_function->args().begin();
	this_llvm_value->setName( Keyword( Keywords::this_ ) );

	Variable this_;
	this_.type= class_type;
	this_.location= Variable::Location::Pointer;
	this_.value_type= ValueType::Reference;
	this_.llvm_value= this_llvm_value;

	FunctionContext function_context(
		destructor_type,
		destructor_type.return_type,
		llvm_context_,
		destructor_function.llvm_function );
	function_context.this_= &this_;

	CallMembersDestructors( function_context, the_class.members->GetErrors(), the_class.body_src_loc );
	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );
	function_context.llvm_ir_builder.CreateRetVoid();

	SetupGeneratedFunctionAttributes( *destructor_function.llvm_function );

	destructor_function.have_body= true;
}

void CodeBuilder::TryGenerateDestructor( const ClassPtr& class_type )
{
	Class& the_class= *class_type;
	// Search for explicit destructor.
	if( Value* const destructor_value=
		the_class.members->GetThisScopeValue( Keyword( Keywords::destructor_ ) ) )
	{
		OverloadedFunctionsSet* const destructors= destructor_value->GetFunctionsSet();
		if( destructors->functions.empty() )
			return; // destructors may be invalid in case of error.

		FunctionVariable& destructor_function= destructors->functions.front();
		if( destructor_function.is_generated && !destructor_function.have_body )
			GenerateDestructorBody( class_type, destructor_function ); // Finish generating pre-generated destructor.

		the_class.have_destructor= true;
		return;
	}

	// SPRACHE_TODO - maybe not generate default destructor for classes, that have no fields with destructors?
	// SPRACHE_TODO - maybe mark generated destructor for this cases as "empty"?

	// Generate destructor.

	FunctionVariable destructor_variable= GenerateDestructorPrototype( class_type );
	GenerateDestructorBody( class_type, destructor_variable );

	// TODO - destructor have no overloads. Maybe store it as FunctionVariable, not as FunctionsSet?
	OverloadedFunctionsSet destructors;
	destructors.functions.push_back( std::move( destructor_variable ) );

	the_class.members->AddName( Keyword( Keywords::destructor_ ), Value( std::move( destructors ) ) );

	// Say "we have destructor".
	the_class.have_destructor= true;
}

void CodeBuilder::TryGenerateCopyAssignmentOperator( const ClassPtr& class_type )
{
	Class& the_class= *class_type;
	const std::string op_name= OverloadedOperatorToString( OverloadedOperator::Assign );

	// Search for explicit assignment operator.
	FunctionVariable* operator_variable= nullptr;
	if( Value* const assignment_operator_value= the_class.members->GetThisScopeValue( op_name ) )
	{
		OverloadedFunctionsSet* const operators= assignment_operator_value->GetFunctionsSet();
		for( FunctionVariable& op : operators->functions )
		{
			// SPRACHE_TODO - support assignment operator with value src argument.
			if( IsCopyAssignmentOperator( *op.type.GetFunctionType(), class_type ) )
			{
				if( op.is_generated )
				{
					U_ASSERT( !op.have_body );
					operator_variable= &op;
				}
				else
				{
					if( !op.is_deleted )
						the_class.is_copy_assignable= true;
					return;
				}
			}
		}
	}

	if( operator_variable == nullptr && the_class.kind != Class::Kind::Struct )
		return; // Do not generate copy-assignement operator for classes. Generate it only if "=default" explicitly specified for this method.

	bool all_fields_is_copy_assignable= true;

	for( const std::string& field_name : the_class.fields_order )
	{
		if( field_name.empty() )
			continue;

		const ClassField& field= *the_class.members->GetThisScopeValue( field_name )->GetClassField();

		// We can not generate assignment operator for classes with references, for classes with immutable fields, for classes with noncopyable fields.
		if( field.is_reference || !field.type.IsCopyAssignable() || !field.is_mutable )
			all_fields_is_copy_assignable= false;
	}

	if( the_class.base_class != nullptr && !the_class.base_class->is_copy_assignable )
		all_fields_is_copy_assignable= false;

	if( !all_fields_is_copy_assignable )
	{
		if( operator_variable != nullptr )
			REPORT_ERROR( MethodBodyGenerationFailed, the_class.members->GetErrors(), operator_variable->prototype_src_loc );
		return;
	}

	if( operator_variable == nullptr )
	{
		// Generate assignment operator
		FunctionType op_type;
		op_type.return_type= void_type_;
		op_type.params.resize(2u);
		op_type.params[0].type= class_type;
		op_type.params[0].is_mutable= true;
		op_type.params[0].is_reference= true;
		op_type.params[1].type= class_type;
		op_type.params[1].is_mutable= false;
		op_type.params[1].is_reference= true;

		// Generate default reference pollution for copying.
		for( size_t i= 0u; i < Type(class_type).ReferencesTagsCount(); ++i )
		{
			FunctionType::ReferencePollution pollution;
			pollution.dst.first= 0u;
			pollution.dst.second= i;
			pollution.src.first= 1u;
			pollution.src.second= i;
			op_type.references_pollution.emplace(pollution);
		}

		op_type.llvm_type=
			llvm::FunctionType::get(
				fundamental_llvm_types_.void_for_ret,
				{ the_class.llvm_type->getPointerTo(), the_class.llvm_type->getPointerTo() },
				false );

		llvm::Function* const llvm_op_function=
			llvm::Function::Create(
				op_type.llvm_type,
				llvm::Function::LinkageTypes::ExternalLinkage,
				mangler_.MangleFunction( *the_class.members, op_name, op_type ),
				module_.get() );

		// Add generated assignment operator
		FunctionVariable new_op_variable;
		new_op_variable.type= std::move( op_type );
		new_op_variable.llvm_function= llvm_op_function;

		if( Value* const operators_value= the_class.members->GetThisScopeValue( op_name ) )
		{
			OverloadedFunctionsSet* const operators= operators_value->GetFunctionsSet();
			U_ASSERT( operators != nullptr );
			operators->functions.push_back( std::move( new_op_variable ) );
			operator_variable= &operators->functions.back();
		}
		else
		{
			OverloadedFunctionsSet operators;
			operators.functions.push_back( std::move( new_op_variable ) );
			Value* const inserted_value= the_class.members->AddName( op_name, std::move( operators ) );
			operator_variable= &inserted_value->GetFunctionsSet()->functions.back();
		}
	}
	SetupGeneratedFunctionAttributes( *operator_variable->llvm_function );
	// Both args are "nonnull", "this" is "noalias".
	operator_variable->llvm_function->addAttribute( 1u, llvm::Attribute::NonNull );
	operator_variable->llvm_function->addAttribute( 1u, llvm::Attribute::NoAlias );
	operator_variable->llvm_function->addAttribute( 2u, llvm::Attribute::NonNull );
	operator_variable->llvm_function->addAttribute( 2u, llvm::Attribute::ReadOnly );

	operator_variable->have_body= true;
	operator_variable->is_this_call= true;
	operator_variable->is_generated= true;
	operator_variable->constexpr_kind= the_class.can_be_constexpr ? FunctionVariable::ConstexprKind::ConstexprComplete : FunctionVariable::ConstexprKind::NonConstexpr;

	FunctionContext function_context(
		*operator_variable->type.GetFunctionType(),
		void_type_,
		llvm_context_,
		operator_variable->llvm_function );

	llvm::Value* const this_llvm_value= &*operator_variable->llvm_function->args().begin();
	this_llvm_value->setName( Keyword( Keywords::this_ ) );
	llvm::Value* const src_llvm_value= &*std::next(operator_variable->llvm_function->args().begin());
	src_llvm_value->setName( "src" );

	if( the_class.base_class != nullptr )
	{
		llvm::Value* const index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex(  0u /*base class is allways first field */ ) };
		BuildCopyAssignmentOperatorPart(
			function_context.llvm_ir_builder.CreateGEP( this_llvm_value, index_list ),
			function_context.llvm_ir_builder.CreateGEP( src_llvm_value , index_list ),
			the_class.base_class,
			function_context );
	}

	for( const std::string& field_name : the_class.fields_order )
	{
		if( field_name.empty() )
			continue;

		const ClassField& field= *the_class.members->GetThisScopeValue( field_name )->GetClassField();
		U_ASSERT( field.type.IsCopyAssignable() );

		llvm::Value* const index_list[2] { GetZeroGEPIndex(), GetFieldGEPIndex( field.index ) };
		BuildCopyAssignmentOperatorPart(
			function_context.llvm_ir_builder.CreateGEP( this_llvm_value, index_list ),
			function_context.llvm_ir_builder.CreateGEP( src_llvm_value , index_list ),
			field.type,
			function_context );
	}

	function_context.alloca_ir_builder.CreateBr( function_context.function_basic_block );
	function_context.llvm_ir_builder.CreateRetVoid();

	// After operator generation, class is copy-assignable.
	the_class.is_copy_assignable= true;
}

void CodeBuilder::BuildCopyConstructorPart(
	llvm::Value* const dst, llvm::Value* const src,
	const Type& type,
	FunctionContext& function_context )
{
	if( type.GetFundamentalType() != nullptr ||
		type.GetEnumType() != nullptr ||
		type.GetRawPointerType() != nullptr ||
		type.GetFunctionPointerType() != nullptr )
	{
		// Create simple load-store.
		if( type == void_type_ ){} // Do nothing for "void".
		else if( src->getType() == dst->getType() )
			function_context.llvm_ir_builder.CreateStore( function_context.llvm_ir_builder.CreateLoad( src ), dst );
		else if( src->getType() == dst->getType()->getPointerElementType() )
			function_context.llvm_ir_builder.CreateStore( src, dst );
		else U_ASSERT( false );
	}
	else if( const ArrayType* const array_type_ptr= type.GetArrayType() )
	{
		const ArrayType& array_type= *array_type_ptr;

		GenerateLoop(
			array_type.size,
			[&](llvm::Value* const counter_value)
			{
				llvm::Value* const index_list[2]{ GetZeroGEPIndex(), counter_value };
				BuildCopyConstructorPart(
					function_context.llvm_ir_builder.CreateGEP( dst, index_list ),
					function_context.llvm_ir_builder.CreateGEP( src, index_list ),
					array_type.type,
					function_context );
			},
			function_context);
	}
	else if( const TupleType* const tuple_type= type.GetTupleType() )
	{
		for( const Type& element_type : tuple_type->elements )
		{
			llvm::Value* const index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( size_t(&element_type - tuple_type->elements.data()) ) };
			BuildCopyConstructorPart(
				function_context.llvm_ir_builder.CreateGEP( dst, index_list ),
				function_context.llvm_ir_builder.CreateGEP( src, index_list ),
				element_type,
				function_context );
		}
	}
	else if( const ClassPtr class_type_ptr= type.GetClassType() )
	{
		const Class& class_type= *class_type_ptr;

		// Search copy constructor.
		const Value* constructor_value=
			class_type.members->GetThisScopeValue( Keyword( Keywords::constructor_ ) );
		U_ASSERT( constructor_value != nullptr );
		const OverloadedFunctionsSet* const constructors_set= constructor_value->GetFunctionsSet();
		U_ASSERT( constructors_set != nullptr );

		const FunctionVariable* constructor= nullptr;;
		for( const FunctionVariable& candidate_constructor : constructors_set->functions )
		{
			const FunctionType& constructor_type= *candidate_constructor.type.GetFunctionType();

			if( constructor_type.params.size() == 2u &&
				constructor_type.params.back().type == class_type_ptr && !constructor_type.params.back().is_mutable )
			{
				constructor= &candidate_constructor;
				break;
			}
		}
		U_ASSERT( constructor != nullptr );

		// Call it.
		function_context.llvm_ir_builder.CreateCall(constructor->llvm_function, { dst, src } );
	}
	else
		U_ASSERT(false);
}

void CodeBuilder::BuildCopyAssignmentOperatorPart(
	llvm::Value* const dst, llvm::Value* const src,
	const Type& type,
	FunctionContext& function_context )
{
	if( type.GetFundamentalType() != nullptr ||
		type.GetEnumType() != nullptr ||
		type.GetRawPointerType() != nullptr ||
		type.GetFunctionPointerType() != nullptr )
	{
		// Create simple load-store.
		if( type == void_type_ ){} // Do nothing for "void".
		else if( src->getType() == dst->getType() )
			function_context.llvm_ir_builder.CreateStore( function_context.llvm_ir_builder.CreateLoad( src ), dst );
		else if( src->getType() == dst->getType()->getPointerElementType() )
			function_context.llvm_ir_builder.CreateStore( src, dst );
		else U_ASSERT( false );
	}
	else if( const ArrayType* const array_type_ptr= type.GetArrayType() )
	{
		const ArrayType& array_type= *array_type_ptr;

		GenerateLoop(
			array_type.size,
			[&](llvm::Value* const counter_value)
			{
				llvm::Value* const index_list[2]{ GetZeroGEPIndex(), counter_value };
				BuildCopyAssignmentOperatorPart(
					function_context.llvm_ir_builder.CreateGEP( dst, index_list ),
					function_context.llvm_ir_builder.CreateGEP( src, index_list ),
					array_type.type,
					function_context );
			},
			function_context);
	}
	else if( const TupleType* const tuple_type= type.GetTupleType() )
	{
		for( const Type& element_type : tuple_type->elements )
		{
			llvm::Value* const index_list[2]{ GetZeroGEPIndex(), GetFieldGEPIndex( size_t(&element_type - tuple_type->elements.data()) ) };
			BuildCopyAssignmentOperatorPart(
				function_context.llvm_ir_builder.CreateGEP( dst, index_list ),
				function_context.llvm_ir_builder.CreateGEP( src, index_list ),
				element_type,
				function_context );
		}
	}
	else if( const ClassPtr class_type_ptr= type.GetClassType() )
	{
		const Class& class_type= *class_type_ptr;

		// Search copy-assignment aoperator.
		const Value* op_value=
			class_type.members->GetThisScopeValue( "=" );
		U_ASSERT( op_value != nullptr );
		const OverloadedFunctionsSet* const operators_set= op_value->GetFunctionsSet();
		U_ASSERT( operators_set != nullptr );

		const FunctionVariable* op= nullptr;;
		for( const FunctionVariable& candidate_op : operators_set->functions )
		{
			const FunctionType& op_type= *candidate_op .type.GetFunctionType();

			if( op_type.params[0u].type == type &&  op_type.params[0u].is_mutable && op_type.params[0u].is_reference &&
				op_type.params[1u].type == type && !op_type.params[1u].is_mutable && op_type.params[1u].is_reference )
			{
				op= &candidate_op;
				break;
			}
		}
		U_ASSERT( op != nullptr );

		// Call it.
		function_context.llvm_ir_builder.CreateCall( op->llvm_function, { dst, src } );
	}
	else
		U_ASSERT(false);
}

void CodeBuilder::CopyBytes(
	llvm::Value* const src, llvm::Value* const dst,
	const Type& type,
	FunctionContext& function_context )
{
	if( type == void_type_ )
		return; // Do nothing for "void" type.

	llvm::Type* const llvm_type= type.GetLLVMType();
	if( llvm_type->isIntegerTy() || llvm_type->isFloatingPointTy() || llvm_type->isPointerTy() )
	{
		// Create simple load-store.
		if( src->getType() == dst->getType() )
			function_context.llvm_ir_builder.CreateStore( function_context.llvm_ir_builder.CreateLoad( src ), dst );
		else if( src->getType() == dst->getType()->getPointerElementType() )
			function_context.llvm_ir_builder.CreateStore( src, dst );
		else U_ASSERT(false);
	}
	else
	{
		// Create memcpy for aggregate types.
		const auto alignment= data_layout_.getABITypeAlignment( llvm_type ); // TODO - is this right alignment?
		function_context.llvm_ir_builder.CreateMemCpy(
			dst, alignment,
			src, alignment,
			llvm::Constant::getIntegerValue( fundamental_llvm_types_.u32, llvm::APInt(32, data_layout_.getTypeAllocSize( llvm_type ) ) ) );
	}
}

void CodeBuilder::MoveConstantToMemory(
	llvm::Value* const ptr, llvm::Constant* const constant,
	FunctionContext& function_context )
{
	llvm::Value* index_list[2];
	index_list[0]= GetZeroGEPIndex();

	llvm::Type* const type= constant->getType();
	if( type->isStructTy() )
	{
		llvm::StructType* const struct_type= llvm::dyn_cast<llvm::StructType>(type);
		for( unsigned int i= 0u; i < struct_type->getNumElements(); ++i )
		{
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, i ) );
			MoveConstantToMemory(
				function_context.llvm_ir_builder.CreateGEP( ptr, index_list ),
				constant->getAggregateElement(i),
				function_context );
		}
	}
	else if( type->isArrayTy() )
	{
		llvm::ArrayType* const array_type= llvm::dyn_cast<llvm::ArrayType>(type);
		for( unsigned int i= 0u; i < array_type->getNumElements(); ++i )
		{
			index_list[1]= llvm::Constant::getIntegerValue( fundamental_llvm_types_.i32, llvm::APInt( 32u, i ) );
			MoveConstantToMemory(
				function_context.llvm_ir_builder.CreateGEP( ptr, index_list ),
				constant->getAggregateElement(i),
				function_context );
		}
	}
	else if( type->isIntegerTy() || type->isFloatingPointTy() )
		function_context.llvm_ir_builder.CreateStore( constant, ptr );
	else U_ASSERT(false);
}

bool CodeBuilder::IsDefaultConstructor( const FunctionType& function_type, const Type& base_class )
{
	return
		function_type.params.size() == 1u &&
		function_type.params[0].type == base_class &&  function_type.params[0].is_mutable && function_type.params[0].is_reference;
}

bool CodeBuilder::IsCopyConstructor( const FunctionType& function_type, const Type& base_class )
{
	return
		function_type.params.size() == 2u &&
		function_type.params[0].type == base_class &&  function_type.params[0].is_mutable && function_type.params[0].is_reference &&
		function_type.params[1].type == base_class && !function_type.params[1].is_mutable && function_type.params[1].is_reference;
}

bool CodeBuilder::IsCopyAssignmentOperator( const FunctionType& function_type, const Type& base_class )
{
	return
		function_type.params.size() == 2u &&
		function_type.params[0].type == base_class &&  function_type.params[0].is_mutable && function_type.params[0].is_reference &&
		function_type.params[1].type == base_class && !function_type.params[1].is_mutable && function_type.params[1].is_reference;
}

} //namespace U
