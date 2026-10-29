#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "../../lex_synt_lib_common/src_loc.hpp"

namespace U
{

struct LexSyntError
{
	std::string text;
	SrcLoc src_loc;

	LexSyntError()= default;
	LexSyntError( std::string in_text, SrcLoc in_src_loc )
		: text(std::move(in_text)), src_loc(std::move(in_src_loc))
	{}
};

using LexSyntErrors= std::vector<LexSyntError>;

} // namespace U
