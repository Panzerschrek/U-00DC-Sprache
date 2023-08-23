#include <sstream>
#include "../lex_synt_lib_common/assert.hpp"
#include "../compiler0/lex_synt_lib/program_writer.hpp"
#include "keywords.hpp"

#include "document_symbols.hpp"

namespace U
{

namespace
{

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

	switch( class_field.mutability_modifier )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : ss << Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: ss << Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: ss << Keyword( Keywords::constexpr_ );break;
	}

	return ss.str();
}

std::string Stringify( const Synt::VariablesDeclaration::VariableEntry& varaible, const std::string& type )
{
	std::stringstream ss;
	ss << varaible.name << ": " << type << " ";

	switch( varaible.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: ss << "&"; break;
	}

	switch( varaible.mutability_modifier )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : ss << Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: ss << Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: ss << Keyword( Keywords::constexpr_ );break;
	}

	return ss.str();
}

std::string Stringify( const Synt::AutoVariableDeclaration& varaible )
{
	std::stringstream ss;
	ss << varaible.name << ": " << Keyword( Keywords::auto_ ) << " ";

	switch( varaible.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: ss << "&"; break;
	}

	switch( varaible.mutability_modifier )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : ss << Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: ss << Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: ss << Keyword( Keywords::constexpr_ );break;
	}

	return ss.str();
}

std::vector<CodeBuilder::Symbol> BuildProgramModel_r( const Synt::Enum& enum_ )
{
	std::vector<CodeBuilder::Symbol> result;

	for( const Synt::Enum::Member& member : enum_.members )
	{
		CodeBuilder::Symbol element;
		element.name= member.name;
		element.kind= CodeBuilder::SymbolKind::EnumMember;
		element.src_loc= member.src_loc;
		result.push_back(element);
	}

	return result;
}

std::vector<CodeBuilder::Symbol> BuildProgramModel_r( const Synt::ClassElements& elements );
std::vector<CodeBuilder::Symbol> BuildProgramModel_r( const Synt::ProgramElements& elements );

struct Visitor final
{
	std::vector<CodeBuilder::Symbol> result;

	void operator()( const Synt::ClassField& class_field_ )
	{
		CodeBuilder::Symbol symbol;
		symbol.name= Stringify( class_field_ );
		symbol.kind= CodeBuilder::SymbolKind::Field;
		symbol.src_loc= class_field_.src_loc_;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::FunctionPtr& func )
	{
		if( func == nullptr )
			return;

		CodeBuilder::Symbol symbol;
		symbol.name= Stringify( *func );
		symbol.kind= CodeBuilder::SymbolKind::Function;
		symbol.src_loc= func->src_loc_;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::FunctionTemplate& func_template )
	{
		CodeBuilder::Symbol symbol;
		symbol.name= Stringify( func_template );
		symbol.kind= CodeBuilder::SymbolKind::Function;
		symbol.src_loc= func_template.src_loc_;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::ClassVisibilityLabel& )
	{
	}
	void operator()( const Synt::TypeTemplate& type_template )
	{
		CodeBuilder::Symbol symbol;
		symbol.name= Stringify( type_template );

		if( const auto class_= std::get_if<Synt::ClassPtr>( &type_template.something_ ) )
		{
			symbol.kind= CodeBuilder::SymbolKind::Class;
			symbol.children= BuildProgramModel_r( (*class_)->elements_ );
		}
		if( std::get_if<std::unique_ptr<const Synt::TypeAlias>>( &type_template.something_ ) != nullptr )
		{
			symbol.kind= CodeBuilder::SymbolKind::Class;
		}

		symbol.src_loc= type_template.src_loc_;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::Enum& enum_ )
	{
		CodeBuilder::Symbol symbol;
		symbol.name= enum_.name;
		symbol.kind= CodeBuilder::SymbolKind::Enum;
		symbol.children= BuildProgramModel_r( enum_ );
		symbol.src_loc= enum_.src_loc_;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::StaticAssert&  )
	{
	}
	void operator()( const Synt::TypeAlias& typedef_ )
	{
		CodeBuilder::Symbol symbol;
		symbol.name= typedef_.name;
		symbol.kind= CodeBuilder::SymbolKind::Class;
		symbol.src_loc= typedef_.src_loc_;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::VariablesDeclaration& variables_declaration )
	{
		const std::string type_name= Stringify( variables_declaration.type );
		for( const auto& variable : variables_declaration.variables )
		{
			CodeBuilder::Symbol symbol;
			symbol.name= Stringify( variable, type_name );
			symbol.kind= CodeBuilder::SymbolKind::Variable;

			symbol.src_loc= variable.src_loc;
			result.push_back( std::move(symbol) );
		}
	}
	void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
	{
		CodeBuilder::Symbol symbol;
		symbol.name= Stringify( auto_variable_declaration );
		symbol.kind= CodeBuilder::SymbolKind::Variable;
		symbol.src_loc= auto_variable_declaration.src_loc_;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::ClassPtr& class_ )
	{
		if( class_ == nullptr )
			return;

		CodeBuilder::Symbol symbol;
		symbol.name= class_->name_;
		symbol.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? CodeBuilder::SymbolKind::Struct : CodeBuilder::SymbolKind::Class;
		symbol.children= BuildProgramModel_r( class_->elements_ );
		symbol.src_loc= class_->src_loc_;
		result.push_back( std::move(symbol) );
	}
	void operator()( const Synt::NamespacePtr& namespace_ )
	{
		if( namespace_ == nullptr )
			return;

		// TODO - what if there are multiple declarations of same namespace in single source file?
		CodeBuilder::Symbol symbol;
		symbol.name= namespace_->name_;
		symbol.kind= CodeBuilder::SymbolKind::Namespace;
		symbol.children= BuildProgramModel_r( namespace_->elements_ );
		symbol.src_loc= namespace_->src_loc_;
		result.push_back( std::move(symbol) );
	}
};

std::vector<CodeBuilder::Symbol> BuildProgramModel_r( const Synt::ClassElements& elements )
{
	Visitor visitor;
	for( const Synt::ClassElement& class_element : elements )
		std::visit( visitor, class_element );

	return std::move(visitor.result);
}

std::vector<CodeBuilder::Symbol> BuildProgramModel_r( const Synt::ProgramElements& elements )
{
	Visitor visitor;
	for( const Synt::ProgramElement& program_element : elements )
		std::visit( visitor, program_element );

	return std::move(visitor.result);
}

} // namespace

std::vector<CodeBuilder::Symbol> BuildSymbols( const Synt::ProgramElements& program_elements )
{
	// TODO - include also macros.
	return BuildProgramModel_r( program_elements );
}


} // namespace U
