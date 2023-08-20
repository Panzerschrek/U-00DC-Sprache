#include "../../lex_synt_lib_common/assert.hpp"
#include "code_builder.hpp"

namespace U
{

std::optional<SrcLoc> CodeBuilder::GetDefinition( const SrcLoc& src_loc )
{
	const auto it= definition_points_.find( src_loc );
	if( it == definition_points_.end() )
		return std::nullopt;

	return it->second.src_loc;
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

void CodeBuilder::CollectDefinition( const NamesScopeValue& value, const SrcLoc& src_loc )
{
	if( !collect_definition_points_ )
		return;

	// TODO - maybe store only definitions for main file (with 0 index)?

	DefinitionPoint point;
	point.src_loc= GetDefinitionFetchSrcLoc( value );

	if( const auto variable_ptr= value.value.GetVariable() )
	{
		if( variable_ptr != nullptr )
			point.type= variable_ptr->type;
	}
	if( const auto class_field= value.value.GetClassField() )
	{
		if( class_field != nullptr )
			point.type= class_field->type;
	}

	definition_points_.insert( std::make_pair( src_loc, std::move(point) ) );
}

} // namespace U
