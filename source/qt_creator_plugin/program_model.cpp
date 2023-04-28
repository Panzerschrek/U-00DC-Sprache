#include <sstream>
#include "../lex_synt_lib_common/assert.hpp"
#include "keywords.hpp"
#include "../compiler0/lex_synt_lib/lexical_analyzer.hpp"
#include "../compiler0/lex_synt_lib/program_string.hpp"
#include "../compiler0/lex_synt_lib/program_writer.hpp"
#include "../compiler0/lex_synt_lib/syntax_analyzer.hpp"

#include "program_model.hpp"

namespace U
{

namespace QtCreatorPlugin
{

namespace
{

std::string Stringify( const Synt::Expression& expression )
{
	std::stringstream ss;
	Synt::WriteExpression( expression, ss );
	return ss.str();
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

std::string Stringify( const Synt::TypeTemplate& type_template )
{
	std::string result= type_template.name_;

	result+= "</";
	if( type_template.is_short_form_ )
	{
		for( const Synt::TypeTemplate::Param& param : type_template.params_ )
		{
			if( param.param_type != std::nullopt )
			{
				//result+= Stringify( *param.param_type );
			}
			else
			{
				result+= Keyword( Keywords::type_ );
			}
			result+= param.name;
			if( &param != &type_template.params_.back() )
				result+= ", ";
		}
	}
	else
	{
		for( const Synt::TypeTemplate::SignatureParam& param : type_template.signature_params_ )
		{
			result+= Stringify( param.name );
			if( std::get_if<Synt::EmptyVariant>(&param.default_value) != nullptr )
			{
				result+= "= ";
				result+= Stringify( param.default_value );
			}
			if( &param != &type_template.signature_params_.back() )
				result+= ", ";
		}
	}
	result+= "/>";

	return result;
}

std::string Stringify( const Synt::FunctionTemplate& function_template )
{
	std::string result;

	result+= Keyword( Keywords::template_ );
	result+= "</";
	for( const Synt::TemplateBase::Param& param : function_template.params_ )
	{
		if( !param.name.empty() )
			result+= param.name;
		if( &param != &function_template.params_.back() )
			result+= ", ";
	}
	result+= "/>";

	result+= " ";
	if( function_template.function_ != nullptr )
		result+= Stringify( *function_template.function_ );
	return result;
}

std::string Stringify( const Synt::ClassField& class_field )
{
	std::string result= class_field.name;
	result+= ": " + Stringify( class_field.type ) + " ";

	switch( class_field.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"; break;
	}

	switch( class_field.mutability_modifier )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ );break;
	}

	return result;
}

std::string Stringify( const Synt::VariablesDeclaration::VariableEntry& varaible, const std::string& type )
{
	std::string result= varaible.name;
	result+= ": " + type + " ";

	switch( varaible.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"; break;
	}

	switch( varaible.mutability_modifier )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ );break;
	}

	return result;
}

std::string Stringify( const Synt::AutoVariableDeclaration& varaible )
{
	std::string result= varaible.name;
	result+= ": " + Keyword( Keywords::auto_ ) + " ";

	switch( varaible.reference_modifier )
	{
	case Synt::ReferenceModifier::None: break;
	case Synt::ReferenceModifier::Reference: result+= "&"; break;
	}

	switch( varaible.mutability_modifier )
	{
	case Synt::MutabilityModifier::None: break;
	case Synt::MutabilityModifier::Mutable  : result+= Keyword( Keywords::mut_       ); break;
	case Synt::MutabilityModifier::Immutable: result+= Keyword( Keywords::imut_      ); break;
	case Synt::MutabilityModifier::Constexpr: result+= Keyword( Keywords::constexpr_ );break;
	}

	return result;
}

std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::Enum& enum_ )
{
	std::vector<ProgramModel::ProgramTreeNode> result;

	for( const Synt::Enum::Member& member : enum_.members )
	{
		ProgramModel::ProgramTreeNode element;
		element.name= QString::fromStdString( member.name );
		element.kind= ProgramModel::ElementKind::EnumElement;
		element.number_in_parent= result.size();
		element.src_loc= member.src_loc;
		result.push_back(element);
	}

	return result;
}

