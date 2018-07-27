#include "assert.hpp"
#include "keywords.hpp"
#include "mangling.hpp"
#include "code_builder.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

static void AddAncestorsAccessRights_r( Class& the_class, const ClassProxyPtr& ancestor_class )
{
	the_class.members.AddAccessRightsFor( ancestor_class, ClassMemberVisibility::Protected );
	for( const ClassProxyPtr& parent : ancestor_class->class_->parents )
		AddAncestorsAccessRights_r( the_class, parent );
}

//
// CodeBuilder
//

bool CodeBuilder::EnsureTypeCompleteness( const Type& type, const TypeCompleteness completeness )
{
	if( completeness == TypeCompleteness::Incomplete )
		return true;

	if( const auto fundamental_type= type.GetFundamentalType() )
	{
		if( completeness > TypeCompleteness::Incomplete )
			return fundamental_type->fundamental_type != U_FundamentalType::Void;
		return true;
	}
	else if( type.GetFunctionPointerType() != nullptr )
		return true;
	else if( const auto enum_type= type.GetEnumTypePtr() )
	{
		NamesScopeBuildEnum( enum_type, completeness );
		return true;
	}
	else if( const auto array_type= type.GetArrayType() )
		return EnsureTypeCompleteness( array_type->type, completeness );
	else if( const auto class_type= type.GetClassTypeProxy() )
	{
		NamesScopeBuildClass( class_type, completeness );
		return class_type->class_->completeness >= completeness; // Return true if we achived required completeness.
	}
	else U_ASSERT(false);

	return false;
}

void CodeBuilder::NamesScopeBuild( NamesScope& names_scope )
{
	names_scope.ForEachInThisScope(
		[&]( const NamesScope::InsertedName& name_imut )
		{
			NamesScope::InsertedName& name= const_cast<NamesScope::InsertedName&>(name_imut); // TODO - remove const_cast

			if( const NamesScopePtr inner_namespace= name.second.GetNamespace() )
				NamesScopeBuild( *inner_namespace );
			else if( OverloadedFunctionsSet* const functions_set= name.second.GetFunctionsSet() )
				NamesScopeBuildFunctionsSet( names_scope, *functions_set, true );
			else if( const Type* const type= name.second.GetTypeName() )
			{
				if( type->GetFundamentalType() != nullptr ||
					type->GetFunctionPointerType() != nullptr ||
					type->GetArrayType() != nullptr )
				{}
				else if( const ClassProxyPtr class_type= type->GetClassTypeProxy() )
				{
					NamesScopeBuildClass( class_type, TypeCompleteness::Complete );
					NamesScopeBuild( class_type->class_->members );
				}
				else if( const EnumPtr enum_type= type->GetEnumTypePtr() )
					NamesScopeBuildEnum( enum_type, TypeCompleteness::Complete );
				else U_ASSERT(false);
			}
			else if( const auto type_templates_set= name.second.GetTypeTemplatesSet() )
				NamesScopeBuildTypetemplatesSet( names_scope, *type_templates_set );
			else if( name.second.GetClassField() != nullptr ) {} // Can be in classes.
			else if( name.second.GetFunctionVariable() != nullptr ) {} // It is function, generating from template.
			else if( name.second.GetVariable() != nullptr ){}
			else if( name.second.GetStoredVariable() != nullptr ){}
			else if( name.second.GetErrorValue() != nullptr ){}
			else if( TypeTemplatesSet* const type_templates_set= name.second.GetTypeTemplatesSet() )
				NamesScopeBuildTypetemplatesSet( names_scope, *type_templates_set );
			else if( const auto static_assert_= name.second.GetStaticAssert() )
				BuildStaticAssert( *static_assert_, names_scope );
			else if( name.second.GetTypedef() != nullptr )
				NamesScopeBuildTypedef( names_scope, name.second );
			else if( name.second.GetIncompleteGlobalVariable() != nullptr )
				NamesScopeBuildGlobalVariable( names_scope, name.second );
			else U_ASSERT(false);
		});
}

void CodeBuilder::NamesScopeBuildFunctionsSet( NamesScope& names_scope, OverloadedFunctionsSet& functions_set, const bool build_body )
{
	if( functions_set.is_incomplete )
	{
		for( const Synt::Function* const function : functions_set.syntax_elements )
			NamesScopeBuildFunction( names_scope, functions_set.base_class, functions_set, *function, false );
		for( const Synt::Function* const function : functions_set.out_of_line_syntax_elements )
			NamesScopeBuildFunction( names_scope, functions_set.base_class, functions_set, *function, true );
		for( const Synt::FunctionTemplate* const function_template : functions_set.template_syntax_elements )
			PrepareFunctionTemplate( *function_template, functions_set, names_scope, functions_set.base_class );
		functions_set.is_incomplete= false;
	}

	if( build_body )
	{
		for( FunctionVariable& function_variable : functions_set.functions )
		{
			if( function_variable.syntax_element != nullptr && function_variable.syntax_element->block_ != nullptr &&
				!function_variable.have_body )
			{
				BuildFuncCode(
					function_variable,
					functions_set.base_class,
					names_scope,
					function_variable.syntax_element->name_.components.back().name,
					function_variable.syntax_element->type_.arguments_,
					function_variable.syntax_element->block_.get(),
					function_variable.syntax_element->constructor_initialization_list_.get() );
			}
		}
	}
	else if( functions_set.base_class == nullptr )
	{
		// Immediately build constexpr functions.
		for( FunctionVariable& function_variable : functions_set.functions )
		{
			if( function_variable.syntax_element != nullptr && function_variable.syntax_element->block_ != nullptr &&
				!function_variable.have_body && function_variable.constexpr_kind != FunctionVariable::ConstexprKind::NonConstexpr )
			{
				BuildFuncCode(
					function_variable,
					functions_set.base_class,
					names_scope,
					function_variable.syntax_element->name_.components.back().name,
					function_variable.syntax_element->type_.arguments_,
					function_variable.syntax_element->block_.get(),
					function_variable.syntax_element->constructor_initialization_list_.get() );
			}
		}
	}
}

