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
	return std::visit( [&]( const auto& el ) { return ResolveValueImpl( names_scope, function_context, el ); }, complex_name );
}

Value CodeBuilder::ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TypeofTypeName& typeof_type_name )
{
	return PrepareTypeImpl( names_scope, function_context, typeof_type_name );
}

Value CodeBuilder::ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookup& root_namespace_lookup )
{
	(void)function_context;

	NamesScope* const root_namespace= names_scope.GetRoot();
	U_ASSERT( root_namespace != nullptr );
	NamesScopeValue* const value= root_namespace->GetThisScopeValue( root_namespace_lookup.name );
	if( value == nullptr )
	{
		REPORT_ERROR( NameNotFound, names_scope.GetErrors(), root_namespace_lookup.src_loc, root_namespace_lookup.name );
		return ErrorValue();
	}

	BuildGlobalThingDuringResolveIfNecessary( *root_namespace, value );

	value->referenced= true;
	CollectDefinition( *value, root_namespace_lookup.src_loc );

	return value->value;
}

Value CodeBuilder::ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookupCompletion& root_namespace_lookup_completion )
{
	(void)function_context;
	RootNamespaseLookupCompleteImpl( names_scope, root_namespace_lookup_completion.name );
	return ErrorValue();
}

Value CodeBuilder::ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookup& name_lookup )
{
	(void)function_context;
	const NameLookupResult result= LookupName( names_scope, name_lookup.name, name_lookup.src_loc );

	if( result.value == nullptr )
		return ErrorValue();

	if( result.space != nullptr )
		BuildGlobalThingDuringResolveIfNecessary( *result.space, result.value );

	result.value->referenced= true;
	CollectDefinition( *result.value, name_lookup.src_loc );

	return result.value->value;
}

Value CodeBuilder::ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookupCompletion& name_lookup_completion )
{
	(void)function_context;
	NameLookupCompleteImpl( names_scope, name_lookup_completion.name );
	return ErrorValue();
}

Value CodeBuilder::ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetch& names_scope_fetch )
{
	const Value base= ResolveValue( names_scope, function_context, names_scope_fetch.base );

	NamesScopeValue* value= nullptr;

	if( const NamesScopePtr inner_namespace= base.GetNamespace() )
	{
		value= inner_namespace->GetThisScopeValue( names_scope_fetch.name );
		BuildGlobalThingDuringResolveIfNecessary( *inner_namespace, value );
	}
	else if( const Type* const type= base.GetTypeName() )
	{
		if( const ClassPtr class_= type->GetClassType() )
		{
			const auto class_value= ResolveClassValue( class_, names_scope_fetch.name );
			if( names_scope.GetAccessFor( class_ ) < class_value.second )
				REPORT_ERROR( AccessingNonpublicClassMember, names_scope.GetErrors(), names_scope_fetch.src_loc, names_scope_fetch.name, class_->members->GetThisNamespaceName() );

			if( ( names_scope_fetch.name == Keywords::constructor_ || names_scope_fetch.name == Keywords::destructor_ ) && !function_context.is_in_unsafe_block )
				REPORT_ERROR( ExplicitAccessToThisMethodIsUnsafe, names_scope.GetErrors(), names_scope_fetch.src_loc, names_scope_fetch.name );

			value= class_value.first;
			// ResolveClassValue performs proper building of resolved value.
		}
		else if( const EnumPtr enum_= type->GetEnumType() )
		{
			value= enum_->members.GetThisScopeValue( names_scope_fetch.name );
			BuildGlobalThingDuringResolveIfNecessary( enum_->members, value );
		}
	}
	else if( base.GetTypeTemplatesSet() != nullptr )
		REPORT_ERROR( TemplateInstantiationRequired, names_scope.GetErrors(), names_scope_fetch.src_loc, names_scope_fetch.base );

	if( value == nullptr )
	{
		REPORT_ERROR( NameNotFound, names_scope.GetErrors(), names_scope_fetch.src_loc, names_scope_fetch.name );
		return ErrorValue();
	}

	value->referenced= true;
	CollectDefinition( *value, names_scope_fetch.src_loc );

	return value->value;
}

Value CodeBuilder::ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetchCompletion& names_scope_fetch_completion )
{
	const Value base= ResolveValue( names_scope, function_context, names_scope_fetch_completion.base );
	NamesScopeFetchComleteImpl( base, names_scope_fetch_completion.name );

	return ErrorValue();
}

Value CodeBuilder::ResolveValueImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::TemplateParametrization& template_parametrization )
{
	const Value base= ResolveValue( names_scope, function_context, template_parametrization.base );

	NamesScopeValue* value= nullptr;

	if( const TypeTemplatesSet* const type_templates_set= base.GetTypeTemplatesSet() )
		value=
			GenTemplateType(
				template_parametrization.src_loc,
				*type_templates_set,
				template_parametrization.template_args,
				names_scope,
				function_context );
	else if( const OverloadedFunctionsSetPtr functions_set= base.GetFunctionsSet() )
	{
		if( functions_set->template_functions.empty() )
			REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), template_parametrization.src_loc );
		else
			value=
				ParametrizeFunctionTemplate(
					template_parametrization.src_loc,
					*functions_set,
					template_parametrization.template_args,
					names_scope,
					function_context );
	}
	else
		REPORT_ERROR( ValueIsNotTemplate, names_scope.GetErrors(), template_parametrization.src_loc );

	if( value == nullptr )
		return ErrorValue();

	value->referenced= true;

	return value->value;
}

