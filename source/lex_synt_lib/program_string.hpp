#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace U
{

using ProgramString= std::string;

using sprache_char = uint32_t;

size_t GetUTF8CharBytes( sprache_char c );
std::string ToUTF8( const ProgramString& str );

sprache_char ReadNextUTF8Char( const char*& start, const char* end );
sprache_char GetUTF8FirstChar( const char* start, const char* end );

// String map/set.
struct ProgramStringHasher
{
	size_t operator()( const ProgramString& str ) const;
};

template<class T>
using ProgramStringMap= std::unordered_map< ProgramString, T, ProgramStringHasher >;

using ProgramStringSet= std::unordered_set< ProgramString, ProgramStringHasher >;

} // namespace U
