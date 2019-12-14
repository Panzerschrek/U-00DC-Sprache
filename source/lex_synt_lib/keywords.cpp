#include "assert.hpp"
#include "program_string.hpp"
#include "keywords.hpp"

namespace U
{

namespace
{

const ProgramStringSet g_keywords_set
{
	#define PROCESS_KEYWORD(x) #x,
	#include "keywords_list.hpp"
	#undef PROCESS_KEYWORD
};

const std::string g_keywords[ size_t(Keywords::LastKeyword) ]
{
	#define PROCESS_KEYWORD(x) #x,
	#include "keywords_list.hpp"
	#undef PROCESS_KEYWORD
};

} // namespace

bool IsKeyword( const std::string& str )
{
	return g_keywords_set.count( str ) != 0;
}

const std::string& Keyword( const Keywords keyword )
{
	U_ASSERT( keyword < Keywords::LastKeyword );
	return g_keywords[ size_t(keyword) ];
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
