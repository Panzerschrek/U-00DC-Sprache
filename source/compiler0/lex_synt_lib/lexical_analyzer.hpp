#pragma once
#include <array>
#include <cstdint>
#include "../../lex_synt_lib_common/lex_synt_error.hpp"


namespace U
{

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
		CompareOrder, // <=>

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

		PointerTypeMark, // $
		ReferenceToPointer, // $<
		PointerToReference, // $>

		Ellipsis, // ...

		// TODO - add other lexems.

		EndOfFile,
	};

	std::string text; // Contains text for all lexem types, except numbers. Contains data of "struct NumberLexemData" for numbers.
	SrcLoc src_loc;
	Type type= Type::None;
};

bool operator==(const Lexem& l, const Lexem& r );
bool operator!=(const Lexem& l, const Lexem& r );

using Lexems= std::vector<Lexem>;

struct LexicalAnalysisResult
{
	Lexems lexems;
	LexSyntErrors errors;
};

LexicalAnalysisResult LexicalAnalysis( std::string_view program_text, bool collect_comments= false );

bool IsValidIdentifier(  std::string_view text );

using ProgramLinearPosition= uint32_t;

// Return mapping line -> linear position.
// Since lines are numbered from 1, element 0 is dummy.
// Result list is (obviously) sorted.
using LineToLinearPositionIndex= std::vector<ProgramLinearPosition>;

LineToLinearPositionIndex BuildLineToLinearPositionIndex( std::string_view program_text );

SrcLoc LinearPositionToSrcLoc( const LineToLinearPositionIndex& index, ProgramLinearPosition position );

ProgramLinearPosition GetIdentifierStartForPosition( std::string_view program_text, ProgramLinearPosition position );
ProgramLinearPosition GetIdentifierEndForPosition( std::string_view program_text, ProgramLinearPosition position );

} // namespace U
