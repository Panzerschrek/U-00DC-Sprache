#pragma once
#include <memory>
#include <vector>

#include "macro.hpp"
#include "syntax_elements.hpp"

namespace U
{

namespace Synt
{

struct SyntaxErrorMessage
{
	ProgramString text;
	FilePos file_pos;
};

using SyntaxErrorMessages= std::vector<SyntaxErrorMessage>;

struct SyntaxAnalysisResult
{
	std::vector<Import> imports;
	MacrosPtr macros;
	ProgramElements program_elements;
	SyntaxErrorMessages error_messages;
};

std::vector<Import> ParseImports( const Lexems& lexems );
SyntaxAnalysisResult SyntaxAnalysis( const Lexems& lexems, const MacrosPtr& macros );

} // namespace Synt

} // namespace U
