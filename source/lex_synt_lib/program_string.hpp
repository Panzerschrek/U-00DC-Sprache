#pragma once

#include <string>
#include <vector>

namespace U
{

typedef std::uint16_t sprache_char;
typedef std::basic_string<sprache_char> ProgramString;

// Char to program string literal.
ProgramString operator "" _SpC( const char* str, size_t size );

// Same as literal operator.
ProgramString ToProgramString( const char* c );

size_t GetUTF8CharBytes( sprache_char c );
std::string ToUTF8( const ProgramString& str );

ProgramString DecodeUTF8( const std::vector<char>& str );
ProgramString DecodeUTF8( const std::string& str );

} // namespace U