std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::ClassElements& elements )
{
	struct Visitor final
	{
		std::vector<ProgramModel::ProgramTreeNode> result;
		ProgramModel::Visibility current_visibility= ProgramModel::Visibility::Public;

		void operator()( const Synt::ClassField& class_field_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( class_field_ ) );
			element.kind= ProgramModel::ElementKind::ClassField;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.src_loc= class_field_.src_loc_;
			result.push_back(element);
		}

		void operator()( const Synt::FunctionPtr& func )
		{
			if( func == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( *func ) );
			element.kind= ProgramModel::ElementKind::Function;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.src_loc= func->src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::FunctionTemplate& func_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( func_template ) );
			element.kind= ProgramModel::ElementKind::FunctionTemplate;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.src_loc= func_template.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::ClassVisibilityLabel& visibility_label )
		{
			current_visibility= visibility_label.visibility_;
		}
		void operator()( const Synt::TypeTemplate& type_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( type_template ) );

			if( const auto class_= std::get_if<Synt::ClassPtr>( &type_template.something_ ) )
			{
				element.kind= ProgramModel::ElementKind::ClassTemplate;
				element.children= BuildProgramModel_r( (*class_)->elements_ );
			}
			if( std::get_if<std::unique_ptr<const Synt::TypeAlias>>( &type_template.something_ ) != nullptr )
			{
				element.kind= ProgramModel::ElementKind::TypeAliasTemplate;
			}
			element.number_in_parent= result.size();
			element.src_loc= type_template.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::Enum& enum_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( enum_.name );
			element.kind= ProgramModel::ElementKind::Enum;
			element.children= BuildProgramModel_r( enum_ );
			element.number_in_parent= result.size();
			element.src_loc= enum_.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::StaticAssert&  )
		{
		}
		void operator()( const Synt::TypeAlias& typedef_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( typedef_.name );
			element.kind= ProgramModel::ElementKind::TypeAloas;
			element.number_in_parent= result.size();
			element.src_loc= typedef_.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::VariablesDeclaration& variables_declaration )
		{
			const std::string type_name= Stringify( variables_declaration.type );
			for( const auto& variable : variables_declaration.variables )
			{
				ProgramModel::ProgramTreeNode element;
				element.name= QString::fromStdString( Stringify( variable, type_name ) );
				element.kind= ProgramModel::ElementKind::Variable;
				element.visibility= current_visibility;
				element.number_in_parent= result.size();
				element.src_loc= variable.src_loc;
				result.push_back(element);
			}
		}
		void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( auto_variable_declaration ) );
			element.kind= ProgramModel::ElementKind::Variable;
			element.visibility= current_visibility;
			element.number_in_parent= result.size();
			element.src_loc= auto_variable_declaration.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::ClassPtr& class_ )
		{
			if( class_ == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( class_->name_ );
			element.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? ProgramModel::ElementKind::Struct : ProgramModel::ElementKind::Class;
			element.children= BuildProgramModel_r( class_->elements_ );
			element.number_in_parent= result.size();
			element.src_loc= class_->src_loc_;
			result.push_back(element);
		}
	};

	Visitor visitor;
	for( const Synt::ClassElement& class_element : elements )
		std::visit( visitor, class_element );

	return std::move(visitor.result);
}

std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::ProgramElements& elements )
{
	struct Visitor final
	{
		std::vector<ProgramModel::ProgramTreeNode> result;

		void operator()( const Synt::ClassField& class_field_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( class_field_ ) );
			element.kind= ProgramModel::ElementKind::ClassField;
			element.number_in_parent= result.size();
			element.src_loc= class_field_.src_loc_;
			result.push_back(element);
		}

		void operator()( const Synt::FunctionPtr& func )
		{
			if( func == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( *func ).data() );
			element.kind= ProgramModel::ElementKind::Function;
			element.number_in_parent= result.size();
			element.src_loc= func->src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::FunctionTemplate& func_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( func_template ) );
			element.kind= ProgramModel::ElementKind::FunctionTemplate;
			element.number_in_parent= result.size();
			element.src_loc= func_template.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::TypeTemplate& type_template )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( type_template ) );

			if( const auto class_= std::get_if<Synt::ClassPtr>( &type_template.something_ ) )
			{
				element.kind= ProgramModel::ElementKind::ClassTemplate;
				element.children= BuildProgramModel_r( (*class_)->elements_ );
			}
			if( std::get_if<std::unique_ptr<const Synt::TypeAlias>>( &type_template.something_ ) != nullptr )
			{
				element.kind= ProgramModel::ElementKind::TypeAliasTemplate;
			}
			element.number_in_parent= result.size();
			element.src_loc= type_template.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::Enum& enum_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( enum_.name );
			element.kind= ProgramModel::ElementKind::Enum;
			element.children= BuildProgramModel_r( enum_ );
			element.number_in_parent= result.size();
			element.src_loc= enum_.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::StaticAssert&  )
		{
		}
		void operator()( const Synt::TypeAlias& typedef_ )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( typedef_.name );
			element.kind= ProgramModel::ElementKind::TypeAloas;
			element.number_in_parent= result.size();
			element.src_loc= typedef_.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::VariablesDeclaration& variables_declaration )
		{
			const std::string type_name= Stringify( variables_declaration.type );
			for( const auto& variable : variables_declaration.variables )
			{
				ProgramModel::ProgramTreeNode element;
				element.name= QString::fromStdString( Stringify( variable, type_name ) );
				element.kind= ProgramModel::ElementKind::Variable;
				element.number_in_parent= result.size();
				element.src_loc= variable.src_loc;
				result.push_back(element);
			}
		}
		void operator()( const Synt::AutoVariableDeclaration& auto_variable_declaration )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( Stringify( auto_variable_declaration ) );
			element.kind= ProgramModel::ElementKind::Variable;
			element.number_in_parent= result.size();
			element.src_loc= auto_variable_declaration.src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::ClassPtr& class_ )
		{
			if( class_ == nullptr )
				return;

			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( class_->name_ );
			element.kind= class_->kind_attribute_ == Synt::ClassKindAttribute::Struct ? ProgramModel::ElementKind::Struct : ProgramModel::ElementKind::Class;
			element.children= BuildProgramModel_r( class_->elements_ );
			element.number_in_parent= result.size();
			element.src_loc= class_->src_loc_;
			result.push_back(element);
		}
		void operator()( const Synt::NamespacePtr& namespace_ )
		{
			if( namespace_ == nullptr )
				return;

			// TODO - what if there are multiple declarations of same namespace in single source file?
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromStdString( namespace_->name_ );
			element.kind= ProgramModel::ElementKind::Namespace;
			element.children= BuildProgramModel_r( namespace_->elements_ );
			element.number_in_parent= result.size();
			element.src_loc= namespace_->src_loc_;
			result.push_back(element);
		}

	};

	Visitor visitor;
	for( const Synt::ProgramElement& program_element : elements )
		std::visit( visitor, program_element );

	return std::move(visitor.result);
}

