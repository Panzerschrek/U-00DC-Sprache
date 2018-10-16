#pragma once
#include <map>
#include <memory>
#include <set>
#include "lexical_analyzer.hpp"
#include "program_string.hpp"

namespace U
{

namespace Synt
{

struct Macro
{
	enum class Context
	{
		Expression,
		Block,
		Class,
		Namespace,
	};

	enum class MatchElementKind
	{
		Lexem,
		Identifier,
		Typename,
		Expression,
		Block,
		Optional,
		Repeated,
	};

	enum class ResultElementKind
	{
		Lexem,
		VariableElement, // Identifier, typename, expression, block
		VariableElementWithMacroBlock, // Optional, loop
	};

	struct MatchElement
	{
		MatchElementKind kind= MatchElementKind::Lexem;
		Lexem lexem; // lexem for lexem elements, separator or EOF for loops
		ProgramString name; // for non-lexems
		std::vector<MatchElement> sub_elements; // For optionals, loops
	};

	struct ResultElement
	{
		ResultElementKind kind= ResultElementKind::Lexem;
		Lexem lexem; // lexem for SimpleElement, separator or EOF for loops
		ProgramString name; // for non-lexems
		std::vector<ResultElement> sub_elements; // For optionals, loops
	};

	FilePos file_pos;
	ProgramString name;
	std::vector<MatchElement> match_template_elements;
	std::vector<ResultElement> result_template_elements;
};

using MacroMap= std::map< ProgramString, Macro >;
using MacrosByContextMap= std::map< Macro::Context, MacroMap >;

using MacrosPtr= std::shared_ptr<MacrosByContextMap>;

} // namespace Synt

} // namespace U
