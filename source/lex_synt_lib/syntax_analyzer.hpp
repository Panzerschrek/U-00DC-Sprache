#pragma once
#include <memory>
#include <vector>

#include "syntax_elements.hpp"

namespace U
{

namespace Synt
{

typedef std::string SyntaxErrorMessage;
typedef std::vector<SyntaxErrorMessage> SyntaxErrorMessages;

struct SyntaxAnalysisResult
{
	std::vector<Import> imports;
	ProgramElements program_elements;
	SyntaxErrorMessages error_messages;
};

SyntaxAnalysisResult SyntaxAnalysis( const Lexems& lexems );

} // namespace Synt

} // namespace U