std::vector<ProgramModel::ProgramTreeNode> BuildProgramModelMacros( const Synt::MacrosPtr& macros )
{
	std::vector<ProgramModel::ProgramTreeNode> result;
	if( macros == nullptr )
		return result;

	for( const auto& constex_macro_map_pair : *macros )
	{
		for( const auto& name_macro_pair : constex_macro_map_pair.second )
		{
			ProgramModel::ProgramTreeNode node;
			node.kind= ProgramModel::ElementKind::Macro;
			node.name= QString::fromStdString( name_macro_pair.second.name );
			node.src_loc= name_macro_pair.second.src_loc;
			result.push_back(node);
		}
	}

	std::sort(
		result.begin(),
		result.end(),
		[](const ProgramModel::ProgramTreeNode& l, const ProgramModel::ProgramTreeNode& r ) -> bool
		{
			return l.src_loc < r.src_loc;
		});

	return result;
}

void SetupParents( ProgramModel::ProgramTreeNode& tree )
{
	size_t i= 0u;
	for( ProgramModel::ProgramTreeNode& child : tree.children )
	{
		child.parent= &tree;
		child.number_in_parent= i;
		++i;
		SetupParents(child);
	}
}

const ProgramModel::ProgramTreeNode* GetNode_r( const std::vector<ProgramModel::ProgramTreeNode>& nodes, const SrcLoc& src_loc )
{
	// TODO - optimize, make O(log(n)), instead of O(n).
	for( size_t i= 0u; i < nodes.size(); ++i )
	{
		const ProgramModel::ProgramTreeNode& node= nodes[i];

		SrcLoc next_src_loc;
		if( i + 1u < nodes.size() )
			next_src_loc= nodes[i+1u].src_loc;
		else
			next_src_loc= SrcLoc( 0u, SrcLoc::c_max_line, SrcLoc::c_max_column );

		if( node.src_loc <= src_loc && src_loc < next_src_loc )
		{
			const ProgramModel::ProgramTreeNode* const child_node= GetNode_r( node.children, src_loc );
			if( child_node != nullptr )
				return child_node;
			return &node;
		}
	}

	return nullptr;
}

} // namespace

const ProgramModel::ProgramTreeNode* ProgramModel::GetNodeForSrcLoc( const SrcLoc& src_loc ) const
{
	return GetNode_r( program_elements, src_loc );
}

ProgramModelPtr BuildProgramModel( const QString& program_text )
{
	const U::LexicalAnalysisResult lex_result= U::LexicalAnalysis( program_text.toStdString() );
	if( !lex_result.errors.empty() )
		return nullptr;

	const Synt::SyntaxAnalysisResult synt_result=
		Synt::SyntaxAnalysis(
			lex_result.lexems,
			Synt::MacrosByContextMap(),
			std::make_shared<Synt::MacroExpansionContexts>(), "" );
	// Do NOT abort on errors, because in process of source code editing may occurs some errors.

	const auto result= std::make_shared<ProgramModel>();

	result->program_elements= BuildProgramModelMacros( synt_result.macros );

	auto program_elements= BuildProgramModel_r( synt_result.program_elements );
	for( auto& element : program_elements )
		result->program_elements.push_back(std::move(element));

	size_t i= 0u;
	for( ProgramModel::ProgramTreeNode& node : result->program_elements )
	{
		node.number_in_parent= i;
		++i;
		SetupParents( node );
	}

	return result;
}

} // namespace QtCreatorPlugin

} // namespace U
