#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "push_disable_boost_warnings.hpp"
#include <boost/functional/hash.hpp>
#include "pop_boost_warnings.hpp"

namespace U
{

using sprache_char= std::uint16_t;
using ProgramString= std::basic_string<sprache_char>;

// Char to program string literal.
ProgramString operator "" _SpC( const char* str, size_t size );

// Same as literal operator.
ProgramString ToProgramString( const char* c );
ProgramString ToProgramString( const std::string& str );

size_t GetUTF8CharBytes( sprache_char c );
std::string ToUTF8( const ProgramString& str );

ProgramString DecodeUTF8( const char* start, const char* end );
ProgramString DecodeUTF8( const std::vector<char>& str );
ProgramString DecodeUTF8( const std::string& str );

// String map/set.
struct ProgramStringHasher
{
	size_t operator()( const ProgramString& str ) const
	{
		return boost::hash_range( str.begin(), str.end() );
	}
};

template<class T>
using ProgramStringMap= std::unordered_map< ProgramString, T, ProgramStringHasher >;

using ProgramStringSet= std::unordered_set< ProgramString, ProgramStringHasher >;

} // namespace U
