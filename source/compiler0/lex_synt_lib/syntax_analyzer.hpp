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

struct ClassElementsParsingResult
{
	ClassElementsList class_elements;
	LexSyntErrors error_messages;
};

struct BlockElementsParsingResult
{
	BlockElementsList block_elements;
	LexSyntErrors error_messages;
};

struct TypeNameParsingResult
{
	TypeName type_name;
	LexSyntErrors error_messages;
};

struct ExpressionParsingResult
{
	Expression expression;
	LexSyntErrors error_messages;
};

std::vector<Import> ParseImports( const Lexems& lexems );
SyntaxAnalysisResult SyntaxAnalysis(
	const Lexems& lexems,
	MacrosByContextMap macros,
	MacroExpansionContextsPtr macro_expansion_contexts, /* in-out contexts */
	std::string file_path_hash );

NamespaceParsingResult ParseNamespaceElements(
	const Lexems& lexems,
	MacrosPtr macros, // Contents does not changed, because no macros can be parsed.
	MacroExpansionContextsPtr macro_expansion_contexts, /* in-out contexts */
	std::string file_path_hash );

ClassElementsParsingResult ParseClassElements(
	const Lexems& lexems,
	MacrosPtr macros, // Contents does not changed, because no macros can be parsed.
	MacroExpansionContextsPtr macro_expansion_contexts, /* in-out contexts */
	std::string file_path_hash );

BlockElementsParsingResult ParseBlockElements(
	const Lexems& lexems,
	MacrosPtr macros, // Contents does not changed, because no macros can be parsed.
	MacroExpansionContextsPtr macro_expansion_contexts, /* in-out contexts */
	std::string file_path_hash );

TypeNameParsingResult ParseTypeName(
	const Lexems& lexems,
	MacrosPtr macros, // Contents does not changed, because no macros can be parsed.
	MacroExpansionContextsPtr macro_expansion_contexts, /* in-out contexts */
	std::string file_path_hash );

ExpressionParsingResult ParseExpression(
	const Lexems& lexems,
	MacrosPtr macros, // Contents does not changed, because no macros can be parsed.
	MacroExpansionContextsPtr macro_expansion_contexts, /* in-out contexts */
	std::string file_path_hash );

} // namespace Synt

} // namespace U
