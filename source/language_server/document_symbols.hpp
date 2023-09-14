#pragma once
#include <functional>
#include "../compiler0/lex_synt_lib/syntax_analyzer.hpp"
#include "document_position.hpp"

namespace U
{

namespace LangServer
{

// Values from LSP specification.
enum class SymbolKind : uint8_t
{
	None= 0,
	File = 1,
	Module = 2,
	Namespace = 3,
	Package = 4,
	Class = 5,
	Method = 6,
	Property = 7,
	Field = 8,
	Constructor = 9,
	Enum = 10,
	Interface = 11,
	Function = 12,
	Variable = 13,
	Constant = 14,
	String = 15,
	Number = 16,
	Boolean = 17,
	Array = 18,
	Object = 19,
	Key = 20,
	Null = 21,
	EnumMember = 22,
	Struct = 23,
	Event = 24,
	Operator = 25,
	TypeParameter = 26,
};

struct Symbol;
using Symbols= std::vector<Symbol>;

struct Symbol
{
	std::string name;
	DocumentRange range;
	DocumentRange selection_range;
	Symbols children;
	SymbolKind kind= SymbolKind::None;
};

using SrcLocToRangeMappingFunction= std::function< std::optional<DocumentRange>( const SrcLoc& src_loc ) >;

Symbols BuildSymbols(
	const Synt::SyntaxAnalysisResult& synt_result,
	const SrcLocToRangeMappingFunction& src_loc_to_range_mapping_function );

} // namespace LangServer

} // namespace U