void CodeBuilder::NamesScopeBuildFunction(
	NamesScope& names_scope,
	const ClassProxyPtr base_class,
	OverloadedFunctionsSet& functions_set,
	const Synt::Function& func,
	const bool is_out_of_line_function )
{
	const ProgramString& func_name= func.name_.components.back().name;
	const bool is_constructor= func_name == Keywords::constructor_;
	const bool is_destructor= func_name == Keywords::destructor_;
	const bool is_special_method= is_constructor || is_destructor;

	if( !is_special_method && IsKeyword( func_name ) )
		errors_.push_back( ReportUsingKeywordAsName( func.file_pos_ ) );

	if( is_special_method && base_class == nullptr )
	{
		errors_.push_back( ReportConstructorOrDestructorOutsideClass( func.file_pos_ ) );
		return;
	}
	if( !is_constructor && func.constructor_initialization_list_ != nullptr )
	{
		errors_.push_back( ReportInitializationListInNonconstructor( func.constructor_initialization_list_->file_pos_ ) );
		return;
	}
	if( is_destructor && !func.type_.arguments_.empty() )
	{
		errors_.push_back( ReportExplicitArgumentsInDestructor( func.file_pos_ ) );
		return;
	}

	FunctionVariable func_variable;
	func_variable.type= Function();
	{ // Prepare function type
		Function& function_type= *func_variable.type.GetFunctionType();

		if( func.type_.return_type_ == nullptr )
			function_type.return_type= void_type_for_ret_;
		else
		{
			function_type.return_type= PrepareType( func.type_.return_type_, names_scope );
			if( function_type.return_type == invalid_type_ )
				return;
		}

		function_type.return_value_is_mutable= func.type_.return_value_mutability_modifier_ == MutabilityModifier::Mutable;
		function_type.return_value_is_reference= func.type_.return_value_reference_modifier_ == ReferenceModifier::Reference;

		// HACK. We have different llvm types for "void".
		// llvm::void used only for empty return value, for other purposes we use "i8" for Ü::void.
		if( !function_type.return_value_is_reference && function_type.return_type == void_type_ )
			function_type.return_type= void_type_for_ret_;

		if( !function_type.return_value_is_reference &&
			!( function_type.return_type.GetFundamentalType() != nullptr ||
			   function_type.return_type.GetClassType() != nullptr ||
			   function_type.return_type.GetEnumType() != nullptr ||
			   function_type.return_type.GetFunctionPointerType() != nullptr ) )
		{
			errors_.push_back( ReportNotImplemented( func.file_pos_, "return value types except fundamentals, enums, classes, function pointers" ) );
			return;
		}

		if( is_special_method && function_type.return_type != void_type_ )
			errors_.push_back( ReportConstructorAndDestructorMustReturnVoid( func.file_pos_ ) );

		ProcessFunctionReturnValueReferenceTags( func.type_, function_type );

		// Args.
		function_type.args.reserve( func.type_.arguments_.size() );

		// Generate "this" arg for constructors.
		if( is_special_method )
		{
			func_variable.is_this_call= true;

			function_type.args.emplace_back();
			Function::Arg& arg= function_type.args.back();
			arg.type= base_class;
			arg.is_reference= true;
			arg.is_mutable= true;
		}

		for( const Synt::FunctionArgumentPtr& arg : func.type_.arguments_ )
		{
			const bool is_this= arg == func.type_.arguments_.front() && arg->name_ == Keywords::this_;

			if( !is_this && IsKeyword( arg->name_ ) )
				errors_.push_back( ReportUsingKeywordAsName( arg->file_pos_ ) );

			if( is_this && is_destructor )
				errors_.push_back( ReportExplicitThisInDestructor( arg->file_pos_ ) );
			if( is_this && is_constructor )
			{
				// Explicit this for constructor.
				U_ASSERT( function_type.args.size() == 1u );
				ProcessFunctionArgReferencesTags( func.type_, function_type, *arg, function_type.args.back(), function_type.args.size() - 1u );
				continue;
			}

			function_type.args.emplace_back();
			Function::Arg& out_arg= function_type.args.back();

			if( is_this )
			{
				func_variable.is_this_call= true;
				if( base_class == nullptr )
				{
					errors_.push_back( ReportThisInNonclassFunction( func.file_pos_, func_name ) );
					return;
				}
				out_arg.type= base_class;
			}
			else
				out_arg.type= PrepareType( arg->type_, names_scope );

			out_arg.is_mutable= arg->mutability_modifier_ == MutabilityModifier::Mutable;
			out_arg.is_reference= is_this || arg->reference_modifier_ == ReferenceModifier::Reference;

			if( !out_arg.is_reference &&
				!( out_arg.type.GetFundamentalType() != nullptr ||
				   out_arg.type.GetClassType() != nullptr ||
				   out_arg.type.GetEnumType() != nullptr ||
				   out_arg.type.GetFunctionPointerType() != nullptr ) )
			{
				errors_.push_back( ReportNotImplemented( func.file_pos_, "parameters types except fundamentals, classes, enums, functionpointers" ) );
				return;
			}

			ProcessFunctionArgReferencesTags( func.type_, function_type, *arg, out_arg, function_type.args.size() - 1u );
		} // for arguments

		function_type.unsafe= func.type_.unsafe_;

		TryGenerateFunctionReturnReferencesMapping( func.type_, function_type );
		ProcessFunctionReferencesPollution( func, function_type, base_class );
		CheckOverloadedOperator( base_class, function_type, func.overloaded_operator_, func.file_pos_ );

	} // end prepare function type

	// Set constexpr.
	if( func.constexpr_ )
	{
		if( func.block_ == nullptr )
			errors_.push_back( ReportConstexprFunctionsMustHaveBody( func.file_pos_ ) );
		if( func.virtual_function_kind_ != Synt::VirtualFunctionKind::None )
			errors_.push_back( ReportConstexprFunctionCanNotBeVirtual( func.file_pos_ ) );

		func_variable.constexpr_kind= FunctionVariable::ConstexprKind::ConstexprIncomplete;
	}

	// Set virtual.
	if( func.virtual_function_kind_ != Synt::VirtualFunctionKind::None )
	{
		if( base_class == nullptr )
			errors_.push_back( ReportVirtualForNonclassFunction( func.file_pos_, func_name ) );
		if( !func_variable.is_this_call )
			errors_.push_back( ReportVirtualForNonThisCallFunction( func.file_pos_, func_name ) );
		if( is_constructor )
			errors_.push_back( ReportFunctionCanNotBeVirtual( func.file_pos_, func_name ) );
		if( base_class != nullptr && ( base_class->class_->kind == Class::Kind::Struct || base_class->class_->kind == Class::Kind::NonPolymorph ) )
			errors_.push_back( ReportVirtualForNonpolymorphClass( func.file_pos_, func_name ) );
		if( is_out_of_line_function )
			errors_.push_back( ReportVirtualForFunctionImplementation( func.file_pos_, func_name ) );

		func_variable.virtual_function_kind= func.virtual_function_kind_;
	}

	// Check "=default" / "=delete".
	if( func.body_kind != Synt::Function::BodyKind::None )
	{
		U_ASSERT( func.block_ == nullptr );
		const Function& function_type= *func_variable.type.GetFunctionType();

		bool invalid_func= false;
		if( base_class == nullptr )
			invalid_func= true;
		else if( is_constructor )
			invalid_func= !( IsDefaultConstructor( function_type, base_class ) || IsCopyConstructor( function_type, base_class ) );
		else if( func.overloaded_operator_ == OverloadedOperator::Assign )
			invalid_func= !IsCopyAssignmentOperator( function_type, base_class );
		else
			invalid_func= true;

		if( invalid_func )
			errors_.push_back( ReportInvalidMethodForBodyGeneration( func.file_pos_ ) );
		else
		{
			if( func.body_kind == Synt::Function::BodyKind::BodyGenerationRequired )
				func_variable.is_generated= true;
			else
				func_variable.is_deleted= true;
		}
	}

	if( FunctionVariable* const prev_function= GetFunctionWithSameType( *func_variable.type.GetFunctionType(), functions_set ) )
	{
			 if( prev_function->syntax_element->block_ == nullptr && func.block_ != nullptr )
		{ // Ok, body after prototype.
			prev_function->syntax_element= &func;
			prev_function->body_file_pos= func.file_pos_;
		}
		else if( prev_function->syntax_element->block_ != nullptr && func.block_ == nullptr )
		{ // Ok, prototype after body. Since order-independent resolving this is correct.
			prev_function->prototype_file_pos= func.file_pos_;
		}
		else if( prev_function->syntax_element->block_ == nullptr && func.block_ == nullptr )
			errors_.push_back( ReportFunctionPrototypeDuplication( func.file_pos_, func_name ) );
		else if( prev_function->syntax_element->block_ != nullptr && func.block_ != nullptr )
			errors_.push_back( ReportFunctionBodyDuplication( func.file_pos_, func_name ) );

		if( prev_function->is_this_call != func_variable.is_this_call )
			errors_.push_back( ReportThiscallMismatch( func.file_pos_, func_name ) );

		if( !is_out_of_line_function ) // Previous function must be out of line. Set virtual specifier for it.
			prev_function->virtual_function_kind= func.virtual_function_kind_;
		else
		{
			// TODO - produce error, if it out of line is body for virtual function.
		}
		// TODO - produce error, if function with prototype and implementation inside class have different virtual function kind.
	}
	else
	{
		if( is_out_of_line_function )
		{
			errors_.push_back( ReportFunctionDeclarationOutsideItsScope( func.file_pos_ ) );
			return;
		}
		const bool overloading_ok= ApplyOverloadedFunction( functions_set, func_variable, func.file_pos_ );
		if( !overloading_ok )
			return;

		FunctionVariable& inserted_func_variable= functions_set.functions.back();
		inserted_func_variable.body_file_pos= inserted_func_variable.prototype_file_pos= func.file_pos_;
		inserted_func_variable.syntax_element= &func;

		BuildFuncCode(
			inserted_func_variable,
			base_class,
			names_scope,
			func_name,
			func.type_.arguments_,
			nullptr,
			func.constructor_initialization_list_.get() );
	}
}

