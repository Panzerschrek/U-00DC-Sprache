#pragma once
#include <memory>

#include <QString>

#include "../lex_synt_lib/program_string.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"
#include "../lex_synt_lib/syntax_elements.hpp"

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
		ClassFiled,
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
		FilePos file_pos= FilePos{ 0, 0, 0 };
	};

public:
	// Elements in order of original file.
	std::vector<ProgramTreeNode> program_elements;

public:
	const ProgramModel::ProgramTreeNode* GetNodeForFilePos( const FilePos& file_pos ) const;
};

using ProgramModelPtr= std::shared_ptr<ProgramModel>;

ProgramModelPtr BuildProgramModel( const QString& program_text );

} // namespace QtCreatorPlugin

} // namespace U
