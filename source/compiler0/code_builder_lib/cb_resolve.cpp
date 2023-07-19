#include "../../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "error_reporting.hpp"

#include "code_builder.hpp"

namespace U
{

Value CodeBuilder::ResolveValue(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::ComplexName& complex_name )
{
	// Fast path for simple typeof.
	if( const auto typeof_type_name= std::get_if<Synt::TypeofTypeName>( &complex_name ) )
		return Value( PrepareTypeImpl( names_scope, function_context, *typeof_type_name ), typeof_type_name->src_loc_ );

	const ResolveValueInternalResult result= ResolveValueInternal( names_scope, function_context, complex_name );

	// Complete some things in resolve.
	if( result.space != nullptr && result.value != nullptr )
	{
		if( const OverloadedFunctionsSetPtr functions_set= result.value->GetFunctionsSet() )
			GlobalThingBuildFunctionsSet( *result.space, *functions_set, false );
		else if( TypeTemplatesSet* const type_templates_set= result.value->GetTypeTemplatesSet() )
			GlobalThingBuildTypeTemplatesSet( *result.space, *type_templates_set );
		else if( result.value->GetTypedef() != nullptr )
			GlobalThingBuildTypedef( *result.space, *result.value );
		else if( result.value->GetIncompleteGlobalVariable() != nullptr )
			GlobalThingBuildVariable( *result.space, *result.value );
		else if( const Type* const type= result.value->GetTypeName() )
		{
			if( const EnumPtr enum_= type->GetEnumType() )
				GlobalThingBuildEnum( enum_ );
		}
	}

	return result.value == nullptr ? ErrorValue() : *result.value;
}

CodeBuilder::ResolveValueInternalResult CodeBuilder::ResolveValueInternal(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::ComplexName& complex_name )
{
	return std::visit( [&]( const auto& el ) { return ResolveValueImpl( names_scope, function_context, el ); }, complex_name );
}

CodeBuilder::ResolveValueInternalResult CodeBuilder::ResolveValueImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::TypeofTypeName& typeof_type_name )
{
	const Type type= PrepareTypeImpl( names_scope, function_context, typeof_type_name );

	auto value_ptr= std::make_unique<Value>( type, typeof_type_name.src_loc_ );
	Value* const value= value_ptr.get();
	typeof_values_storage_.push_back(std::move(value_ptr));

	return ResolveValueInternalResult{ nullptr, value };
}

CodeBuilder::ResolveValueInternalResult CodeBuilder::ResolveValueImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::RootNamespaceNameLookup& root_namespace_lookup )
{
	(void)function_context;

	NamesScope* const last_space= names_scope.GetRoot();
	Value* const value= last_space->GetThisScopeValue( root_namespace_lookup.name );
	if( value == nullptr )
		REPORT_ERROR( NameNotFound, names_scope.GetErrors(), root_namespace_lookup.src_loc_, root_namespace_lookup.name );

	return ResolveValueInternalResult{ last_space, value };
}

CodeBuilder::ResolveValueInternalResult CodeBuilder::ResolveValueImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::NameLookup& name_lookup )
{
	(void)function_context;
	return LookupName( names_scope, name_lookup.name, name_lookup.src_loc_ );
}

CodeBuilder::ResolveValueInternalResult CodeBuilder::ResolveValueImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::NamesScopeNameFetch& names_scope_fetch )
{
	const ResolveValueInternalResult base= ResolveValueInternal( names_scope, function_context, *names_scope_fetch.base );
	if( base.value == nullptr )
		return ResolveValueInternalResult{ nullptr, nullptr };

	NamesScope* last_space= nullptr;
	Value* value= nullptr;

	// In case of typedef convert it to type before other checks.
	if( base.value->GetTypedef() != nullptr && base.space != nullptr )
		GlobalThingBuildTypedef( *base.space, *base.value );

	if( const NamesScopePtr inner_namespace= base.value->GetNamespace() )
	{
		last_space= inner_namespace.get();
		value= inner_namespace->GetThisScopeValue( names_scope_fetch.name );
	}
	else if( const Type* const type= base.value->GetTypeName() )
	{
		if( ClassPtr class_= type->GetClassType() )
		{
			const auto class_value= ResolveClassValue( class_, names_scope_fetch.name );
			if( names_scope.GetAccessFor( class_ ) < class_value.second )
				REPORT_ERROR( AccessingNonpublicClassMember, names_scope.GetErrors(), names_scope_fetch.src_loc_, names_scope_fetch.name, class_->members->GetThisNamespaceName() );

			if( ( names_scope_fetch.name == Keywords::constructor_ || names_scope_fetch.name == Keywords::destructor_ ) && !function_context.is_in_unsafe_block )
				REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names_scope.GetErrors(), names_scope_fetch.src_loc_, names_scope_fetch.name );

			last_space= class_->members.get();
			value= class_value.first;
		}
		else if( EnumPtr const enum_= type->GetEnumType() )
		{
			GlobalThingBuildEnum( enum_ );
			last_space= &enum_->members;
			value= enum_->members.GetThisScopeValue( names_scope_fetch.name );
		}
	}
	else if( base.value->GetTypeTemplatesSet() != nullptr )
		REPORT_ERROR( TemplateInstantiationRequired, names_scope.GetErrors(), names_scope_fetch.src_loc_, names_scope_fetch.name );

	if( value == nullptr )
		REPORT_ERROR( NameNotFound, names_scope.GetErrors(), names_scope_fetch.src_loc_, names_scope_fetch.name );

	return ResolveValueInternalResult{ last_space, value };
}

