#include "../Compiler/src/lexical_analyzer.hpp"
#include "../Compiler/src/program_string.hpp"
#include "../Compiler/src/syntax_analyzer.hpp"

#include "program_model.h"

namespace U
{

namespace QtCreatorPlugin
{

static std::vector<ProgramModel::ProgramTreeNode> BuildProgramModel_r( const Synt::ProgramElements& elements )
{
	std::vector<ProgramModel::ProgramTreeNode> result;

	for( const Synt::IProgramElementPtr& program_element : elements )
	{
		if( const auto namespace_= dynamic_cast<const Synt::Namespace*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromUtf8( ToUTF8( namespace_->name_).data() );
			element.childs= BuildProgramModel_r( namespace_->elements_ );
			element.number_in_parent= result.size();
			result.push_back(element);
		}
		else if( const auto class_= dynamic_cast<const Synt::Class*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromUtf8( ToUTF8( class_->name_ ).data() );
			element.number_in_parent= result.size();
			result.push_back(element);
		}
		else if( const auto function_= dynamic_cast<const Synt::Function*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromUtf8( ToUTF8( function_->name_.components.back().name ).data() );
			element.number_in_parent= result.size();
			result.push_back(element);
		}
		else if( const auto variables_= dynamic_cast<const Synt::VariablesDeclaration*>( program_element.get() ) )
		{
			for( const auto& variable : variables_->variables )
			{
				ProgramModel::ProgramTreeNode element;
				element.name= QString::fromUtf8( ToUTF8( variable.name ).data() );
				element.number_in_parent= result.size();
				result.push_back(element);
			}
		}
		else if( const auto auto_variable_= dynamic_cast<const Synt::AutoVariableDeclaration*>( program_element.get() ) )
		{
			ProgramModel::ProgramTreeNode element;
			element.name= QString::fromUtf8( ToUTF8( auto_variable_->name ).data() );
			element.number_in_parent= result.size();
			result.push_back(element);
		}
	}

	return result;
}

static void SetupParents( ProgramModel::ProgramTreeNode& tree )
{
	for( ProgramModel::ProgramTreeNode& child : tree.childs )
	{
		child.parent= &tree;
		SetupParents(child);
	}
}

ProgramModelPtr BuildProgramModel( const ProgramString& program_text )
{

	const U::LexicalAnalysisResult lex_result= U::LexicalAnalysis( program_text );
	if( !lex_result.error_messages.empty() )
		return nullptr;

	U::Synt::SyntaxAnalysisResult synt_result= U::Synt::SyntaxAnalysis( lex_result.lexems );
	if( !synt_result.error_messages.empty() )
		return nullptr;

	const auto result= std::make_shared<ProgramModel>();
	result->program_elements= BuildProgramModel_r( synt_result.program_elements );

	for( ProgramModel::ProgramTreeNode& node : result->program_elements )
		SetupParents( node );

	return result;
}

} // namespace QtCreatorPlugin

} // namespace U
