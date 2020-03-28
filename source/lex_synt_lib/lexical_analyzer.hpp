#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>


namespace U
{

struct FilePos
{
	unsigned short file_index;
	unsigned short line; // from 1
	unsigned short column;
};

bool operator==( const FilePos& l, const FilePos& r );
bool operator!=( const FilePos& l, const FilePos& r );
bool operator< ( const FilePos& l, const FilePos& r );
bool operator<=( const FilePos& l, const FilePos& r );

struct NumberLexemData
{
	double value_double= 0.0f;
	uint64_t value_int= 0u;
	bool has_fractional_point= false;
	std::array<char, 7> type_suffix{0};
};

struct Lexem
{
	enum class Type : uint8_t
	{
		None,

		Comment,

		Identifier,
		MacroIdentifier,
		MacroUniqueIdentifier,
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
		Question, // ?

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

	std::string text; // Contains text for all lexem types, except numbers. Contains data of "struct NumberLexemData" for numbers.
	FilePos file_pos;
	Type type= Type::None;
};

bool operator==(const Lexem& l, const Lexem& r );
bool operator!=(const Lexem& l, const Lexem& r );

using Lexems= std::vector<Lexem>;

using LexicalErrorMessage= std::string;
using LexicalErrorMessages= std::vector<LexicalErrorMessage>;

struct LexicalAnalysisResult
{
	Lexems lexems;
	LexicalErrorMessages error_messages;
};

LexicalAnalysisResult LexicalAnalysis( const std::string& program_text, bool collect_comments= false );
LexicalAnalysisResult LexicalAnalysis( const char* program_text_data, size_t program_text_size, bool collect_comments= false );

} // namespace U