void CodeBuilder::NamesScopeBuildClass( const ClassProxyPtr class_type, const TypeCompleteness completeness )
{
	Class& the_class= *class_type->class_;

	if( completeness <= the_class.completeness ||
		completeness == TypeCompleteness::Incomplete ||
		the_class.syntax_element->is_forward_declaration_ )
		return;

	const Synt::Class& class_declaration= *the_class.syntax_element;
	const ProgramString& class_name= class_declaration.name_.components.back().name;

	if( completeness >= TypeCompleteness::ReferenceTagsComplete )
	{
		NamesScope& class_parent_namespace= const_cast<NamesScope&>(*the_class.members.GetParent()); // TODO - remove const cast
		for( const Synt::ComplexName& parent : class_declaration.parents_ )
		{
			const NamesScope::InsertedName* const parent_name= ResolveName( class_declaration.file_pos_, class_parent_namespace, parent );
			if( parent_name == nullptr )
			{
				errors_.push_back( ReportNameNotFound( class_declaration.file_pos_, parent ) );
				continue;
			}

			const Type* const type_name= parent_name->second.GetTypeName();
			if( type_name == nullptr )
			{
				errors_.push_back( ReportNameIsNotTypeName( class_declaration.file_pos_, parent_name->first ) );
				continue;
			}

			const ClassProxyPtr parent_class_proxy= type_name->GetClassTypeProxy();
			if( parent_class_proxy == nullptr )
			{
				errors_.push_back( ReportCanNotDeriveFromThisType( class_declaration.file_pos_, type_name->ToString() ) );
				continue;
			}
			if( !EnsureTypeCompleteness( *type_name, TypeCompleteness::Complete ) )
			{
				errors_.push_back( ReportUsingIncompleteType( class_declaration.file_pos_, type_name->ToString() ) );
				continue;
			}

			if( std::find( the_class.parents.begin(), the_class.parents.end(), parent_class_proxy ) != the_class.parents.end() )
			{
				errors_.push_back( ReportDuplicatedParentClass( class_declaration.file_pos_, type_name->ToString() ) );
				continue;
			}

			const auto parent_kind= parent_class_proxy->class_->kind;
			if( !( parent_kind == Class::Kind::Abstract || parent_kind == Class::Kind::Interface || parent_kind == Class::Kind::PolymorphNonFinal ) )
			{
				errors_.push_back( ReportCanNotDeriveFromThisType( class_declaration.file_pos_, type_name->ToString() ) );
				continue;
			}

			if( parent_kind != Class::Kind::Interface ) // not interface=base
			{
				if( the_class.base_class != nullptr )
				{
					errors_.push_back( ReportDuplicatedBaseClass( class_declaration.file_pos_, type_name->ToString() ) );
					continue;
				}
				the_class.base_class= parent_class_proxy;
			}

			the_class.parents.push_back( parent_class_proxy );
			AddAncestorsAccessRights_r( the_class, parent_class_proxy );
		} // for parents

		ProcessClassParentsVirtualTables( the_class );

		// Pre-mark class as polymorph. Later we know class kind exactly, now, we only needs to know, that is polymorph - for virtual functions preparation.
		if( class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Polymorph ||
			class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Interface ||
			class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Abstract ||
			!class_declaration.parents_.empty() )
			the_class.kind= Class::Kind::PolymorphNonFinal;
		else if( class_declaration.kind_attribute_ == Synt::ClassKindAttribute::Struct )
			the_class.kind= Class::Kind::Struct;
		else
			the_class.kind= Class::Kind::NonPolymorph;

		the_class.can_be_constexpr= the_class.kind == Class::Kind::Struct;

		the_class.members.ForEachInThisScope(
			[&]( const NamesScope::InsertedName& name_imut )
			{
				ClassField* const class_field= const_cast<ClassField*>(name_imut.second.GetClassField()); // TODO - remove const_cast
				if( class_field == nullptr )
					return;

				const Synt::ClassField& in_field= *class_field->syntax_element;

				class_field->class_= class_type;
				class_field->is_reference= in_field.reference_modifier == Synt::ReferenceModifier::Reference;
				class_field->type= PrepareType( class_field->syntax_element->type, the_class.members );

				if( !EnsureTypeCompleteness( class_field->type, class_field->is_reference ? TypeCompleteness::Incomplete : TypeCompleteness::Complete ) )
				{
					errors_.push_back( ReportUsingIncompleteType( in_field.file_pos_, class_field->type.ToString() ) );
					return;
				}

				if( class_field->is_reference ) // Reference-fields are immutable by default
					class_field->is_mutable= in_field.mutability_modifier == Synt::MutabilityModifier::Mutable;
				else // But value-fields are mutable by default
					class_field->is_mutable= in_field.mutability_modifier != Synt::MutabilityModifier::Immutable;

				// Disable constexpr, if field can not be constexpr, or if field is mutable reference.
				if( !class_field->type.CanBeConstexpr() || ( class_field->is_reference && class_field->is_mutable ) )
					the_class.can_be_constexpr= false;

				++the_class.field_count;
			} );

		// Count reference tags.
		// SPRACHE_TODO - allow user explicitly set tag count.
		the_class.members.ForEachInThisScope(
			[&]( const NamesScope::InsertedName& name )
			{
				const ClassField* const field= name.second.GetClassField();
				if( field != nullptr && ( field->is_reference || field->type.ReferencesTagsCount() != 0u ) )
					the_class.references_tags_count= 1u;
			});
		for( const ClassProxyPtr& parent_class : the_class.parents )
		{
			if( parent_class->class_->references_tags_count != 0u )
				the_class.references_tags_count= 1u;
		}

		the_class.completeness= TypeCompleteness::ReferenceTagsComplete;
	} // if reference tags completeness required

	if( completeness == TypeCompleteness::Complete )
	{
		// Fill llvm struct type fields
		std::vector<llvm::Type*> fields_llvm_types;
		for( const ClassProxyPtr& parent : the_class.parents )
		{
			const auto field_number= static_cast<unsigned int>(fields_llvm_types.size());
			if( parent == the_class.base_class )
				the_class.base_class_field_number= field_number;

			the_class.parents_fields_numbers.push_back( field_number );
			fields_llvm_types.emplace_back( parent->class_->llvm_type );
		}
		the_class.members.ForEachInThisScope(
			[&]( const NamesScope::InsertedName& name )
			{
				if( ClassField* const class_field= const_cast<ClassField*>(name.second.GetClassField()) ) // TODO - remove const cast
				{
					class_field->index= static_cast<unsigned int>(fields_llvm_types.size());
					if( class_field->is_reference )
						fields_llvm_types.emplace_back( llvm::PointerType::get( class_field->type.GetLLVMType(), 0u ) );
					else
						fields_llvm_types.emplace_back( class_field->type.GetLLVMType() );
				}
			});

		// Complete another body elements.
		the_class.members.ForEachInThisScope(
			[&]( const NamesScope::InsertedName& name )
			{
				if( const auto functions_set= const_cast<OverloadedFunctionsSet*>(name.second.GetFunctionsSet()) )
				{
					NamesScopeBuildFunctionsSet( the_class.members, *functions_set, false );

					PrepareFunctionResult prepare_function_result; // TODO - remove "PrepareFunctionResult"
					prepare_function_result.functions_set= functions_set;
					for( FunctionVariable& function : functions_set->functions )
						ProcessClassVirtualFunction( the_class, function );
				}
				else if( const auto* type= name.second.GetTypeName() )
				{
					U_UNUSED(type); // TODO
				}
				else if( const auto type_templates_set= name.second.GetTypeTemplatesSet() )
					NamesScopeBuildTypetemplatesSet( the_class.members, const_cast<TypeTemplatesSet&>(*type_templates_set) );
				else if( name.second.GetClassField() != nullptr ) {}
				else if( name.second.GetVariable() != nullptr ){}
				else if( name.second.GetStoredVariable() != nullptr ){}
				else if( name.second.GetErrorValue() != nullptr ){}
				else if( name.second.GetStaticAssert() != nullptr ){}
				else if( name.second.GetTypedef() != nullptr )
					NamesScopeBuildTypedef( the_class.members, const_cast<Value&>(name.second) );
				else if( name.second.GetIncompleteGlobalVariable() != nullptr )
					NamesScopeBuildGlobalVariable( the_class.members, const_cast<Value&>(name.second) );
				else U_ASSERT(false);
			});

		// Search for explicit noncopy constructors.
		if( const NamesScope::InsertedName* const constructors_name=
			the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
		{
			const OverloadedFunctionsSet* const constructors= constructors_name->second.GetFunctionsSet();
			U_ASSERT( constructors != nullptr );
			for( const FunctionVariable& constructor : constructors->functions )
			{
				const Function& constructor_type= *constructor.type.GetFunctionType();

				U_ASSERT( constructor_type.args.size() >= 1u && constructor_type.args.front().type == class_type );
				if( !( constructor_type.args.size() == 2u && constructor_type.args.back().type == class_type && !constructor_type.args.back().is_mutable ) )
				{
					the_class.have_explicit_noncopy_constructors= true;
					break;
				}
			};
		}

		// Disable constexpr possibility for structs with explicit destructors, non-default copy-assignment operators and non-default copy constructors.
		if( const NamesScope::InsertedName* const destructor_name=
			the_class.members.GetThisScopeName( Keyword( Keywords::destructor_ ) ) )
		{
			const OverloadedFunctionsSet* const destructors= destructor_name->second.GetFunctionsSet();
			// Destructors may be invalid in case of error.
			if( !destructors->functions.empty() && !destructors->functions[0].is_generated )
				the_class.can_be_constexpr= false;
		}
		if( const NamesScope::InsertedName* const constructor_name=
			the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) )
		{
			const OverloadedFunctionsSet* const constructors= constructor_name->second.GetFunctionsSet();
			U_ASSERT( constructors != nullptr );
			for( const FunctionVariable& constructor : constructors->functions )
			{
				if( IsCopyConstructor( *constructor.type.GetFunctionType(), class_type ) && !constructor.is_generated )
					the_class.can_be_constexpr= false;
			}
		}
		if( const NamesScope::InsertedName* const assignment_operator_name=
			the_class.members.GetThisScopeName( OverloadedOperatorToString( OverloadedOperator::Assign ) ) )
		{
			const OverloadedFunctionsSet* const operators= assignment_operator_name->second.GetFunctionsSet();
			U_ASSERT( operators != nullptr );
			for( const FunctionVariable& op : operators->functions )
			{
				if( IsCopyAssignmentOperator( *op.type.GetFunctionType(), class_type ) && !op.is_generated )
					the_class.can_be_constexpr= false;
			}
		}

		bool class_contains_pure_virtual_functions= false;
		for( Class::VirtualTableEntry& virtual_table_entry : the_class.virtual_table )
		{
			if( virtual_table_entry.is_pure )
			{
				class_contains_pure_virtual_functions= true;
				break;
			}
		}

		// Check given kind attribute and actual class properties.
		switch( class_declaration.kind_attribute_ )
		{
		case Synt::ClassKindAttribute::Struct:
			U_ASSERT( class_declaration.parents_.empty() );
			the_class.kind= Class::Kind::Struct;
			break;

		case Synt::ClassKindAttribute::Class: // Class without parents and without kind attribute is non-polymorph.
			if( the_class.parents.empty() )
				the_class.kind= Class::Kind::NonPolymorph;
			else
				the_class.kind= Class::Kind::PolymorphNonFinal;
			if( class_contains_pure_virtual_functions )
			{
				errors_.push_back( ReportClassContainsPureVirtualFunctions( class_declaration.file_pos_, class_name ) );
				the_class.kind= Class::Kind::Abstract;
			}
			break;

		case Synt::ClassKindAttribute::Final:
			if( the_class.parents.empty() )
				the_class.kind= Class::Kind::NonPolymorph;
			else
			{
				for( Class::VirtualTableEntry& virtual_table_entry : the_class.virtual_table )
					virtual_table_entry.is_final= true; // All virtual functions of final class is final.
				the_class.kind= Class::Kind::PolymorphFinal;
			}
			if( class_contains_pure_virtual_functions )
			{
				errors_.push_back( ReportClassContainsPureVirtualFunctions( class_declaration.file_pos_, class_name ) );
				the_class.kind= Class::Kind::Abstract;
			}
			break;

		case Synt::ClassKindAttribute::Polymorph:
			the_class.kind= Class::Kind::PolymorphNonFinal;
			if( class_contains_pure_virtual_functions )
			{
				errors_.push_back( ReportClassContainsPureVirtualFunctions( class_declaration.file_pos_, class_name ) );
				the_class.kind= Class::Kind::Abstract;
			}
			break;

		case Synt::ClassKindAttribute::Interface:
			if( the_class.field_count != 0u )
				errors_.push_back( ReportFieldsForInterfacesNotAllowed( class_declaration.file_pos_ ) );
			if( the_class.base_class != nullptr )
				errors_.push_back( ReportBaseClassForInterface( class_declaration.file_pos_ ) );
			if( the_class.members.GetThisScopeName( Keyword( Keywords::constructor_ ) ) != nullptr )
				errors_.push_back( ReportConstructorForInterface( class_declaration.file_pos_ ) );
			for( const Class::VirtualTableEntry& virtual_table_entry : the_class.virtual_table )
			{
				if( !virtual_table_entry.is_pure && virtual_table_entry.name != Keywords::destructor_ )
				{
					errors_.push_back( ReportNonPureVirtualFunctionInInterface( class_declaration.file_pos_, class_name ) );
					break;
				}
			}
			the_class.kind= Class::Kind::Interface;
			break;

		case Synt::ClassKindAttribute::Abstract:
			the_class.kind= Class::Kind::Abstract;
			break;
		};

		if( the_class.kind == Class::Kind::Interface ||
			the_class.kind == Class::Kind::Abstract ||
			the_class.kind == Class::Kind::PolymorphNonFinal ||
			the_class.kind == Class::Kind::PolymorphFinal )
			TryGenerateDestructorPrototypeForPolymorphClass( the_class, class_type );

		// Merge namespaces of parents into result class.
		for( const ClassProxyPtr& parent : the_class.parents )
		{
			parent->class_->members.ForEachInThisScope(
				[&]( const NamesScope::InsertedName& name )
				{
					if( parent->class_->GetMemberVisibility( name.first ) == ClassMemberVisibility::Private )
						return; // Do not inherit private members.

					NamesScope::InsertedName* const result_class_name= the_class.members.GetThisScopeName(name.first);

					if( const OverloadedFunctionsSet* const functions= name.second.GetFunctionsSet() )
					{
						// SPARCHE_TODO - maybe also skip additive-assignment operators?
						if( name.first == Keyword( Keywords::constructor_ ) ||
							name.first == Keyword( Keywords::destructor_ ) ||
							name.first == OverloadedOperatorToString( OverloadedOperator::Assign ) )
							return; // Did not inherit constructors, destructors, assignment operators.

						if( result_class_name != nullptr )
						{
							if( OverloadedFunctionsSet* const result_class_functions= result_class_name->second.GetFunctionsSet() )
							{
								if( the_class.GetMemberVisibility( name.first ) != parent->class_->GetMemberVisibility( name.first ) )
								{
									const auto& file_pos= result_class_functions->functions.empty() ? result_class_functions->template_functions.front()->file_pos : result_class_functions->functions.front().prototype_file_pos;
									errors_.push_back( ReportFunctionsVisibilityMismatch( file_pos, name.first ) );
								}

								// Merge function sets, if result class have functions set with given name.
								// TODO - merge function templates
								for( const FunctionVariable& parent_function : functions->functions )
								{
									bool overrides= false;
									for( FunctionVariable& result_class_function : result_class_functions->functions )
									{
										if( parent_function.type == result_class_function.type )
										{
											overrides= true; // Ok, result class function overrides parent clas function.
											break;
										}
									}
									if( !overrides )
										ApplyOverloadedFunction( *result_class_functions, parent_function, class_declaration.file_pos_ );
								} // for parent functions

								for( const FunctionTemplatePtr& function_template : functions->template_functions )
									result_class_functions->template_functions.push_back(function_template);
							}
						}
						else
						{
							// Result class have no functions with this name. Inherit all functions from parent calass.
							the_class.members.AddName( name.first, name.second );
						}
					}
					else
					{
						// Just override other kinds of symbols.
						if( result_class_name == nullptr )
							the_class.members.AddName( name.first, name.second );
					}
				});
		}

		PrepareClassVirtualTableType( the_class );
		if( the_class.virtual_table_llvm_type != nullptr )
		{
			the_class.virtual_table_field_number= static_cast<unsigned int>( fields_llvm_types.size() );
			fields_llvm_types.push_back( llvm::PointerType::get( the_class.virtual_table_llvm_type, 0u ) ); // TODO - maybe store virtual table pointer in base class?
		}

		the_class.llvm_type->setBody( fields_llvm_types );

		BuildClassVirtualTables( the_class, class_type );

		the_class.completeness= TypeCompleteness::Complete;

		TryGenerateDefaultConstructor( the_class, class_type );
		TryGenerateDestructor( the_class, class_type );
		TryGenerateCopyConstructor( the_class, class_type );
		TryGenerateCopyAssignmentOperator( the_class, class_type );

		// Immediately build constexpr functions.
		the_class.members.ForEachInThisScope(
			[&]( const NamesScope::InsertedName& name )
			{
				OverloadedFunctionsSet* const functions_set= const_cast<OverloadedFunctionsSet*>(name.second.GetFunctionsSet());
				if( functions_set == nullptr )
					return;

				for( FunctionVariable& function : functions_set->functions )
				{
					if( function.constexpr_kind != FunctionVariable::ConstexprKind::NonConstexpr &&
						function.syntax_element != nullptr )
						BuildFuncCode(
							function,
							class_type,
							the_class.members,
							name.first,
							function.syntax_element->type_.arguments_,
							function.syntax_element->block_.get(),
							function.syntax_element->constructor_initialization_list_.get() );
				}
			}); // for functions
	} // if full comleteness required
}

