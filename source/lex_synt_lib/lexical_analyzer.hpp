#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "program_string.hpp"

namespace U
{

struct FilePos
{
	unsigned short line; // from 1
	unsigned short pos_in_line;
	unsigned short file_index;
};

bool operator==( const FilePos& l, const FilePos& r );
bool operator!=( const FilePos& l, const FilePos& r );
bool operator< ( const FilePos& l, const FilePos& r );
bool operator<=( const FilePos& l, const FilePos& r );

struct Lexem
{
	enum class Type : uint8_t
	{
		None,

		Comment,

		Identifier,
		MacroIdentifier,
		String,
		Number,

		LiteralSuffix, // For strings, numbers

		BracketLeft, // (
		BracketRight, // )

		SquareBracketLeft, // [
		SquareBracketRight, // ]

		BraceLeft, // {
		BraceRight, // }

		TemplateBracketLeft , // </
		TemplateBracketRight, // />

		MacroBracketLeft,  // <?
		MacroBracketRight, // ?>

		Scope, // ::

		Comma, // ,
		Dot, // .
		Colon, // :
		Semicolon, // ;

		Assignment, // =
		Plus, // +
		Minus, // -
		Star, // *
		Slash, // /
		Percent, // %

		And, // &
		Or, // |
		Xor, // ^
		Tilda, // ~
		Not, // !

		Apostrophe, // '

		Increment, // ++
		Decrement, // --

		CompareLess, // <
		CompareGreater, // >
		CompareEqual, // ==
		CompareNotEqual, // !=
		CompareLessOrEqual, // <=
		CompareGreaterOrEqual, // >=

		Conjunction, // &&
		Disjunction, // ||

		AssignAdd, // +=
		AssignSub, // -=
		AssignMul, // *=
		AssignDiv, // /=
		AssignRem, // %=
		AssignAnd, // &=
		AssignOr,  // |=
		AssignXor, // ^=

		ShiftLeft , // <<
		ShiftRight, // >>

		AssignShiftLeft , // <<=
		AssignShiftRight, // >>=

		LeftArrow,  // <-
		RightArrow, // ->

		Ellipsis, // ...

		// TODO - add other lexems.

		EndOfFile,
	};

	ProgramString text; // Non-empty for identifiers, strings, numbers. Empty for simple lexems.
	FilePos file_pos;
	Type type= Type::None;
};

typedef std::vector<Lexem> Lexems;

typedef std::string LexicalErrorMessage;
typedef std::vector<LexicalErrorMessage> LexicalErrorMessages;

struct LexicalAnalysisResult
{
	Lexems lexems;
	LexicalErrorMessages error_messages;
};

LexicalAnalysisResult LexicalAnalysis( const ProgramString& program_text, bool collect_comments= false );
LexicalAnalysisResult LexicalAnalysis( const sprache_char* program_text_data, size_t program_text_size, bool collect_comments= false );

} // namespace U
