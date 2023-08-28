#include <sstream>
#include "keywords.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "../lex_synt_lib/program_writer.hpp"
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

std::vector<SrcLoc> CodeBuilder::GetAllOccurrences( const SrcLoc& src_loc )
{
	std::vector<SrcLoc> result;

	if( const auto definition_point= GetDefinition( src_loc ) )
	{
		// Found a definition point. Find all usages of it.
		result.push_back( *definition_point );
		for( const auto& definition_point_pair : definition_points_ )
		{
			if( definition_point == definition_point_pair.second.src_loc )
				result.push_back( definition_point_pair.first );
		}
	}
	else
	{
		// Assume, that this is definition itself. Find all usages.
		result.push_back( src_loc );
		for( const auto& definition_point_pair : definition_points_ )
		{
			if( src_loc == definition_point_pair.second.src_loc )
				result.push_back( definition_point_pair.first );
		}
	}

	std::sort( result.begin(), result.end() );
	result.erase( std::unique( result.begin(), result.end() ), result.end() );
	return result;
}

std::vector<std::string> CodeBuilder::Complete( const Synt::ProgramElement& program_element )
{
	// TODO - fetch namespaces/classes.
	NamesScope& names_scope= *compiled_sources_.front().names_map;

	// Put given element into dummy names scope.
	NamesScope element_names_scope( "", &names_scope );

	completion_items_.clear();

	// Create name for given item.
	std::visit(
		[&]( const auto& el ) { NamesScopeFill( element_names_scope, el ); },
		program_element );

	// Build it.
	GlobalThingBuildNamespace( element_names_scope );

	std::vector<std::string> result;
	result.swap( completion_items_ );
	std::sort( result.begin(), result.end() );
	result.erase( std::unique( result.begin(), result.end() ), result.end() );
	return result;
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
		if( !functions_set->syntax_elements.empty() )
			return functions_set->syntax_elements.front()->src_loc_;
		if( !functions_set->out_of_line_syntax_elements.empty() )
			return functions_set->out_of_line_syntax_elements.front()->src_loc_;
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

	// For now enable saving definitions for non-main (imported) files, in order to implement occurences search.

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

void CodeBuilder::NameLookupCompleteImpl( const NamesScope& names_scope, const std::string_view name )
{
	const NamesScope* current_scope= &names_scope;
	do
	{
		current_scope->ForEachInThisScope(
			[&]( const std::string_view namespace_name, const NamesScopeValue& value )
			{
				(void)value;
				if( namespace_name.size() >= name.size() && namespace_name.substr(0, name.size() ) == name )
					completion_items_.push_back( std::string(namespace_name) );
			});

		current_scope= current_scope->GetParent();
	} while( current_scope != nullptr );
}

} // namespace U
