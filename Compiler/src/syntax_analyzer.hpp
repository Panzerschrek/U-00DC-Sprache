#pragma once
#include <memory>
#include <vector>

#include "syntax_elements.hpp"

namespace U
{

typedef std::string SyntaxErrorMessage;
typedef std::vector<SyntaxErrorMessage> SyntaxErrorMessages;

struct SyntaxAnalysisResult
{
	ProgramElements program_elements;
	SyntaxErrorMessages error_messages;
};

SyntaxAnalysisResult SyntaxAnalysis( const Lexems& lexems );

} // namespace U
