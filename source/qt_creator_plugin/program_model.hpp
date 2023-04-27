#pragma once
#include <memory>

#include <QString>

#include "../compiler0/lex_synt_lib/program_string.hpp"
#include "../compiler0/lex_synt_lib/lexical_analyzer.hpp"
#include "../compiler0/lex_synt_lib/syntax_elements.hpp"

namespace U
{

namespace QtCreatorPlugin
{

struct ProgramModel
{
public:
	enum class ElementKind
	{
		Unknown,
		Macro,
		Namespace,
		Class,
		Struct,
		Enum,
		EnumElement,
		Function,
		ClassField,
		Variable,
		ClassTemplate,
		Typedef,
		TypedefTemplate,
		FunctionTemplate,
	};

	using Visibility= Synt::ClassMemberVisibility;

	struct ProgramTreeNode
	{
		QString name;
		ElementKind kind= ElementKind::Unknown;
		Visibility visibility= Visibility::Public;

		// Elements in order of original file.
		std::vector<ProgramTreeNode> childs;

		ProgramTreeNode* parent= nullptr;
		size_t number_in_parent= 0;
		SrcLoc src_loc;
	};

public:
	// Elements in order of original file.
	std::vector<ProgramTreeNode> program_elements;

public:
	const ProgramModel::ProgramTreeNode* GetNodeForSrcLoc( const SrcLoc& src_loc ) const;
};

using ProgramModelPtr= std::shared_ptr<ProgramModel>;

ProgramModelPtr BuildProgramModel( const QString& program_text );

} // namespace QtCreatorPlugin

} // namespace U
