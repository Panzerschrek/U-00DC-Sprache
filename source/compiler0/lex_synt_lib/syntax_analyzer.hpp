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
	SrcLoc macro_declaration_src_loc;
	SrcLoc src_loc; // Contains also macro expansion context of parent macro.
};

using MacroExpansionContexts= std::vector<MacroExpansionContext>;
using MacroExpansionContextsPtr = std::shared_ptr<MacroExpansionContexts>;

struct SyntaxAnalysisResult
{
	std::vector<Import> imports;
	MacrosPtr macros;
	ProgramElementsList program_elements;
	LexSyntErrors error_messages;
};

struct NamespaceParsingResult
{
	ProgramElementsList namespace_elements;
	LexSyntErrors error_messages;
};

std::vector<Import> ParseImports( const Lexems& lexems );
SyntaxAnalysisResult SyntaxAnalysis(
	const Lexems& lexems,
	MacrosByContextMap macros,
	const MacroExpansionContextsPtr& macro_expansion_contexts, /* in-out contexts */
	std::string source_file_contents_hash );

NamespaceParsingResult ParseNamespaceElements(
	const Lexems& lexems,
	MacrosByContextMap macros,
	const MacroExpansionContextsPtr& macro_expansion_contexts, /* in-out contexts */
	std::string source_file_contents_hash );

} // namespace Synt

} // namespace U
