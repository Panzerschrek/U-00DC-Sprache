#pragma once
#include <array>
#include <cstdint>
#include <optional>
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

		// Special kind of lexems, that can be created only manually (and not parsed).
		CompletionIdentifier,
		CompletionScope,
		CompletionDot,
		SignatureHelpBracketLeft,
		SignatureHelpComma,

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

//
// Additional text-related stuff.
//

bool IsValidIdentifier( std::string_view text );

using TextLinearPosition= uint32_t;

// Mapping line -> linear position.
// Since lines are numbered from 1, element 0 is dummy.
// It is (obviously) sorted.
using LineToLinearPositionIndex= std::vector<TextLinearPosition>;

// Build index and return it.
LineToLinearPositionIndex BuildLineToLinearPositionIndex( std::string_view text );
// Build index, reusing provided output container.
void BuildLineToLinearPositionIndex( std::string_view text, LineToLinearPositionIndex& out_index );

// Get line number (starting from 0).
uint32_t LinearPositionToLine( const LineToLinearPositionIndex& index, TextLinearPosition position );

// Get position of the start of identifier at given position.
// Returns none if there is no identifier here.
std::optional<TextLinearPosition> GetIdentifierStartForPosition( std::string_view text, TextLinearPosition position );

// Get position of the end of identifier at given position.
// Returns none if there is no identifier here.
std::optional<TextLinearPosition> GetIdentifierEndForPosition( std::string_view text, TextLinearPosition position );

} // namespace U
