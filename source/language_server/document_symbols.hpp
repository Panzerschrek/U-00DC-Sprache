#pragma once
#include "../compiler0/lex_synt_lib/syntax_elements.hpp"

namespace U
{

// Position within document, without specifying document instance (specific document depends on context).
struct DocumentPosition
{
	uint32_t line= 0; // From 1, as in SrcLoc.
	uint32_t column= 0; // From 0, as in SrcLoc.
};

struct DocumentRange
{
	DocumentPosition start;
	DocumentPosition end;
};

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

struct Symbol
{
	std::string name;
	DocumentRange range;
	std::vector<Symbol> children;
	SymbolKind kind= SymbolKind::None;
};

std::vector<Symbol> BuildSymbols( const Synt::ProgramElements& program_elements );

} // namespace U
