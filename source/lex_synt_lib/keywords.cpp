#include "assert.hpp"

#include "keywords.hpp"

namespace U
{

namespace
{

struct KeywordEntry
{
	KeywordEntry( const char* str )
		: program_string( str )
		, ascii( str )
	{}

	const std::string program_string;
	const char* const ascii;
};

struct Globals
{
	const KeywordEntry (&keywords)[ size_t(Keywords::LastKeyword) ];
	const ProgramStringSet& keywords_set;
};

// Hack for initialization.
// Use in-function static variables for cross-module initialization order setup.
const Globals& GetGlobals()
{
	static const KeywordEntry c_keywords[ size_t(Keywords::LastKeyword) ]
	{
		#define PROCESS_KEYWORD(x) #x,
		#include "keywords_list.hpp"
		#undef PROCESS_KEYWORD
	};
	
	static const ProgramStringSet c_keywords_set=
	[]() -> ProgramStringSet
	{
		ProgramStringSet result;
		for( const KeywordEntry& k : c_keywords )
			result.emplace( k.program_string );
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

	return GetGlobals().keywords[ size_t(keyword) ].program_string;
}

const char* KeywordAscii( const Keywords keyword )
{
	U_ASSERT( keyword < Keywords::LastKeyword );
	return GetGlobals().keywords[ size_t(keyword) ].ascii;
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
