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
	// It is wrong, since it uses no valid utf32->utf16 convertion and doesn't extract correct identifier ranges,
	// but this is better than nothing.
	return DocumentRange{ { src_loc.GetLine(), src_loc.GetColumn() }, { src_loc.GetLine(), src_loc.GetColumn()+ 1 } };
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
	ss << type_template.name_;

	ss << "</";
	if( type_template.is_short_form_ )
		WriteTemplateParams( type_template.params_, ss );
	else
	{
		for( const Synt::TypeTemplate::SignatureParam& param : type_template.signature_params_ )
		{
			Synt::WriteExpression( param.name, ss );
			if( std::get_if<Synt::EmptyVariant>(&param.default_value) != nullptr )
			{
				ss << "= ";
				Synt::WriteExpression( param.default_value, ss );
			}
			if( &param != &type_template.signature_params_.back() )
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
	WriteTemplateParams( function_template.params_, ss );
	ss << "/>";

	ss << " ";
	if( function_template.function_ != nullptr )
		Synt::WriteFunctionDeclaration( *function_template.function_, ss );

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

std::vector<Symbol> BuildProgramModel_r(
	const Synt::Enum& enum_,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function )
{
	std::vector<Symbol> result;

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

std::vector<Symbol> BuildProgramModel_r(
	const Synt::ClassElements& elements,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function );

std::vector<Symbol> BuildProgramModel_r(
	const Synt::ProgramElements& elements,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function );

struct Visitor final
{
	explicit Visitor( const SrcLocToRangeMappingFunction& in_src_loc_to_range_mapping_function )
		: src_loc_to_range_mapping_function(in_src_loc_to_range_mapping_function)
	{}

	std::vector<Symbol> result;
	SrcLocToRangeMappingFunction src_loc_to_range_mapping_function;

	void operator()( const Synt::ClassField& class_field_ )
	{
		Symbol symbol;
		symbol.name= Stringify( class_field_ );
		symbol.range= MakeRange( class_field_.src_loc_, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Field;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::FunctionPtr& func )
	{
		if( func == nullptr || func->name_.empty() )
			return;

		Symbol symbol;
		symbol.name= Stringify( *func );
		symbol.range= MakeRange( func->src_loc_, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Function;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::FunctionTemplate& func_template )
	{
		if( func_template.function_ == nullptr || func_template.function_->name_.empty() )
			return;

		Symbol symbol;
		symbol.name= Stringify( func_template );
		symbol.range= MakeRange( func_template.src_loc_, src_loc_to_range_mapping_function );
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
		symbol.range= MakeRange( type_template.src_loc_, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range; // TODO - set it properly.
		symbol.kind= SymbolKind::Class;

		if( const auto class_= std::get_if<Synt::ClassPtr>( &type_template.something_ ) )
		{
			symbol.children= BuildProgramModel_r( (*class_)->elements_, src_loc_to_range_mapping_function );
		}
		if( std::get_if<std::unique_ptr<const Synt::TypeAlias>>( &type_template.something_ ) != nullptr )
		{}

		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::Enum& enum_ )
	{
		Symbol symbol;
		symbol.name= enum_.name;
		symbol.range= MakeRange( enum_.src_loc_, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Enum;
		symbol.children= BuildProgramModel_r( enum_, src_loc_to_range_mapping_function );
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::StaticAssert&  )
	{
	}
	void operator()( const Synt::TypeAlias& typedef_ )
	{
		Symbol symbol;
		symbol.name= typedef_.name;
		symbol.range= MakeRange( typedef_.src_loc_, src_loc_to_range_mapping_function );
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
		symbol.range= MakeRange( auto_variable_declaration.src_loc_, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Variable;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::ClassPtr& class_ )
	{
		if( class_ == nullptr )
			return;

		Symbol symbol;
		symbol.name= class_->name_;
		symbol.range= MakeRange( class_->src_loc_, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? SymbolKind::Struct : SymbolKind::Class;
		symbol.children= BuildProgramModel_r( class_->elements_, src_loc_to_range_mapping_function );
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::NamespacePtr& namespace_ )
	{
		if( namespace_ == nullptr )
			return;

		Symbol symbol;
		symbol.name= namespace_->name_;
		symbol.range= MakeRange( namespace_->src_loc_, src_loc_to_range_mapping_function );
		symbol.selection_range= symbol.range;
		symbol.kind= SymbolKind::Namespace;
		symbol.children= BuildProgramModel_r( namespace_->elements_, src_loc_to_range_mapping_function );
		result.push_back( std::move(symbol) );
	}
};

std::vector<Symbol> BuildProgramModel_r(
	const Synt::ClassElements& elements,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function )
{
	Visitor visitor( src_loc_to_range_mapping_function );
	for( const Synt::ClassElement& class_element : elements )
		std::visit( visitor, class_element );

	return std::move(visitor.result);
}

std::vector<Symbol> BuildProgramModel_r(
	const Synt::ProgramElements& elements,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function )
{
	Visitor visitor( src_loc_to_range_mapping_function );
	for( const Synt::ProgramElement& program_element : elements )
		std::visit( visitor, program_element );

	return std::move(visitor.result);
}

void SetupRanges_r( std::vector<Symbol>& symbols, DocumentPosition& prev_position )
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

void SetupRanges( std::vector<Symbol>& symbols )
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

std::vector<Symbol> BuildSymbols(
	const Synt::ProgramElements& program_elements,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function )
{
	// TODO - include also macros.
	auto result= BuildProgramModel_r( program_elements, src_loc_to_range_mapping_function );
	SetupRanges(result);
	return result;
}

} // namespace LangServer

} // namespace U
