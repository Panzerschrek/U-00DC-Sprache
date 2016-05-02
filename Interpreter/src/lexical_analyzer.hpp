#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Interpreter
{

typedef std::uint16_t sprache_char;

typedef std::basic_string<sprache_char> ProgramString;

ProgramString ToProgramString( const char* c );

// Warning, possible lost of data in conversion.
std::string ToStdString( const ProgramString& str );

struct Lexem
{
	enum class Type
	{
		None,

		Identifier,
		String,
		Number,

		BracketLeft, // (
		BracketRight, // )

		SquareBracketLeft, // [
		SquareBracketRight, // ]

		BraceLeft, // {
		BraceRight, // }

		Comma, // ,
		Dot, // .
		Colon, // :
		Semicolon, // ;

		Assignment, // =
		Plus, // +
		Minus, // -
		Star, // *
		Slash, // /

		Or, // |
		Xor, // ^
		Tilda, // ~
		Not, // !

		Increment, // ++
		Decrement, // --

		CompareLess, // <
		CommpareGreater, // >
		CompareEqual, // ==
		CompareNotEqual, // !=
		CompareLessOrEqual, // <=
		CompareGreaterOrEqual, // >=

		Conjunction, // &&
		Disjunction, // ||

		// TODO - add other lexems.

		EndOfFile,
	};

	Type type= Type::None;
	ProgramString text;

	unsigned int line= 0;
	unsigned int pos_in_line= 0;
};

typedef std::vector<Lexem> Lexems;

typedef std::string LexicalErrorMessage;
typedef std::vector<LexicalErrorMessage> LexicalErrorMessages;

struct LexicalAnalysisResult
{
	Lexems lexems;
	LexicalErrorMessages error_messages;
};

LexicalAnalysisResult LexicalAnalysis( const ProgramString& program_text );

} // namespace Interpreter
