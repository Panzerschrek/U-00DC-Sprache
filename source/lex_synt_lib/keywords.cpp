#include "assert.hpp"
#include "program_string.hpp"
#include "keywords.hpp"

namespace U
{

namespace
{

struct Globals
{
	const std::string (&keywords)[ size_t(Keywords::LastKeyword) ];
	const ProgramStringSet& keywords_set;
};

// Hack for initialization.
// Use in-function static variables for cross-module initialization order setup.
const Globals& GetGlobals()
{
	static const std::string c_keywords[ size_t(Keywords::LastKeyword) ]
	{
		#define PROCESS_KEYWORD(x) #x,
		#include "keywords_list.hpp"
		#undef PROCESS_KEYWORD
	};
	
	static const ProgramStringSet c_keywords_set=
	[]() -> ProgramStringSet
	{
		ProgramStringSet result;
		for( const std::string& k : c_keywords )
			result.emplace( k );
		return result;
	}
	();

	static const Globals c_globals{ c_keywords, c_keywords_set };

	return c_globals;
}

} // namespace

bool IsKeyword( const std::string& str )
{
	return GetGlobals().keywords_set.count( str ) != 0;
}

const std::string& Keyword( Keywords keyword )
{
	U_ASSERT( keyword < Keywords::LastKeyword );

	return GetGlobals().keywords[ size_t(keyword) ];
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