CodeBuilder::ResolveValueInternalResult CodeBuilder::ResolveValueImpl(
	NamesScope& names_scope,
	FunctionContext& function_context,
	const Synt::TemplateParametrization& template_parametrization )
{
	const ResolveValueInternalResult base= ResolveValueInternal( names_scope, function_context, *template_parametrization.base );
	if( base.space == nullptr && base.value == nullptr )
		return ResolveValueInternalResult{ nullptr, nullptr };

	NamesScope* last_space= nullptr;
	Value* value= nullptr;

	if( TypeTemplatesSet* const type_templates_set= base.value->GetTypeTemplatesSet() )
	{
		GlobalThingBuildTypeTemplatesSet( *base.space, *type_templates_set );
		value=
			GenTemplateType(
				template_parametrization.src_loc_,
				*type_templates_set,
				template_parametrization.template_args,
				names_scope,
				function_context );

		if( value != nullptr )
		{
			const Type* const type= value->GetTypeName();
			U_ASSERT( type != nullptr );
			if( Class* const class_= type->GetClassType() )
				last_space= class_->members.get();
		}
	}
	else if( const OverloadedFunctionsSetPtr functions_set= base.value->GetFunctionsSet() )
	{
		GlobalThingBuildFunctionsSet( *base.space, *functions_set, false );
		if( functions_set->template_functions.empty() )
		{
			REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), template_parametrization.src_loc_ );
		}
		else
		{
			value=
				ParametrizeFunctionTemplate(
					template_parametrization.src_loc_,
					*functions_set,
					template_parametrization.template_args,
					names_scope,
					function_context );
			if( value == nullptr )
				REPORT_ERROR( TemplateFunctionGenerationFailed, names_scope.GetErrors(), template_parametrization.src_loc_, "TODO - name" );
		}
	}
	else
		REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), template_parametrization.src_loc_ );

	return ResolveValueInternalResult{ last_space, value };
}

CodeBuilder::ResolveValueInternalResult CodeBuilder::LookupName( NamesScope& names_scope, const std::string& name, const SrcLoc& src_loc )
{
	NamesScope* last_space= &names_scope;
	Value* value= nullptr;

	do
	{
		if( const auto class_type= last_space->GetClass() )
		{
			const auto class_value= ResolveClassValue( class_type, name );
			value= class_value.first;
			if( names_scope.GetAccessFor( class_type ) < class_value.second )
				REPORT_ERROR( AccessingNonpublicClassMember, names_scope.GetErrors(), src_loc, name, last_space->GetThisNamespaceName() );
		}
		else
			value= last_space->GetThisScopeValue( name );

		if( value != nullptr )
			break;

		last_space= last_space->GetParent();
	} while( last_space != nullptr );

	if( value == nullptr )
		REPORT_ERROR( NameNotFound, names_scope.GetErrors(), src_loc, name );

	if( value != nullptr && value->GetYetNotDeducedTemplateArg() != nullptr )
			REPORT_ERROR( TemplateArgumentIsNotDeducedYet, names_scope.GetErrors(), src_loc, name );

	return ResolveValueInternalResult{ last_space, value };
}

std::pair<Value*, ClassMemberVisibility> CodeBuilder::ResolveClassValue( const ClassPtr class_type, const std::string& name )
{
	return ResolveClassValueImpl( class_type, name );
}

