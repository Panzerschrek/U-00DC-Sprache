#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "program_string.hpp"

namespace Interpreter
{

struct FilePos
{
	unsigned int line; // from 1
	unsigned int pos_in_line;
};

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

		And, // &
		Or, // |
		Xor, // ^
		Tilda, // ~
		Not, // !

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

		// TODO - add other lexems.

		EndOfFile,
	};

	Type type= Type::None;
	ProgramString text;

	FilePos file_pos;
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
