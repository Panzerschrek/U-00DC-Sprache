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
	};

	enum class ResultElementKind
	{
		Lexem,
		SubElement,
	};

	struct MatchElement
	{
		ElementKind kind= ElementKind::Lexem;
		Lexem lexem;
		ProgramString name; // for non-lexems
	};

	struct ResultElement
	{
		ResultElementKind kind= ResultElementKind::Lexem;
		Lexem lexem;
		ProgramString name; // for non-lexems
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
