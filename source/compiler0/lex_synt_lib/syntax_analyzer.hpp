#pragma once
#include <memory>
#include <vector>

#include "../../lex_synt_lib_common/lex_synt_error.hpp"
#include "macro.hpp"
#include "syntax_elements.hpp"

namespace U
{

namespace Synt
{

struct MacroExpansionContext
{
	std::string macro_name;
	SrcLoc macro_declaration_file_pos;
	SrcLoc file_pos; // Contains also macro expansion context of parent macro.
};

using MacroExpansionContexts= std::vector<MacroExpansionContext>;
using MacroExpansionContextsPtr = std::shared_ptr<MacroExpansionContexts>;

struct SyntaxAnalysisResult
{
	std::vector<Import> imports;
	MacrosPtr macros;
	ProgramElements program_elements;
	LexSyntErrors error_messages;
};

std::vector<Import> ParseImports( const Lexems& lexems );
SyntaxAnalysisResult SyntaxAnalysis(
	const Lexems& lexems,
	MacrosByContextMap macros,
	const MacroExpansionContextsPtr& macro_expansion_contexts /* in-out contexts */ );

} // namespace Synt

} // namespace U
