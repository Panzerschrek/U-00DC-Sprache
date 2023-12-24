#include <sstream>
#include "../lex_synt_lib_common/assert.hpp"
#include "../compiler0/lex_synt_lib/program_writer.hpp"
#include "keywords.hpp"

#include "document_symbols.hpp"

namespace U
{

namespace LangServer
{

namespace
{

DocumentRange MakeRange(
	const SrcLoc& src_loc,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function )
{
	if( auto range= src_loc_to_range_mapping_function(src_loc) )
		return std::move(*range);

	// Backup range creation.
	// It is wrong, since it uses no valid utf32->utf16 convesion and doesn't extract correct identifier ranges,
	// but this is better than nothing.
	return DocumentRange{ { src_loc.GetLine(), src_loc.GetColumn() }, { src_loc.GetLine(), src_loc.GetColumn()+ 1 } };
}

Symbols BuildMacrosSymbols(
	const Synt::MacrosPtr& macros,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function )
{
	Symbols result;
	if( macros == nullptr )
		return result;

	for( const auto& constex_macro_map_pair : *macros )
	{
		for( const auto& name_macro_pair : constex_macro_map_pair.second )
		{
			const Synt::Macro& macro= name_macro_pair.second;
			if( macro.src_loc.GetFileIndex() != 0 )
				continue; // Ignore imported macros.

			if( macro.name == "if_var" || macro.name == "foreach" ) // HACK - ignore built-in macros.
				continue;

			Symbol symbol;
			symbol.kind= SymbolKind::Namespace; // There is no kind for macros in LSP.
			symbol.name= macro.name;
			symbol.range= MakeRange( macro.src_loc, src_loc_to_range_mapping_function );
			symbol.selection_range= symbol.range;
			result.push_back(symbol);
		}
	}

	std::sort(
		result.begin(), result.end(),
		[](const Symbol& l, const Symbol& r ) { return l.range < r.range; } );

	return result;
}

std::string Stringify( const Synt::TypeName& type_name )
{
	std::stringstream ss;
	Synt::WriteTypeName( type_name, ss );
	return ss.str();
}

std::string Stringify( const Synt::Function& function )
{
	std::stringstream ss;
	Synt::WriteFunctionDeclaration( function, ss );
	return ss.str();
}

void WriteMutabilityModifier( const Synt::MutabilityModifier mutability_modifier, std::ostream& stream )
{
	switch( mutability_modifier )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : stream << Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: stream << Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: stream << Keyword( Keywords::constexpr_ );break;
	}
}

void WriteTemplateParams( const std::vector<Synt::TypeTemplate::Param>& params, std::ostream& stream )
{
	for( const Synt::TypeTemplate::Param& param : params )
	{
		if( param.param_type != std::nullopt )
			Synt::WriteProgramElement( *param.param_type, stream );
		else
			stream << Keyword( Keywords::type_ );

		stream << " " << param.name;
		if( &param != &params.back() )
			stream << ", ";
	}
}

std::string Stringify( const Synt::TypeTemplate& type_template )
{
	std::stringstream ss;
	ss << type_template.name;

	ss << "</";
	if( type_template.is_short_form )
		WriteTemplateParams( type_template.params, ss );
	else
	{
		for( const Synt::TypeTemplate::SignatureParam& param : type_template.signature_params )
		{
			Synt::WriteExpression( param.name, ss );
			if( std::get_if<Synt::EmptyVariant>(&param.default_value) != nullptr )
			{
				ss << "= ";
				Synt::WriteExpression( param.default_value, ss );
			}
			if( &param != &type_template.signature_params.back() )
				ss << ", ";
		}
	}
	ss << "/>";

	return ss.str();
}

std::string Stringify( const Synt::FunctionTemplate& function_template )
{
	std::stringstream ss;

	ss << Keyword( Keywords::template_ );
	ss << "</";
	WriteTemplateParams( function_template.params, ss );
	ss << "/>";

	ss << " ";
	if( function_template.function != nullptr )
		Synt::WriteFunctionDeclaration( *function_template.function, ss );

	return ss.str();
}

std::string Stringify( const Synt::ClassField& class_field )
{
	std::stringstream ss;
	ss << class_field.name << ": ";
	Synt::WriteTypeName( class_field.type, ss );
	ss << " ";

	switch( class_field.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: ss << "&"; break;
	}

	WriteMutabilityModifier( class_field.mutability_modifier, ss );

	return ss.str();
}

std::string Stringify( const Synt::VariablesDeclaration::VariableEntry& variable, const std::string& type )
{
	std::stringstream ss;
	ss << variable.name << ": " << type << " ";

	switch( variable.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: ss << "&"; break;
	}

	WriteMutabilityModifier( variable.mutability_modifier, ss );

	return ss.str();
}

std::string Stringify( const Synt::AutoVariableDeclaration& variable )
{
	std::stringstream ss;
	ss << variable.name << ": " << Keyword( Keywords::auto_ ) << " ";

	switch( variable.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: ss << "&"; break;
	}

	WriteMutabilityModifier( variable.mutability_modifier, ss );

	return ss.str();
}

Symbols BuildProgramModel_r(
	const Synt::Enum& enum_,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function )
{
	Symbols result;

	for( const Synt::Enum::Member& member : enum_.members )
	{
		Symbol symbol;
		symbol.name= member.name;
		symbol.range= MakeRange( member.src_loc, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::EnumMember;
		result.push_back(symbol);
	}

	return result;
}

Symbols BuildProgramModel_r(
	const Synt::ClassElementsList& elements,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function );

Symbols BuildProgramModel_r(
	const Synt::ProgramElementsList& elements,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function );

struct Visitor final
{
	explicit Visitor( const SrcLocToRangeMappingFunction& in_src_loc_to_range_mapping_function )
		: src_loc_to_range_mapping_function(in_src_loc_to_range_mapping_function)
	{}

	Symbols result;
	SrcLocToRangeMappingFunction src_loc_to_range_mapping_function;

	void operator()( const Synt::ClassField& class_field_ )
	{
		Symbol symbol;
		symbol.name= Stringify( class_field_ );
		symbol.range= MakeRange( class_field_.src_loc, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Field;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::Function& func )
	{
		if( func.name.empty() )
			return;

		Symbol symbol;
		symbol.name= Stringify( func );
		symbol.range= MakeRange( func.src_loc, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Function;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::FunctionTemplate& func_template )
	{
		if( func_template.function == nullptr || func_template.function->name.empty() )
			return;

		Symbol symbol;
		symbol.name= Stringify( func_template );
		symbol.range= MakeRange( func_template.src_loc, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Function;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::ClassVisibilityLabel& )
	{
	}
	void operator()( const Synt::TypeTemplate& type_template )
	{
		Symbol symbol;
		symbol.name= Stringify( type_template );
		symbol.range= MakeRange( type_template.src_loc, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range; // TODO - set it properly.
		symbol.kind= SymbolKind::Class;

		if( const auto class_= std::get_if< std::unique_ptr<const Synt::Class> >( &type_template.something ) )
		{
			symbol.children= BuildProgramModel_r( (*class_)->elements, src_loc_to_range_mapping_function );
		}
		if( std::get_if<std::unique_ptr<const Synt::TypeAlias>>( &type_template.something ) != nullptr )
		{}

		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::Enum& enum_ )
	{
		Symbol symbol;
		symbol.name= enum_.name;
		symbol.range= MakeRange( enum_.src_loc, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Enum;
		symbol.children= BuildProgramModel_r( enum_, src_loc_to_range_mapping_function );
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::StaticAssert&  )
	{
	}
	void operator()( const Synt::TypeAlias& type_alias )
	{
		Symbol symbol;
		symbol.name= type_alias.name;
		symbol.range= MakeRange( type_alias.src_loc, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Class;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::VariablesDeclaration& variables_declaration )
	{
		const std::string type_name= Stringify( variables_declaration.type );
		for( const auto& variable : variables_declaration.variables )
		{
			Symbol symbol;
			symbol.name= Stringify( variable, type_name );
			symbol.range= MakeRange( variable.src_loc, src_loc_to_range_mapping_function );
			symbol.selection_range= symbol.range;
			symbol.kind= SymbolKind::Variable;
			result.push_back( std::move(symbol) );
		}
	}
	void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
	{
		Symbol symbol;
		symbol.name= Stringify( auto_variable_declaration );
		symbol.range= MakeRange( auto_variable_declaration.src_loc, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Variable;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::Class& class_ )
	{
		Symbol symbol;
		symbol.name= class_.name;
		symbol.range= MakeRange( class_.src_loc, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= class_.kind_attribute_ == Synt::ClassKindAttribute::Struct ? SymbolKind::Struct : SymbolKind::Class;
		symbol.children= BuildProgramModel_r( class_.elements, src_loc_to_range_mapping_function );
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::Namespace& namespace_ )
	{
		Symbol symbol;
		symbol.name= namespace_.name;
		symbol.range= MakeRange( namespace_.src_loc, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Namespace;
		symbol.children= BuildProgramModel_r( namespace_.elements, src_loc_to_range_mapping_function );
		result.push_back( std::move(symbol) );
	}
};

Symbols BuildProgramModel_r(
	const Synt::ClassElementsList& elements,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function )
{
	Visitor visitor( src_loc_to_range_mapping_function );
	elements.Iter( visitor );

	return std::move(visitor.result);
}

Symbols BuildProgramModel_r(
	const Synt::ProgramElementsList& elements,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function )
{
	Visitor visitor( src_loc_to_range_mapping_function );
	elements.Iter( visitor );

	return std::move(visitor.result);
}

void SetupRanges_r( Symbols& symbols, DocumentPosition& prev_position )
{
	for( auto it= symbols.rbegin(), it_end= symbols.rend(); it != it_end; ++it )
	{
		// it->range.end= prev_position;
		SetupRanges_r( it->children, prev_position );
		// HACK! Limit symbol position to start position of first child.
		// This is needed, because some stupid IDEs (like QtCreator) can't properly select the most deep symbol under cursor.
		it->range.end= prev_position;
		prev_position= it->range.start;
	}
}

void SetupRanges( Symbols& symbols )
{
	if( symbols.empty() )
		return;

	DocumentPosition end_position;
	Symbol* current_symbol= &symbols.back();
	while(true)
	{
		end_position= current_symbol->range.end;
		if( !current_symbol->children.empty() )
			current_symbol= &current_symbol->children.back();
		else
			break;
	}

	SetupRanges_r( symbols, end_position );
}

} // namespace

Symbols BuildSymbols(
	const Synt::SyntaxAnalysisResult& synt_result,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function )
{
	auto result= BuildMacrosSymbols( synt_result.macros, src_loc_to_range_mapping_function );

	{
		auto document_symbols= BuildProgramModel_r( synt_result.program_elements, src_loc_to_range_mapping_function );

		result.reserve( result.size() + document_symbols.size() );
		for( Symbol& symbol : document_symbols )
			result.push_back( std::move(symbol) );
	}

	SetupRanges(result);
	return result;
}

} // namespace LangServer

} // namespace U
