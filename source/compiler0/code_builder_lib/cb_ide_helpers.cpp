#include "keywords.hpp"
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

std::vector<CodeBuilder::Symbol> CodeBuilder::GetMainFileSymbols()
{
	const NamesScope& root_names_scope= *compiled_sources_.front().names_map;

	return GetMainFileSymbols_r( root_names_scope );
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

	// Store only definitions for main file (in order to consume less memory), since this functionality is used only for main file.
	// TODO - make this behaviour configurable?
	if( src_loc.GetFileIndex() != 0 )
		return;

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

std::vector<CodeBuilder::Symbol> CodeBuilder::GetMainFileSymbols_r( const NamesScope& names_scope )
{
	std::vector<Symbol> result;

	names_scope.ForEachInThisScope(
		[&]( const std::string_view name, const NamesScopeValue& names_scope_value )
		{
			const Value& value= names_scope_value.value;
			if( const auto functions_set= value.GetFunctionsSet() )
			{
				// Process function sets specially.
				for( const FunctionVariable& function_variable : functions_set->functions )
				{
					if( function_variable.is_generated && function_variable.prototype_src_loc == SrcLoc() && function_variable.body_src_loc == SrcLoc() )
						continue; // Skip generated stuff.

					Symbol symbol;
					// TODO - encode also params.
					symbol.src_loc= function_variable.body_src_loc;

					if( name == Keywords::constructor_ )
						symbol.kind= SymbolKind::Constructor;
					else if( functions_set->base_class != nullptr )
						symbol.kind= SymbolKind::Method;
					else
						symbol.kind= SymbolKind::Function;

					symbol.name= std::string(name);

					result.push_back( std::move(symbol) );
				}

				return;
			}

			if( names_scope_value.src_loc.GetFileIndex() != 0 )
				return; // Imported or generated stuff.

			Symbol symbol;
			symbol.src_loc= names_scope_value.src_loc;

			if( value.GetVariable() != nullptr )
				symbol.kind= SymbolKind::Variable;
			else if( const auto type= value.GetTypeName() )
			{
				if( const auto class_= type->GetClassType() )
				{
					if( class_->members->GetParent() == &names_scope )
					{
						symbol.children= GetMainFileSymbols_r( *class_->members );
						symbol.kind= SymbolKind::Class;
					}
				}
				else if( const auto enum_= type->GetEnumType() )
				{
					if( enum_->members.GetParent() == &names_scope )
					{
						symbol.children= GetMainFileSymbols_r( enum_->members );
						symbol.kind= SymbolKind::Enum;
					}
				}

				// TODO - set kind for type alias?
			}
			else if( value.GetClassField() != nullptr )
				symbol.kind= SymbolKind::Field;
			else if( const auto child_namespace= value.GetNamespace() )
			{
				symbol.children= GetMainFileSymbols_r( *child_namespace );
				symbol.kind= SymbolKind::Namespace;
			}
			else if( value.GetStaticAssert() != nullptr )
				return;

			symbol.name= std::string(name);

			// TODO - handle each function each type template in set.

			result.push_back( std::move(symbol) );
		} );

	std::sort(
		result.begin(), result.end(),
		[]( const Symbol& l, const Symbol& r ) { return l.src_loc < r.src_loc; } );

	return result;
}

} // namespace U
