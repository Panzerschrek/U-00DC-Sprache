#include "program_string.hpp"

namespace U
{

namespace
{

uint32_t GetNumberOfUTF16Words( const sprache_char code_point )
{
	if( code_point <= 0xFFFFu )
	{
		// Code points in range [0;0xFFFF] are encoded as singe UTF-16 word.
		return 1;
	}
	else
	{
		// Code points abowe 0xFFFF are encoded as two UTF-16 words.
		return 2;
	}
}

} // namespace

sprache_char ReadNextUTF8Char( const char*& start, const char* const end )
{
	// c_bit_masks[4] - menas save first 4 bits
	static const sprache_char c_bit_masks[9]=
	{
		(1 << 0) - 1,
		(1 << 1) - 1,
		(1 << 2) - 1,
		(1 << 3) - 1,
		(1 << 4) - 1,
		(1 << 5) - 1,
		(1 << 6) - 1,
		(1 << 7) - 1,
		(1 << 8) - 1,
	};

	const char c= *start;
	sprache_char code = 0;

	if( ( c & 0b10000000 ) == 0 )
	{
		code= sprache_char(c);
		++start;
	}
	else if( ( c & 0b11100000 ) == 0b11000000 )
	{
		if( start + 2 > end )
		{
			start= end;
			return code;
		}

		code=
			( (sprache_char(start[0]) & c_bit_masks[5]) << 6u ) |
			( (sprache_char(start[1]) & c_bit_masks[6]) << 0u );

		start+= 2;
	}
	else if( ( c & 0b11110000 ) == 0b11100000 )
	{
		if( start + 3 > end )
		{
			start= end;
			return code;
		}

		code=
			( (sprache_char(start[0]) & c_bit_masks[4]) << 12u ) |
			( (sprache_char(start[1]) & c_bit_masks[6]) <<  6u ) |
			( (sprache_char(start[2]) & c_bit_masks[6]) <<  0u );

		start+= 3;
	}
	else if( ( c & 0b11111000 ) == 0b11110000 )
	{
		if( start + 4 > end )
		{
			start= end;
			return code;
		}

		code=
			( (sprache_char(start[0]) & c_bit_masks[3]) << 18u ) |
			( (sprache_char(start[1]) & c_bit_masks[6]) << 12u ) |
			( (sprache_char(start[2]) & c_bit_masks[6]) <<  6u ) |
			( (sprache_char(start[3]) & c_bit_masks[6]) <<  0u );

		start+= 4;
	}
	else
	{
		// Codes above unicode range - wtf?
		++start;
	}

	return code;
}

sprache_char GetUTF8FirstChar( const char* start, const char* const end )
{
	return ReadNextUTF8Char( start, end );
}

void PushCharToUTF8String( const sprache_char c, std::string& str )
{
	if( c <= 0x7Fu )
		str.push_back( char(c) );
	else if( c <= 0x7FFu )
	{
		str.push_back( char( 0b11000000u | (c >>  6u) ) );
		str.push_back( char( 0b10000000u | (c &  63u) ) );
	}
	else if( c <= 0xFFFFu )
	{
		str.push_back( char( 0b11100000u |  (c >> 12u) ) );
		str.push_back( char( 0b10000000u | ((c >> 6u) & 63u) ) );
		str.push_back( char( 0b10000000u |  (c  & 63u) ) );
	}
	else if( c <= 0x10FFFFu )
	{
		str.push_back( char( 0b11110000u | ((c >> 18u) &  7u) ) );
		str.push_back( char( 0b10000000u | ((c >> 12u) & 63u) ) );
		str.push_back( char( 0b10000000u | ((c >>  6u) & 63u) ) );
		str.push_back( char( 0b10000000u |  (c  & 63u) ) );
	}
	else
	{
		// Codes above unicode range - wtf?
	}
}

std::optional<uint32_t> Utf8PositionToUtf16Position( const std::string_view text, const uint32_t position )
{
	uint32_t current_utf16_position= 0;
	const char* s= text.data();
	const char* const s_end= s + text.size();
	while( s < s_end && uint32_t(s - text.data()) < position )
		current_utf16_position+= GetNumberOfUTF16Words( ReadNextUTF8Char( s, s_end ) );

	if( uint32_t(s - text.data()) != position )
		return std::nullopt;

	return current_utf16_position;
}

std::optional<uint32_t> Utf8PositionToUtf32Position( const std::string_view text, const uint32_t position )
{
	uint32_t current_utf32_position= 0;
	const char* s= text.data();
	const char* const s_end= s + text.size();
	while( s < s_end && uint32_t(s - text.data()) < position )
	{
		ReadNextUTF8Char( s, s_end );
		++current_utf32_position;
	}

	if( uint32_t(s - text.data()) != position )
		return std::nullopt;

	return current_utf32_position;
}

std::optional<uint32_t> Utf16PositionToUtf8Position( const std::string_view text, const uint32_t position )
{
	// Extract from UTF-8 string code points and count number of UTF-16 words.
	uint32_t current_utf16_position= 0;
	const char* s= text.data();
	const char* const s_end= s + text.size();
	while( s < s_end && current_utf16_position < position )
		current_utf16_position+= GetNumberOfUTF16Words( ReadNextUTF8Char( s, s_end ) );

	if( current_utf16_position != position )
	{
		// Something went wrong. Maybe utf-16 position is too big?
		return std::nullopt;
	}

	return uint32_t( s - text.data() );
}

std::optional<uint32_t> Utf32PositionToUtf8Position( const std::string_view text, const uint32_t position )
{
	uint32_t current_utf32_position= 0;
	const char* s= text.data();
	const char* const s_end= s + text.size();
	while( s < s_end && current_utf32_position < position )
	{
		ReadNextUTF8Char( s, s_end );
		++current_utf32_position;
	}

	if( current_utf32_position != position )
		return std::nullopt;

	return uint32_t(s - text.data());
}

} // namespace U
