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
	std::string text;
	FilePos file_pos;
};

using SyntaxErrorMessages= std::vector<SyntaxErrorMessage>;

struct MacroExpansionContext
{
	std::string macro_name;
	FilePos file_pos; // Contains also macro expansion context of parent macro.
};

using MacroExpansionContexts= std::vector<MacroExpansionContext>;
using MacroExpansionContextsPtr = std::shared_ptr<MacroExpansionContexts>;

struct SyntaxAnalysisResult
{
	std::vector<Import> imports;
	MacrosPtr macros;
	MacroExpansionContextsPtr macro_expansion_contexts;
	ProgramElements program_elements;
	SyntaxErrorMessages error_messages;
};

std::vector<Import> ParseImports( const Lexems& lexems );
SyntaxAnalysisResult SyntaxAnalysis(
	const Lexems& lexems,
	MacrosByContextMap macros,
	const MacroExpansionContextsPtr& macro_expansion_contexts /* in-out contexts */ );

} // namespace Synt

} // namespace U
