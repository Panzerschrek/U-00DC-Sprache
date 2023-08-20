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
	else U_ASSERT(false);

	return nullptr;
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

	const NamesScopeValue* const value= names_scope.GetRoot()->GetThisScopeValue( root_namespace_lookup->name );
	if( value != nullptr )
		return value->src_loc;

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
