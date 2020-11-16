#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "src_loc.hpp"

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

enum class ErrorsFormat{ GCC, MSVC };


void PrintLexSyntErrors( const std::vector<std::string>& source_files, const LexSyntErrors& errors, ErrorsFormat format= ErrorsFormat::GCC, std::ostream& errors_stream= std::cerr );


} // namespace U
