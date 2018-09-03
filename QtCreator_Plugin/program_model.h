#pragma once
#include <memory>

#include <QString>

#include "../Compiler/src/program_string.hpp"
#include "../Compiler/src/lexical_analyzer.hpp"
#include "../Compiler/src/syntax_elements.hpp"

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
		Namespace,
		Class,
		Enum,
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

ProgramModelPtr BuildProgramModel( const ProgramString& program_text );

} // namespace QtCreatorPlugin

} // namespace U
