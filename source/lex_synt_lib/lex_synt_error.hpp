#pragma once
#include <string>
#include <vector>
#include "file_pos.hpp"

namespace U
{

struct LexSyntError
{
	std::string text;
	FilePos file_pos;

	LexSyntError()= default;
	LexSyntError( std::string in_text, FilePos in_file_pos )
		: text(std::move(in_text)), file_pos(std::move(in_file_pos))
	{}
};

using LexSyntErrors= std::vector<LexSyntError>;

} // namespace U