void CodeBuilder::NamesScopeBuildEnum( const EnumPtr& enum_, TypeCompleteness completeness )
{
	if( completeness < TypeCompleteness::Complete )
		return;
	if( !enum_->is_incomplete )
		return;

	// Default underlaying type is 32bit. TODO - maybe do it platform-dependent?
	enum_->underlaying_type= FundamentalType( U_FundamentalType::u32, fundamental_llvm_types_.u32 );

	const Synt::Enum& enum_decl= *enum_->syntax_element;
	NamesScope& names_scope= const_cast<NamesScope&>(*enum_->members.GetParent()); // TODO - remove const_cast

	if( !enum_decl.underlaying_type_name.components.empty() )
	{
		const NamesScope::InsertedName* const type_name= ResolveName( enum_decl.file_pos_, names_scope, enum_decl.underlaying_type_name );
		if( type_name == nullptr )
			errors_.push_back( ReportNameNotFound( enum_decl.file_pos_, enum_decl.underlaying_type_name ) );
		else
		{
			const Type* const type= type_name->second.GetTypeName();
			if( type == nullptr )
				errors_.push_back( ReportNameIsNotTypeName( enum_decl.file_pos_, enum_decl.underlaying_type_name.components.back().name ) );
			else
			{
				const FundamentalType* const fundamental_type= type->GetFundamentalType();
				if( fundamental_type == nullptr || !IsInteger( fundamental_type->fundamental_type ) )
				{
					// SPRACHE_TODO - maybe allow inheritance of enums?
					errors_.push_back( ReportTypesMismatch( enum_decl.file_pos_, "any integer type"_SpC, type->ToString() ) );
				}
				else
					enum_->underlaying_type= *fundamental_type;
			}
		}
	}

	for( const Synt::Enum::Member& in_member : enum_decl.members )
	{
		Variable var;

		var.type= enum_;
		var.location= Variable::Location::Pointer;
		var.value_type= ValueType::ConstReference;
		var.constexpr_value=
			llvm::Constant::getIntegerValue(
				enum_->underlaying_type.llvm_type,
				llvm::APInt( enum_->underlaying_type.llvm_type->getIntegerBitWidth(), enum_->element_count ) );
		var.llvm_value=
			CreateGlobalConstantVariable(
				var.type,
				MangleGlobalVariable( enum_->members, in_member.name ),
				var.constexpr_value );

		if( enum_->members.AddName( in_member.name, Value( var, in_member.file_pos ) ) == nullptr )
			errors_.push_back( ReportRedefinition( in_member.file_pos, in_member.name ) );

		++enum_->element_count;
	}

	{
		const SizeType max_value_plus_one=
			SizeType(1) << ( SizeType(enum_->underlaying_type.llvm_type->getIntegerBitWidth()) - ( IsSignedInteger( enum_->underlaying_type.fundamental_type ) ? 1u : 0u ) );
		const SizeType max_value= max_value_plus_one - 1u;

		if( enum_->element_count > max_value )
			errors_.push_back( ReportUnderlayingTypeForEnumIsTooSmall( enum_decl.file_pos_, enum_->element_count - 1u, max_value ) );
	}

	enum_->is_incomplete= false;
}

