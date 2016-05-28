#include "assert.hpp"

#include "keywords.hpp"

namespace Interpreter
{

namespace
{

struct KeywordEntry
{
	KeywordEntry( const char* str )
		: program_string( ToProgramString(str) )
		, ascii( str )
	{}

	const ProgramString program_string;
	const char* const ascii;
};

static const KeywordEntry g_keywords[ size_t(Keywords::LastKeyword) ]=
{
	[ size_t(Keywords::fn_) ]= "fn",
	[ size_t(Keywords::let_) ]= "let",
	[ size_t(Keywords::return_) ]= "return",
	[ size_t(Keywords::while_) ]= "while",
	[ size_t(Keywords::break_) ]= "break",
	[ size_t(Keywords::continue_) ]= "continue",
	[ size_t(Keywords::if_) ]= "if",
	[ size_t(Keywords::else_) ]= "else",

	[ size_t(Keywords::void_) ]= "void",
	[ size_t(Keywords::bool_) ]= "bool",
	[ size_t(Keywords::i8_ ) ]= "i8" ,
	[ size_t(Keywords::u8_ ) ]= "u8" ,
	[ size_t(Keywords::i16_) ]= "i16",
	[ size_t(Keywords::u16_) ]= "u16",
	[ size_t(Keywords::i32_) ]= "i32",
	[ size_t(Keywords::u32_) ]= "u32",
	[ size_t(Keywords::i64_) ]= "i64",
	[ size_t(Keywords::u64_) ]= "u64",

	[ size_t(Keywords::true_) ]= "true",
	[ size_t(Keywords::false_) ]= "false",
};

} // namespace

const ProgramString& Keyword( Keywords keyword )
{
	U_ASSERT( keyword < Keywords::LastKeyword );

	return g_keywords[ size_t(keyword) ].program_string;
}

const char* KeywordAscii( Keywords keyword )
{
	U_ASSERT( keyword < Keywords::LastKeyword );

	return g_keywords[ size_t(keyword) ].ascii;
}

bool operator==( Keywords keyword, const ProgramString& str )
{
	return Keyword( keyword ) == str;
}

bool operator==( const ProgramString& str, Keywords keyword )
{
	return keyword == str;
}

bool operator!=( Keywords keyword, const ProgramString& str )
{
	return Keyword( keyword ) != str;
}

bool operator!=( const ProgramString& str, Keywords keyword )
{
	return keyword != str;
}

} // namespace Interpreter
