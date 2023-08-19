#include "code_builder.hpp"

namespace U
{

std::optional<SrcLoc> CodeBuilder::GetDefinition( const GetDefinitionRequestItem& item )
{
	// TODO - allow fetching from non-main file?
	NamesScope& names_scope= *compiled_sources_.front().names_map;
	FunctionContext& function_context= *global_function_context_;

	return std::visit( [&]( const auto& el ) { return GetDefinitionImpl( names_scope, function_context, el ); }, item );
}

std::optional<SrcLoc> CodeBuilder::GetDefinitionImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::EmptyVariant& empty_variant )
{
	(void)empty_variant;
	(void)names_scope;
	(void)function_context;
	return std::nullopt;
}

std::optional<SrcLoc> CodeBuilder::GetDefinitionImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::NameLookup* const name_lookup )
{
	(void)function_context;

	const NameLookupResult result= LookupName( names_scope, name_lookup->name, name_lookup->src_loc_ );
	if( result.value != nullptr )
		return result.value->src_loc;

	return std::nullopt;
}

std::optional<SrcLoc> CodeBuilder::GetDefinitionImpl( NamesScope& names_scope, FunctionContext& function_context, const Synt::RootNamespaceNameLookup* const root_namespace_lookup )
{
	(void)function_context;

	const NameLookupResult result= LookupName( *names_scope.GetRoot(), root_namespace_lookup->name, root_namespace_lookup->src_loc_ );
	if( result.value != nullptr )
		return result.value->src_loc;

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
		return value->src_loc;

	return std::nullopt;
}

std::optional<SrcLoc> CodeBuilder::GetDefinitionImpl( NamesScope& names_scope, FunctionContext& function_context,  const Synt::MemberAccessOperator* const member_access_operator )
{
	// TODO
	(void)names_scope;
	(void)function_context;
	(void)member_access_operator;
	return std::nullopt;
}

} // namespace U