std::pair<Value*, ClassMemberVisibility> CodeBuilder::ResolveClassValueImpl( ClassPtr class_type, const std::string& name, const bool recursive_call )
{
	const bool is_special_method=
		name == Keyword( Keywords::constructor_ ) ||
		name == Keyword( Keywords::destructor_ ) ||
		name == OverloadedOperatorToString( OverloadedOperator::Assign ) ||
		name == OverloadedOperatorToString( OverloadedOperator::CompareEqual ) ||
		name == OverloadedOperatorToString( OverloadedOperator::CompareOrder );

	if( is_special_method )
	{
		// Special methods may be generated during class build. So, require complete type to access these methods.
		GlobalThingBuildClass( class_type ); // Functions set changed in this call.
	}

	if( const auto value= class_type->members->GetThisScopeValue( name ) )
	{
		const auto visibility= class_type->GetMemberVisibility( name );

		// We need to build some things right now (typedefs, global variables, etc.) in order to do this in correct namespace - namespace of class itself but not namespace of one of its child.

		if( value->GetClassField() != nullptr )
		{
			if( !class_type->is_complete )
			{
				// We can't just return class field value if class is incomplete. So, request class build to fill class field properly.
				GlobalThingBuildClass( class_type ); // Class field value changed in this call.
			}
		}
		else if( const auto functions_set= value->GetFunctionsSet() )
		{
			GlobalThingPrepareClassParentsList( class_type );
			if( !class_type->is_complete && !class_type->parents.empty() )
			{
				// Request class build in order to merge functions from parent classes into this functions set.
				GlobalThingBuildClass( class_type ); // Functions set changed in this call.
			}
			GlobalThingBuildFunctionsSet( *class_type->members, *functions_set, false );
		}
		else if( const auto type_templates_set= value->GetTypeTemplatesSet() )
		{
			GlobalThingPrepareClassParentsList( class_type );
			if( !class_type->is_complete && !class_type->parents.empty() )
			{
				// Request class build in order to merge type templaes from parent classes into this type templates set.
				GlobalThingBuildClass( class_type ); // Type templates set changed in this call.
			}
			GlobalThingBuildTypeTemplatesSet( *class_type->members, *type_templates_set );
		}
		else if( value->GetTypedef() != nullptr )
		{
			GlobalThingBuildTypedef( *class_type->members, *value );
		}
		else if( value->GetIncompleteGlobalVariable() != nullptr )
		{
			GlobalThingBuildVariable( *class_type->members, *value );
		}
		else if( const auto type= value->GetTypeName() )
		{
			if( const auto enum_= type->GetEnumType() )
				GlobalThingBuildEnum( enum_ );
		}
		else if(
			value->GetVariable() != nullptr ||
			value->GetStaticAssert() != nullptr ||
			value->GetYetNotDeducedTemplateArg() != nullptr )
		{}
		else U_ASSERT(false);

		return std::make_pair( value, visibility );
	}

	// Do not try to fetch special methods from parent.
	if( is_special_method )
		return std::make_pair( nullptr, ClassMemberVisibility::Public );

	// Value not found in this class. Try to fetch value from parents.
	GlobalThingPrepareClassParentsList( class_type );

	// TODO - maybe produce some kind of error if name found more than in one parents?
	std::pair<Value*, ClassMemberVisibility> parent_class_value{nullptr, ClassMemberVisibility::Public};
	bool has_mergable_thing= false;
	for( const Class::Parent& parent : class_type->parents )
	{
		const auto current_parent_class_value= ResolveClassValue( parent.class_, name );
		if( current_parent_class_value.first != nullptr && current_parent_class_value.second != ClassMemberVisibility::Private )
		{
			const bool current_thing_is_mergable=
				current_parent_class_value.first->GetFunctionsSet() != nullptr ||
				current_parent_class_value.first->GetTypeTemplatesSet() != nullptr;

			if( current_thing_is_mergable && has_mergable_thing && !class_type->is_complete && !recursive_call )
			{
				// If we found more than one functions sets or template sets - trigger class build and perform resolve again.
				// Mergable things will be merged during class build and added into class namespace.
				GlobalThingBuildClass( class_type );
				return ResolveClassValueImpl( class_type, name, true );
			}

			parent_class_value= current_parent_class_value;
			has_mergable_thing|= current_thing_is_mergable;
		}
	}

	// Return public class member as public, protected as protected. Do not return parent class private members.
	return parent_class_value;
}

} // namespace U