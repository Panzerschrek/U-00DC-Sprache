#include <sstream>
#include "../lex_synt_lib_common/assert.hpp"
#include "../compiler0/lex_synt_lib/program_writer.hpp"
#include "keywords.hpp"

#include "document_symbols.hpp"

namespace U
{

namespace
{

DocumentRange MakeRangeInitial( const SrcLoc& src_loc )
{
	return DocumentRange{ { src_loc.GetLine(), src_loc.GetColumn() }, { src_loc.GetLine(), src_loc.GetColumn() } };
}

DocumentRange MakeSelectionRange( const SrcLoc& src_loc, const std::string& name )
{
	// TODO - count UTF-32 points, not UTF-8.
	const uint32_t size= uint32_t(name.size());
	return DocumentRange{ { src_loc.GetLine(), src_loc.GetColumn() }, { src_loc.GetLine(), src_loc.GetColumn() + size } };
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

std::vector<Symbol> BuildProgramModel_r( const Synt::Enum& enum_ )
{
	std::vector<Symbol> result;

	for( const Synt::Enum::Member& member : enum_.members )
	{
		Symbol symbol;
		symbol.name= member.name;
		symbol.range= MakeRangeInitial( member.src_loc );
		symbol.selection_range= MakeSelectionRange( member.src_loc, member.name );
		symbol.kind= SymbolKind::EnumMember;
		result.push_back(symbol);
	}

	return result;
}

std::vector<Symbol> BuildProgramModel_r( const Synt::ClassElements& elements );
std::vector<Symbol> BuildProgramModel_r( const Synt::ProgramElements& elements );

struct Visitor final
{
	std::vector<Symbol> result;

	void operator()( const Synt::ClassField& class_field_ )
	{
		Symbol symbol;
		symbol.name= Stringify( class_field_ );
		symbol.range= MakeRangeInitial( class_field_.src_loc_ );
		symbol.selection_range= MakeSelectionRange( class_field_.src_loc_, class_field_.name );
		symbol.kind= SymbolKind::Field;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::FunctionPtr& func )
	{
		if( func == nullptr || func->name_.empty() )
			return;

		Symbol symbol;
		symbol.name= Stringify( *func );
		symbol.range= MakeRangeInitial( func->src_loc_ );
		symbol.selection_range= MakeSelectionRange( func->src_loc_, func->name_.back().name );
		symbol.kind= SymbolKind::Function;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::FunctionTemplate& func_template )
	{
		if( func_template.function_ == nullptr || func_template.function_->name_.empty() )
			return;

		Symbol symbol;
		symbol.name= Stringify( func_template );
		symbol.range= MakeRangeInitial( func_template.src_loc_ );
		symbol.selection_range= MakeSelectionRange( func_template.function_->src_loc_, func_template.function_->name_.back().name );
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
		symbol.range= MakeRangeInitial( type_template.src_loc_ );
		symbol.selection_range= symbol.range; // TODO - set it properly.
		symbol.kind= SymbolKind::Class;

		if( const auto class_= std::get_if<Synt::ClassPtr>( &type_template.something_ ) )
		{
			symbol.children= BuildProgramModel_r( (*class_)->elements_ );
		}
		if( std::get_if<std::unique_ptr<const Synt::TypeAlias>>( &type_template.something_ ) != nullptr )
		{}

		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::Enum& enum_ )
	{
		Symbol symbol;
		symbol.name= enum_.name;
		symbol.range= MakeRangeInitial( enum_.src_loc_ );
		symbol.selection_range= MakeSelectionRange( enum_.src_loc_, enum_.name );
		symbol.kind= SymbolKind::Enum;
		symbol.children= BuildProgramModel_r( enum_ );
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::StaticAssert&  )
	{
	}
	void operator()( const Synt::TypeAlias& typedef_ )
	{
		Symbol symbol;
		symbol.name= typedef_.name;
		symbol.range= MakeRangeInitial( typedef_.src_loc_ );
		symbol.selection_range= MakeSelectionRange( typedef_.src_loc_, typedef_.name );
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
			symbol.range= MakeRangeInitial( variable.src_loc );
			symbol.selection_range= MakeSelectionRange( variable.src_loc, variable.name );
			symbol.kind= SymbolKind::Variable;
			result.push_back( std::move(symbol) );
		}
	}
	void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
	{
		Symbol symbol;
		symbol.name= Stringify( auto_variable_declaration );
		symbol.range= MakeRangeInitial( auto_variable_declaration.src_loc_ );
		symbol.selection_range= MakeSelectionRange( auto_variable_declaration.src_loc_, auto_variable_declaration.name );
		symbol.kind= SymbolKind::Variable;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::ClassPtr& class_ )
	{
		if( class_ == nullptr )
			return;

		Symbol symbol;
		symbol.name= class_->name_;
		symbol.range= MakeRangeInitial( class_->src_loc_ );
		symbol.selection_range= MakeSelectionRange( class_->src_loc_, class_->name_ );
		symbol.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? SymbolKind::Struct : SymbolKind::Class;
		symbol.children= BuildProgramModel_r( class_->elements_ );
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::NamespacePtr& namespace_ )
	{
		if( namespace_ == nullptr )
			return;

		Symbol symbol;
		symbol.name= namespace_->name_;
		symbol.range= MakeRangeInitial( namespace_->src_loc_ );
		symbol.selection_range= MakeSelectionRange( namespace_->src_loc_, namespace_->name_ );
		symbol.kind= SymbolKind::Namespace;
		symbol.children= BuildProgramModel_r( namespace_->elements_ );
		result.push_back( std::move(symbol) );
	}
};

std::vector<Symbol> BuildProgramModel_r( const Synt::ClassElements& elements )
{
	Visitor visitor;
	for( const Synt::ClassElement& class_element : elements )
		std::visit( visitor, class_element );

	return std::move(visitor.result);
}

std::vector<Symbol> BuildProgramModel_r( const Synt::ProgramElements& elements )
{
	Visitor visitor;
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

std::vector<Symbol> BuildSymbols( const Synt::ProgramElements& program_elements )
{
	// TODO - include also macros.
	auto result= BuildProgramModel_r( program_elements );
	SetupRanges(result);
	return result;
}


} // namespace U
