#pragma once
#include <memory>

#include <QString>

#include "../Compiler/src/program_string.hpp"

namespace U
{

namespace QtCreatorPlugin
{

struct ProgramModel
{
	struct ProgramTreeNode
	{
		QString name;
		std::vector<ProgramTreeNode> childs;
		ProgramTreeNode* parent= nullptr;
		size_t number_in_parent= 0;
		int line= 0, pos_in_line= 0;
	};

	std::vector<ProgramTreeNode> program_elements;
};

using ProgramModelPtr= std::shared_ptr<ProgramModel>;

ProgramModelPtr BuildProgramModel( const ProgramString& program_text );

} // namespace QtCreatorPlugin

} // namespace U
