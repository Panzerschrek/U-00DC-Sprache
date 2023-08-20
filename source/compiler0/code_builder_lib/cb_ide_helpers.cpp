#include "../../lex_synt_lib_common/assert.hpp"
#include "code_builder.hpp"

namespace U
{

std::optional<SrcLoc> CodeBuilder::GetDefinition( const llvm::ArrayRef<DefinitionRequestPrefixComponent> prefix, const GetDefinitionRequestItem& item )
{
	// TODO - allow fetching from non-main file?
	NamesScope& start_names_scope= *compiled_sources_.front().names_map;
	FunctionContext& function_context= *global_function_context_;

	NamesScope* const scope= EvaluateGetDefinitionRequestPrefix( start_names_scope, prefix );
	if( scope == nullptr )
		return std::nullopt;

	return std::visit( [&]( const auto& el ) { return GetDefinitionImpl( *scope, function_context, el ); }, item );
}

NamesScope* CodeBuilder::EvaluateGetDefinitionRequestPrefix( NamesScope& start_scope, const llvm::ArrayRef<DefinitionRequestPrefixComponent> prefix )
{
	if( prefix.empty() )
		return &start_scope;

	const DefinitionRequestPrefixComponent& prefix_head= prefix.front();
	const auto prefix_tail= prefix.drop_front();

	if( const auto namespace_= std::get_if<const Synt::Namespace*>( &prefix_head ) )
	{
		if( const NamesScopeValue* const value= start_scope.GetThisScopeValue( (*namespace_)->name_ ) )
		{
			if( const auto names_scope= value->value.GetNamespace() )
				return EvaluateGetDefinitionRequestPrefix( *names_scope, prefix_tail );
		}
	}
	else if( const auto class_= std::get_if<const Synt::Class*>( &prefix_head ) )
	{
		if( const NamesScopeValue* const value= start_scope.GetThisScopeValue( (*class_)->name_ ) )
		{
			if( const auto type_name= value->value.GetTypeName() )
			{
				if( const auto class_type= type_name->GetClassType() )
					return EvaluateGetDefinitionRequestPrefix( *class_type->members, prefix_tail );
			}
		}
	}
	else if( const auto type_template= std::get_if<const Synt::TypeTemplate*>( &prefix_head ) )
	{
		if( const NamesScopeValue* const value= start_scope.GetThisScopeValue( (*type_template)->name_ ) )
		{
			if( const auto type_templates_set= value->value.GetTypeTemplatesSet() )
			{
				for( const TypeTemplatePtr& type_template_ptr : type_templates_set->type_templates )
				{
					if( type_template_ptr->syntax_element == *type_template )
					{
						// Found this type template.
						// TODO - instantiate type template with dummy ags and search something inside it.
						return nullptr;
					}
				}
			}
		}
	}
	else U_ASSERT(false);

	return nullptr;
}

std::optional<SrcLoc> CodeBuilder::GetDefinitionImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookup* const name_lookup )
{
	(void)function_context;

	const NameLookupResult result= LookupName( names_scope, name_lookup->name, name_lookup->src_loc_ );
	if( result.value != nullptr )
		return GetDefinitionFetchSrcLoc( *result.value );

	return std::nullopt;
}

std::optional<SrcLoc> CodeBuilder::GetDefinitionImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookup* const root_namespace_lookup )
{
	(void)function_context;

	const NamesScopeValue* const value= names_scope.GetRoot()->GetThisScopeValue( root_namespace_lookup->name );
	if( value != nullptr )
		return GetDefinitionFetchSrcLoc( *value );

	return std::nullopt;
}

std::optional<SrcLoc> CodeBuilder::GetDefinitionImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NamesScopeNameFetch* const names_scope_fetch )
{
	const Value base= ResolveValue( names_scope, function_context, *names_scope_fetch->base );

	// Do not need to make things complete here and/or check for access rights.

	NamesScopeValue* value= nullptr;
	if( const NamesScopePtr inner_namespace= base.GetNamespace() )
		value= inner_namespace->GetThisScopeValue( names_scope_fetch->name );
	else if( const Type* const type= base.GetTypeName() )
	{
		if( const ClassPtr class_= type->GetClassType() )
			value= ResolveClassValue( class_, names_scope_fetch->name ).first;
		else if( const EnumPtr enum_= type->GetEnumType() )
			value= enum_->members.GetThisScopeValue( names_scope_fetch->name );
	}

	if( value != nullptr )
		return GetDefinitionFetchSrcLoc( *value );

	return std::nullopt;
}

std::optional<SrcLoc> CodeBuilder::GetDefinitionImpl( NamesScope& names_scope, FunctionContext& function_context,  const Synt::MemberAccessOperator* const member_access_operator )
{
	const VariablePtr variable= BuildExpressionCodeEnsureVariable( *member_access_operator->expression_, names_scope, function_context );

	Class* const class_type= variable->type.GetClassType();
	if( class_type == nullptr )
		return std::nullopt;

	const auto class_value= ResolveClassValue( class_type, member_access_operator->member_name_ );
	NamesScopeValue* const class_member= class_value.first;
	if( class_member == nullptr )
		return std::nullopt;

	return GetDefinitionFetchSrcLoc( *class_member );
}

SrcLoc CodeBuilder::GetDefinitionFetchSrcLoc( const NamesScopeValue& value )
{
	// Process functions set specially.
	// TODO - maybe perform overloaidng resolution to fetch proper function?
	if( const auto functions_set= value.value.GetFunctionsSet() )
	{
		if( !functions_set->functions.empty() )
			return functions_set->functions.front().body_src_loc;
		if( !functions_set->template_functions.empty() )
			return functions_set->template_functions.front()->src_loc;
	}
	if( const auto type_templates_set= value.value.GetTypeTemplatesSet() )
	{
		if( !type_templates_set->type_templates.empty() )
			return type_templates_set->type_templates.front()->src_loc;
	}

	return value.src_loc;
}

} // namespace U