void CodeBuilder::NamesScopeBuildTypetemplatesSet( NamesScope& names_scope, TypeTemplatesSet& type_templates_set )
{
	if( !type_templates_set.is_incomplete )
		return;

	for( const auto syntax_element : type_templates_set.syntax_elements )
		PrepareTypeTemplate( *syntax_element, type_templates_set, names_scope );

	type_templates_set.is_incomplete= false;
}

void CodeBuilder::NamesScopeBuildTypedef( NamesScope& names_scope, Value& typedef_value )
{
	U_ASSERT( typedef_value.GetTypedef() != nullptr );
	const Synt::Typedef& syntax_element= *typedef_value.GetTypedef()->syntax_element;

	// Replace value in names map, when typedef is comlete.
	typedef_value= Value( PrepareType( syntax_element.value, names_scope ), syntax_element.file_pos_ );
}

void CodeBuilder::NamesScopeBuildGlobalVariable( NamesScope& names_scope, Value& global_variable_value )
{
	U_ASSERT( global_variable_value.GetIncompleteGlobalVariable() != nullptr );
	const IncompleteGlobalVariable incomplete_global_variable= *global_variable_value.GetIncompleteGlobalVariable();
	global_variable_value= ErrorValue();

	FunctionContext& function_context= *dummy_function_context_;
	const StackVariablesStorage dummy_stack( function_context );

	if( const auto variables_declaration= dynamic_cast<const Synt::VariablesDeclaration*>(incomplete_global_variable.syntax_element) )
	{
		const Synt::VariablesDeclaration::VariableEntry& variable_declaration= variables_declaration->variables[ incomplete_global_variable.element_index ];

		if( variable_declaration.mutability_modifier == Synt::MutabilityModifier::Mutable )
		{
			errors_.push_back( ReportGlobalVariableMustBeConstexpr( variable_declaration.file_pos, variable_declaration.name ) );
			return;
		}

		const Type type= PrepareType( variables_declaration->type, names_scope );
		// Report about incomplete type only for values, not references.
		if( !EnsureTypeCompleteness( type, variable_declaration.reference_modifier == ReferenceModifier::Reference ? TypeCompleteness::Incomplete : TypeCompleteness::Complete ) )
		{
			errors_.push_back( ReportUsingIncompleteType( variable_declaration.file_pos, type.ToString() ) );
			return;
		}

		if( !type.CanBeConstexpr() )
		{
			errors_.push_back( ReportInvalidTypeForConstantExpressionVariable( variable_declaration.file_pos ) );
			return;
		}

		// Destruction frame for temporary variables of initializer expression.
		const StackVariablesStorage temp_variables_storage( function_context );

		const StoredVariablePtr stored_variable=
			std::make_shared<StoredVariable>(
				variable_declaration.name,
				Variable(),
				variable_declaration.reference_modifier == ReferenceModifier::Reference ? StoredVariable::Kind::Reference : StoredVariable::Kind::Variable,
				true );
		function_context.stack_variables_stack[ function_context.stack_variables_stack.size() - 2u ]->RegisterVariable( stored_variable );

		Variable& variable= stored_variable->content;
		variable.type= type;
		variable.location= Variable::Location::Pointer;
		variable.value_type= ValueType::Reference;

		if( variable_declaration.reference_modifier == ReferenceModifier::None )
		{
			llvm::GlobalVariable* global_variable= nullptr;
			variable.llvm_value= global_variable= CreateGlobalConstantVariable( type, MangleGlobalVariable( names_scope, variable_declaration.name ) );

			variable.referenced_variables.insert( stored_variable );
			if( variable_declaration.initializer != nullptr )
				variable.constexpr_value= ApplyInitializer( variable, stored_variable, *variable_declaration.initializer, names_scope, function_context );
			else
				ApplyEmptyInitializer( variable_declaration.name, variable_declaration.file_pos, variable, function_context );
			variable.referenced_variables.erase( stored_variable );

			// Make immutable, if needed, only after initialization, because in initialization we need call constructors, which is mutable methods.
			variable.value_type= ValueType::ConstReference;

			for( const auto& referenced_variable_pair : function_context.variables_state.GetVariableReferences( stored_variable ) )
				CheckVariableReferences( *referenced_variable_pair.first, variable_declaration.file_pos );

			if( global_variable != nullptr && variable.constexpr_value != nullptr )
				global_variable->setInitializer( variable.constexpr_value );
		}
		else if( variable_declaration.reference_modifier == ReferenceModifier::Reference )
		{
			if( variable_declaration.initializer == nullptr )
			{
				errors_.push_back( ReportExpectedInitializer( variable_declaration.file_pos, variable_declaration.name ) );
				return;
			}

			variable.value_type= ValueType::ConstReference;

			const Synt::IExpressionComponent* initializer_expression= nullptr;
			if( const auto expression_initializer= dynamic_cast<const Synt::ExpressionInitializer*>( variable_declaration.initializer.get() ) )
				initializer_expression= expression_initializer->expression.get();
			else if( const auto constructor_initializer= dynamic_cast<const Synt::ConstructorInitializer*>( variable_declaration.initializer.get() ) )
			{
				if( constructor_initializer->call_operator.arguments_.size() != 1u )
				{
					errors_.push_back( ReportReferencesHaveConstructorsWithExactlyOneParameter( constructor_initializer->file_pos_ ) );
					return;
				}
				initializer_expression= constructor_initializer->call_operator.arguments_.front().get();
			}
			else
			{
				errors_.push_back( ReportUnsupportedInitializerForReference( variable_declaration.initializer->GetFilePos() ) );
				return;
			}

			const Value expression_result_value= BuildExpressionCode( *initializer_expression, names_scope, function_context );

			if( !expression_result_value.GetType().ReferenceIsConvertibleTo( variable.type ) )
			{
				errors_.push_back( ReportTypesMismatch( variable_declaration.file_pos, variable.type.ToString(), expression_result_value.GetType().ToString() ) );
				return;
			}
			const Variable& expression_result= *expression_result_value.GetVariable();

			if( expression_result.value_type == ValueType::Value )
			{
				errors_.push_back( ReportExpectedReferenceValue( variable_declaration.file_pos ) );
				return;
			}
			if( expression_result.value_type == ValueType::ConstReference && variable.value_type == ValueType::Reference )
			{
				errors_.push_back( ReportBindingConstReferenceToNonconstReference( variable_declaration.file_pos ) );
				return;
			}

			variable.referenced_variables= expression_result.referenced_variables;

			for( const StoredVariablePtr& referenced_variable : variable.referenced_variables )
				function_context.variables_state.AddPollution( stored_variable, referenced_variable, false );
			CheckReferencedVariables( stored_variable->content, variable_declaration.file_pos );

			// TODO - maybe make copy of varaible address in new llvm register?
			llvm::Value* result_ref= expression_result.llvm_value;
			if( variable.type != expression_result.type )
				result_ref= CreateReferenceCast( result_ref, expression_result.type, variable.type, function_context );
			variable.llvm_value= result_ref;
			variable.constexpr_value= expression_result.constexpr_value;
		}
		else U_ASSERT(false);

		if( variable.constexpr_value == nullptr )
		{
			errors_.push_back( ReportVariableInitializerIsNotConstantExpression( variable_declaration.file_pos ) );
			return;
		}

		// Do not call destructors, because global variables can be only constexpr and any constexpr type have trivial destructor.

		global_variable_value= Value( stored_variable, variable_declaration.file_pos );
	}
	else if( const auto auto_variable_declaration= dynamic_cast<const Synt::AutoVariableDeclaration*>(incomplete_global_variable.syntax_element) )
	{
		if( auto_variable_declaration->mutability_modifier == MutabilityModifier::Mutable )
		{
			errors_.push_back( ReportGlobalVariableMustBeConstexpr( auto_variable_declaration->file_pos_, auto_variable_declaration->name ) );
			return;
		}

		// Destruction frame for temporary variables of initializer expression.
		const StackVariablesStorage temp_variables_storage( function_context );

		const Value initializer_experrsion_value= BuildExpressionCode( *auto_variable_declaration->initializer_expression, names_scope, function_context );

		{ // Check expression type. Expression can have exotic types, such "Overloading functions set", "class name", etc.
			const Type& type= initializer_experrsion_value.GetType();
			const bool type_is_ok=
				type.GetFundamentalType() != nullptr ||
				type.GetArrayType() != nullptr ||
				type.GetClassType() != nullptr ||
				type.GetEnumType() != nullptr ||
				type.GetFunctionPointerType() != nullptr;
			if( !type_is_ok )
			{
				errors_.push_back( ReportInvalidTypeForAutoVariable( auto_variable_declaration->file_pos_, initializer_experrsion_value.GetType().ToString() ) );
				return;
			}
		}

		const Variable& initializer_experrsion= *initializer_experrsion_value.GetVariable();

		const StoredVariablePtr stored_variable=
			std::make_shared<StoredVariable>(
				auto_variable_declaration->name,
				Variable(),
				auto_variable_declaration->reference_modifier == ReferenceModifier::Reference ? StoredVariable::Kind::Reference : StoredVariable::Kind::Variable,
				true );
		function_context.stack_variables_stack[ function_context.stack_variables_stack.size() - 2u ]->RegisterVariable( stored_variable );

		Variable& variable= stored_variable->content;
		variable.type= initializer_experrsion.type;
		variable.value_type= ValueType::ConstReference;
		variable.location= Variable::Location::Pointer;

		// Report about incomplete type only for values, not references.
		if( !EnsureTypeCompleteness( variable.type, auto_variable_declaration->reference_modifier == ReferenceModifier::Reference ? TypeCompleteness::Incomplete : TypeCompleteness::Complete ) )
		{
			errors_.push_back( ReportUsingIncompleteType( auto_variable_declaration->file_pos_, variable.type.ToString() ) );
			return;
		}
		if( !variable.type.CanBeConstexpr() )
		{
			errors_.push_back( ReportInvalidTypeForConstantExpressionVariable( auto_variable_declaration->file_pos_ ) );
			return;
		}

		if( auto_variable_declaration->reference_modifier == ReferenceModifier::Reference )
		{
			if( initializer_experrsion.value_type == ValueType::Value )
			{
				errors_.push_back( ReportExpectedReferenceValue( auto_variable_declaration->file_pos_ ) );
				return;
			}
			if( initializer_experrsion.value_type == ValueType::ConstReference && variable.value_type != ValueType::ConstReference )
			{
				errors_.push_back( ReportBindingConstReferenceToNonconstReference( auto_variable_declaration->file_pos_ ) );
				return;
			}

			variable.referenced_variables= initializer_experrsion.referenced_variables;

			variable.llvm_value= initializer_experrsion.llvm_value;
			variable.constexpr_value= initializer_experrsion.constexpr_value;

			for( const StoredVariablePtr& referenced_variable : variable.referenced_variables )
				function_context.variables_state.AddPollution( stored_variable, referenced_variable, false );
			CheckReferencedVariables( variable, auto_variable_declaration->file_pos_ );
		}
		else if( auto_variable_declaration->reference_modifier == ReferenceModifier::None )
		{
			VariablesState::VariableReferences moved_variable_referenced_variables;

			llvm::GlobalVariable* const global_variable= CreateGlobalConstantVariable( variable.type, MangleGlobalVariable( names_scope, auto_variable_declaration->name ) );
			variable.llvm_value= global_variable;

			if( variable.type.GetFundamentalType() != nullptr || variable.type.GetEnumType() != nullptr || variable.type.GetFunctionPointerType() != nullptr )
			{
				llvm::Value* const value_for_assignment= CreateMoveToLLVMRegisterInstruction( initializer_experrsion, function_context );
				function_context.llvm_ir_builder.CreateStore( value_for_assignment, variable.llvm_value );
				variable.constexpr_value= initializer_experrsion.constexpr_value;
			}
			else if( const ClassProxyPtr class_type= variable.type.GetClassTypeProxy() )
			{
				U_ASSERT( class_type->class_->completeness == TypeCompleteness::Complete );
				if( initializer_experrsion.value_type == ValueType::Value )
				{
					U_ASSERT( initializer_experrsion.referenced_variables.size() == 1u );
					const StoredVariablePtr& variable_for_move= *initializer_experrsion.referenced_variables.begin();

					moved_variable_referenced_variables= function_context.variables_state.GetVariableReferences( variable_for_move );
					function_context.variables_state.Move( variable_for_move );

					CopyBytes( initializer_experrsion.llvm_value, variable.llvm_value, variable.type, function_context );
					variable.constexpr_value= initializer_experrsion.constexpr_value; // Move can preserve constexpr.
				}
				else
					TryCallCopyConstructor(
						auto_variable_declaration->file_pos_,
						variable.llvm_value, initializer_experrsion.llvm_value,
						variable.type.GetClassTypeProxy(),
						function_context );
			}
			else
			{
				errors_.push_back( ReportNotImplemented( auto_variable_declaration->file_pos_, "expression initialization for nonfundamental types" ) );
				return;
			}

			if( global_variable != nullptr && variable.constexpr_value != nullptr )
				global_variable->setInitializer( variable.constexpr_value );

			// Take references inside variables in initializer expression.
			for( const auto& ref : moved_variable_referenced_variables )
				function_context.variables_state.AddPollution( stored_variable, ref.first, ref.second.IsMutable() );
			for( const StoredVariablePtr& referenced_variable : initializer_experrsion.referenced_variables )
			{
				for( const auto& inner_variable_pair : function_context.variables_state.GetVariableReferences( referenced_variable ) )
				{
					const bool ok= function_context.variables_state.AddPollution( stored_variable, inner_variable_pair.first, inner_variable_pair.second.IsMutable() );
					if(!ok)
						errors_.push_back( ReportReferenceProtectionError( auto_variable_declaration->file_pos_, inner_variable_pair.first->name ) );
				}
			}
		}
		else U_ASSERT(false);

		if( variable.constexpr_value == nullptr )
		{
			errors_.push_back( ReportVariableInitializerIsNotConstantExpression( auto_variable_declaration->file_pos_ ) );
			return;
		}

		// Do not call destructors, because global variables can be only constexpr and any constexpr type have trivial destructor.

		global_variable_value= Value( stored_variable, auto_variable_declaration->file_pos_ );
	}
	else U_ASSERT(false);
}

} // namespace CodeBuilderPrivate

} // namespace U
