#include "assert.hpp"
#include "program_string.hpp"

namespace U
{

size_t GetUTF8CharBytes( const sprache_char c )
{
	if( c <= 0x7Fu )
		return 1u;
	else if( c <= 0x7FFu )
		return 2u;
	else
		return 3u;
}

sprache_char ReadNextUTF8Char( const char*& start, const char* const end )
{
	// c_bit_masks[4] - menas save first 4 bits
	static const uint32_t c_bit_masks[9]=
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
		code= c;
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
			( (start[0] & c_bit_masks[5]) << 6u ) |
			( (start[1] & c_bit_masks[6]) << 0u );

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
			( (start[0] & c_bit_masks[4]) << 12u ) |
			( (start[1] & c_bit_masks[6]) <<  6u ) |
			( (start[2] & c_bit_masks[6]) <<  0u );

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
			( (start[0] & c_bit_masks[3]) << 18u ) |
			( (start[1] & c_bit_masks[6]) << 12u ) |
			( (start[2] & c_bit_masks[6]) <<  6u ) |
			( (start[3] & c_bit_masks[6]) <<  0u );

		start+= 4;
	}
	else if( ( c & 0b11111100 ) == 0b11111000 )
	{
		if( start + 5 > end )
		{
			start= end;
			return code;
		}

		code=
			( (start[0] & c_bit_masks[2]) << 24u ) |
			( (start[1] & c_bit_masks[6]) << 18u ) |
			( (start[2] & c_bit_masks[6]) << 12u ) |
			( (start[3] & c_bit_masks[6]) <<  6u ) |
			( (start[4] & c_bit_masks[6]) <<  0u );

		start+= 5;
	}
	else if( ( c & 0b11111110 ) == 0b11111100 )
	{
		if( start + 6 > end )
		{
			start= end;
			return code;
		}

		code=
			( (start[0] & c_bit_masks[1]) << 30u ) |
			( (start[1] & c_bit_masks[6]) << 24u ) |
			( (start[2] & c_bit_masks[6]) << 18u ) |
			( (start[3] & c_bit_masks[6]) << 12u ) |
			( (start[4] & c_bit_masks[6]) <<  6u ) |
			( (start[5] & c_bit_masks[6]) <<  0u );

		start+= 6;
	}
	else
	{
		// WTF-?
		++start;
	}

	return code;
}

sprache_char GetUTF8FirstChar( const char* start, const char* const end )
{
	return ReadNextUTF8Char( start, end );
}

} // namespace U
