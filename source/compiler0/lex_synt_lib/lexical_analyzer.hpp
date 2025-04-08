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

		Identifier,
		MacroIdentifier,
		MacroUniqueIdentifier,
		String,
		CharLiteral,
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

		DoubleColon, // ::

		Comma, // ,
		Dot, // .
		Colon, // :
		Semicolon, // ;
		Question, // ?

		Equal, // =
		Plus, // +
		Minus, // -
		Asterisk, // *
		Slash, // /
		Percent, // %

		Ampersand, // &
		Pipe, // |
		Caret, // ^
		Tilde, // ~
		Exclamation, // !

		At, // @

		DoublePlus, // ++
		DoubleMinus, // --

		Less, // <
		Greater, // >
		DoubleEqual, // ==
		ExclamationEqual, // !=
		LessEqual, // <=
		GreaterEqual, // >=
		LessEqualGreater, // <=>

		DoubleAmpersand, // &&
		DoublePipe, // ||

		PlusEqual, // +=
		MinusEqual, // -=
		AsteriskEqual, // *=
		SlashEqual, // /=
		PercentEqual, // %=
		AmpersandEqual, // &=
		PipeEqual,  // |=
		CaretEqual, // ^=

		DoubleLess , // <<
		DoubleGreater, // >>

		DoubleLessEqual , // <<=
		DoubleGreaterEqual, // >>=

		MinusGreater, // ->

		Dollar, // $
		DollarLess, // $<
		DollarGreater, // $>

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

using Lexems= std::vector<Lexem>;

struct LexicalAnalysisResult
{
	Lexems lexems;
	LexSyntErrors errors;
};

LexicalAnalysisResult LexicalAnalysis( std::string_view program_text );

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

// Returns true if successfuly parsed string literal at start of given string.
std::optional<std::string> ParseStringLiteral( std::string_view text );

} // namespace U
