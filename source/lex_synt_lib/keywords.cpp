#include "assert.hpp"
#include "program_string.hpp"
#include "keywords.hpp"

namespace U
{

// Hack for initialization.
// Use in-function static variables for cross-module initialization order setup.

bool IsKeyword( const std::string& str )
{
	static const ProgramStringSet c_keywords_set
	{
		#define PROCESS_KEYWORD(x) #x,
		#include "keywords_list.hpp"
		#undef PROCESS_KEYWORD
	};

	return c_keywords_set.count( str ) != 0;
}

const std::string& Keyword( Keywords keyword )
{
	U_ASSERT( keyword < Keywords::LastKeyword );
	static const std::string c_keywords[ size_t(Keywords::LastKeyword) ]
	{
		#define PROCESS_KEYWORD(x) #x,
		#include "keywords_list.hpp"
		#undef PROCESS_KEYWORD
	};

	return c_keywords[ size_t(keyword) ];
}

bool operator==( Keywords keyword, const std::string& str )
{
	return Keyword( keyword ) == str;
}

bool operator==( const std::string& str, Keywords keyword )
{
	return keyword == str;
}

bool operator!=( Keywords keyword, const std::string& str )
{
	return Keyword( keyword ) != str;
}

bool operator!=( const std::string& str, Keywords keyword )
{
	return keyword != str;
}

} // namespace U
