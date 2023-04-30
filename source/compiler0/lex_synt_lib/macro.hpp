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
	enum class Context : uint8_t
	{
		Expression,
		Block,
		Class,
		Namespace,
	};

	enum class MatchElementKind : uint8_t
	{
		Lexem,
		Identifier,
		Typename,
		Expression,
		Block,
		IfAlternative,
		Optional,
		Repeated,
	};

	enum class ResultElementKind : uint8_t
	{
		Lexem,
		VariableElement, // Identifier, typename, expression, block
		VariableElementWithMacroBlock, // Optional, loop
	};

	enum class BlockCheckLexemKind : uint8_t
	{
		LexemAfterBlockEnd, // Detect optional/loop end, using lexem after optional/loop as terminator.
		LexemAtBlockStart, // Detect optional/loop, using lexem in beginning of optional/loop.
	};

	struct MatchElement;
	using MatchElements= std::vector<MatchElement>;

	struct MatchElement
	{
		MatchElementKind kind= MatchElementKind::Lexem;
		BlockCheckLexemKind block_check_lexem_kind= BlockCheckLexemKind::LexemAfterBlockEnd; // For optionals and loops.
		Lexem lexem; // lexem for lexem elements, separator or EOF for loops
		std::string name; // for non-lexems
		MatchElements sub_elements; // For optionals, loops
	};

	struct ResultElement;
	using ResultElements= std::vector<ResultElement>;

	struct ResultElement
	{
		ResultElementKind kind= ResultElementKind::Lexem;
		Lexem lexem; // lexem for SimpleElement, separator or EOF for loops
		std::string name; // for non-lexems
		ResultElements sub_elements; // For optionals, loops
	};

	SrcLoc src_loc;
	std::string name;
	MatchElements match_template_elements;
	ResultElements result_template_elements;
};

using MacroMap= ProgramStringMap< Macro >;
using MacrosByContextMap= std::unordered_map< Macro::Context, MacroMap >;

using MacrosPtr= std::shared_ptr<MacrosByContextMap>;

} // namespace Synt

} // namespace U
