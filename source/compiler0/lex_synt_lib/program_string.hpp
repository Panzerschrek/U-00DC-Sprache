#pragma once
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace U
{

using sprache_char = uint32_t;
sprache_char ReadNextUTF8Char( const char*& start, const char* end );
sprache_char GetUTF8FirstChar( const char* start, const char* end );
void PushCharToUTF8String( sprache_char c, std::string& str );

std::optional<uint32_t> Utf8PositionToUtf16Position( std::string_view text, uint32_t position );
std::optional<uint32_t> Utf8PositionToUtf32Position( std::string_view text, uint32_t position );
std::optional<uint32_t> Utf16PositionToUtf8Position( std::string_view text, uint32_t position );
std::optional<uint32_t> Utf32PositionToUtf8Position( std::string_view text, uint32_t position );

template<class T>
using ProgramStringMap= std::unordered_map< std::string, T >;
using ProgramStringSet= std::unordered_set< std::string >;

} // namespace U
