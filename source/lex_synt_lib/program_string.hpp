#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace U
{

using sprache_char = uint32_t;
sprache_char ReadNextUTF8Char( const char*& start, const char* end );
sprache_char GetUTF8FirstChar( const char* start, const char* end );

template<class T>
using ProgramStringMap= std::unordered_map< std::string, T >;
using ProgramStringSet= std::unordered_set< std::string >;

} // namespace U
