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

sprache_char ReadNextUTF8Char( const char*& start, const char* end );
sprache_char GetUTF8FirstChar( const char* start, const char* end );

template<class T>
using ProgramStringMap= std::unordered_map< ProgramString, T >;

using ProgramStringSet= std::unordered_set< ProgramString >;

} // namespace U
