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
		// TODO - add other context
	};

	enum class ElementKind
	{
		Lexem,
		Identifier,
		Typename,
		Expression,
		Block,
		Optional,
	};

	enum class ResultElementKind
	{
		Lexem,
		SimpleElement, // Identifier, typename, expression, block
		ElementWithMacroBlock, // Optional, loop
	};

	struct MatchElement
	{
		ElementKind kind= ElementKind::Lexem;
		Lexem lexem;
		ProgramString name; // for non-lexems
		std::vector<MatchElement> sub_elements; // For optionals, loops
	};

	struct ResultElement
	{
		ResultElementKind kind= ResultElementKind::Lexem;
		Lexem lexem;
		ProgramString name; // for non-lexems
		std::vector<ResultElement> sub_elements; // For optionals, loops
	};

	ProgramString name;
	std::vector<MatchElement> match_template_elements;
	std::vector<ResultElement> result_template_elements;
};

using MacroMap= std::map< ProgramString, Macro >;
using MacrosByContextMap= std::map< Macro::Context, MacroMap >;

using MacrosPtr= std::shared_ptr<MacrosByContextMap>;

} // namespace Synt

} // namespace U
