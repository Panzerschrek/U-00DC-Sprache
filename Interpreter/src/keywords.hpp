#pragma once

#include "program_string.hpp"

namespace Interpreter
{

// Identificators of all language keywords.
// Names same, as in Ü-Sprache, but with '_' at the end, for prevention of intersection with C++ keywords.
enum class Keywords : unsigned int
{
	// Common keywords.
	fn_,
	let_,
	return_,
	while_,
	break_,
	continue_,
	if_,
	else_,

	class_,

	// Fundamental types names.
	void_,
	bool_,
	i8_ ,
	u8_ ,
	i16_,
	u16_,
	i32_,
	u32_,
	i64_,
	u64_,
	f32_,
	f64_,

	// Boolean constants.
	true_,
	false_,

	LastKeyword,
};

bool IsKeyword( const ProgramString& str );

const ProgramString& Keyword( Keywords keyword );
const char* KeywordAscii( Keywords keyword );

// Relation operators for program string and keyword enum.
// This operators make posible to write "str == Keywords::let_".
bool operator==( Keywords keyword, const ProgramString& str );
bool operator==( const ProgramString& str, Keywords keyword );
bool operator!=( Keywords keyword, const ProgramString& str );
bool operator!=( const ProgramString& str, Keywords keyword );

} // namespace Interpreter
