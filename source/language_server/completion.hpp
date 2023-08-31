#pragma once
#include "../compiler0/code_builder_lib/code_builder.hpp"

namespace U
{

namespace LangServer
{

// Values from LSP specification.
enum class CompletionItemKind  : uint8_t
{
	None= 0,
	Text = 1,
	Method = 2,
	Function = 3,
	Constructor = 4,
	Field = 5,
	Variable = 6,
	Class = 7,
	Interface = 8,
	Module = 9,
	Property = 10,
	Unit = 11,
	Value = 12,
	Enum = 13,
	Keyword = 14,
	Snippet = 15,
	Color = 16,
	File = 17,
	Reference = 18,
	Folder = 19,
	EnumMember = 20,
	Constant = 21,
	Struct = 22,
	Event = 23,
	Operator = 24,
	TypeParameter = 25,
};

struct CompletionItem
{
	std::string label;
	std::string sort_text;
	std::string detail;
	CompletionItemKind kind= CompletionItemKind::None;
};

CompletionItemKind TranslateCompletionItemKind( CodeBuilder::CompletionItemKind value_kind );

} // namespace LangServer

} // namespace U
