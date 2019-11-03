#pragma once

#include "program_string.hpp"

namespace U
{

// Identificators of all language keywords.
// Names same, as in Ü-Sprache, but with '_' at the end, for prevention of intersection with C++ keywords.
enum class Keywords : unsigned int
{
	#define PROCESS_KEYWORD(x) x##_,
	#include "keywords_list.hpp"
	#undef PROCESS_KEYWORD
	LastKeyword,
};

bool IsKeyword( const ProgramString& str );

const ProgramString& Keyword( Keywords keyword );
const char* KeywordAscii( Keywords keyword );

// Relation operators for program string and keyword enum.
// This operators make posible to write "str == Keywords::var_".
bool operator==( Keywords keyword, const ProgramString& str );
bool operator==( const ProgramString& str, Keywords keyword );
bool operator!=( Keywords keyword, const ProgramString& str );
bool operator!=( const ProgramString& str, Keywords keyword );

} // namespace U