void CodeBuilder::BuildGlobalThingDuringResolveIfNecessary( NamesScope& names_scope, NamesScopeValue* const value )
{
	if( value == nullptr )
		return;

	// Build almost everything except classes.
	// Classes building will be triggered later - during class usage or class name lookup (if it is necessary).

	if( const OverloadedFunctionsSetPtr functions_set= value->value.GetFunctionsSet() )
		GlobalThingBuildFunctionsSet( names_scope, *functions_set, false );
	else if( TypeTemplatesSet* const type_templates_set= value->value.GetTypeTemplatesSet() )
		GlobalThingBuildTypeTemplatesSet( names_scope, *type_templates_set );
	else if( value->value.GetTypeAlias() != nullptr )
		GlobalThingBuildTypeAlias( names_scope, value->value );
	else if( value->value.GetIncompleteGlobalVariable() != nullptr )
		GlobalThingBuildVariable( names_scope, value->value );
	else if( const Type* const type= value->value.GetTypeName() )
	{
		if( const EnumPtr enum_= type->GetEnumType() )
			GlobalThingBuildEnum( enum_ );
	}
}

CodeBuilder::NameLookupResult CodeBuilder::LookupName( NamesScope& names_scope, const std::string_view name, const SrcLoc& src_loc )
{
	NamesScope* last_space= &names_scope;
	NamesScopeValue* value= nullptr;

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
	else if( value->value.GetYetNotDeducedTemplateArg() != nullptr )
		REPORT_ERROR( TemplateArgumentIsNotDeducedYet, names_scope.GetErrors(), src_loc, name );

	return NameLookupResult{ last_space, value };
}

std::pair<NamesScopeValue*, ClassMemberVisibility> CodeBuilder::ResolveClassValue( const ClassPtr class_type, const std::string_view name )
{
	return ResolveClassValueImpl( class_type, name );
}

std::pair<NamesScopeValue*, ClassMemberVisibility> CodeBuilder::ResolveClassValueImpl( ClassPtr class_type, const std::string_view name, const bool recursive_call )
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

	if( NamesScopeValue* const value= class_type->members->GetThisScopeValue( name ) )
	{
		const auto visibility= class_type->GetMemberVisibility( name );

		// We need to build some things right now (type aliases, global variables, etc.) in order to do this in correct namespace - namespace of class itself but not namespace of one of its child.

		if( value->value.GetClassField() != nullptr )
		{
			if( !class_type->is_complete )
			{
				// We can't just return class field value if class is incomplete. So, request class build to fill class field properly.
				GlobalThingBuildClass( class_type ); // Class field value changed in this call.
			}
		}
		else if( const auto functions_set= value->value.GetFunctionsSet() )
		{
			GlobalThingPrepareClassParentsList( class_type );
			if( !class_type->is_complete && !class_type->parents.empty() )
			{
				// Request class build in order to merge functions from parent classes into this functions set.
				GlobalThingBuildClass( class_type ); // Functions set changed in this call.
			}
			GlobalThingBuildFunctionsSet( *class_type->members, *functions_set, false );
		}
		else if( const auto type_templates_set= value->value.GetTypeTemplatesSet() )
		{
			GlobalThingPrepareClassParentsList( class_type );
			if( !class_type->is_complete && !class_type->parents.empty() )
			{
				// Request class build in order to merge type templaes from parent classes into this type templates set.
				GlobalThingBuildClass( class_type ); // Type templates set changed in this call.
			}
			GlobalThingBuildTypeTemplatesSet( *class_type->members, *type_templates_set );
		}
		else if( value->value.GetTypeAlias() != nullptr )
		{
			GlobalThingBuildTypeAlias( *class_type->members, value->value );
		}
		else if( value->value.GetIncompleteGlobalVariable() != nullptr )
		{
			GlobalThingBuildVariable( *class_type->members, value->value );
		}
		else if( const auto type= value->value.GetTypeName() )
		{
			if( const auto enum_= type->GetEnumType() )
				GlobalThingBuildEnum( enum_ );
		}
		else if(
			value->value.GetVariable() != nullptr ||
			value->value.GetStaticAssert() != nullptr ||
			value->value.GetYetNotDeducedTemplateArg() != nullptr )
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
	std::pair<NamesScopeValue*, ClassMemberVisibility> parent_class_value{nullptr, ClassMemberVisibility::Public};
	bool has_mergable_thing= false;
	for( const Class::Parent& parent : class_type->parents )
	{
		const auto current_parent_class_value= ResolveClassValue( parent.class_, name );
		if( current_parent_class_value.first != nullptr && current_parent_class_value.second != ClassMemberVisibility::Private )
		{
			const bool current_thing_is_mergable=
				current_parent_class_value.first->value.GetFunctionsSet() != nullptr ||
				current_parent_class_value.first->value.GetTypeTemplatesSet() != nullptr;

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
