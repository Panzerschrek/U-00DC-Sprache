#pragma once
#include <memory>

#include <QString>

#include "../Compiler/src/program_string.hpp"
#include "../Compiler/src/lexical_analyzer.hpp"

namespace U
{

namespace QtCreatorPlugin
{

struct ProgramModel
{
	struct ProgramTreeNode
	{
		QString name;

		// Elements in order of original file.
		std::vector<ProgramTreeNode> childs;

		ProgramTreeNode* parent= nullptr;
		size_t number_in_parent= 0;
		FilePos file_pos= FilePos{ 0, 0, 0 };
	};

	// Elements in order of original file.
	std::vector<ProgramTreeNode> program_elements;
};

using ProgramModelPtr= std::shared_ptr<ProgramModel>;

ProgramModelPtr BuildProgramModel( const ProgramString& program_text );

} // namespace QtCreatorPlugin

} // namespace U
