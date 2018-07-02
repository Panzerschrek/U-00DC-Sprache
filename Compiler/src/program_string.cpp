#include "assert.hpp"

#include "program_string.hpp"

namespace U
{

ProgramString operator "" _SpC( const char* str, size_t size )
{
	ProgramString result;
	result.reserve( size );

	const char* end= str + size;
	while( str < end )
	{
		result.push_back( *str );
		++str;
	}

	return result;
}

ProgramString ToProgramString( const char* str )
{
	ProgramString result;

	while( *str != 0 )
	{
		result.push_back( *str );
		++str;
	}

	return result;
}

std::string ToStdString( const ProgramString& str )
{
	std::string result( str.size(), '\0' );

	for( unsigned int i= 0; i < str.size(); i++ )
		result[i]= std::string::value_type(str[i]);

	return result;
}

static ProgramString DecodeUTF8( const char* start, const char* end )
{
	U_ASSERT( start <= end );

	// c_bit_masks[4] - menas save first 4 bits
	static const int c_bit_masks[9]=
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

	ProgramString result;

	// Reserve approximated size.
	result.reserve( ( end - start ) * 5 / 4 );

	const char* p= start;
	while( p < end )
	{
		size_t code;
		char c= *p;

		if( ( c & 0b10000000 ) == 0 )
		{
			code= c;
			++p;
		}
		else if( ( c & 0b11100000 ) == 0b11000000 )
		{
			if( p + 2 > end ) break;

			code=
				( (p[0] & c_bit_masks[5]) << 6 ) |
				( (p[1] & c_bit_masks[6]) << 0 );

			p+= 2;
		}
		else if( ( c & 0b11110000 ) == 0b11100000 )
		{
			if( p + 3 > end ) break;

			code=
				( (p[0] & c_bit_masks[4]) << 12 ) |
				( (p[1] & c_bit_masks[6]) <<  6 ) |
				( (p[2] & c_bit_masks[6]) <<  0 );

			p+= 3;
		}
		else if( ( c & 0b11111000 ) == 0b11110000 )
		{
			if( p + 4 > end ) break;

			code=
				( (p[0] & c_bit_masks[3]) << 18 ) |
				( (p[1] & c_bit_masks[6]) << 12 ) |
				( (p[2] & c_bit_masks[6]) <<  6 ) |
				( (p[3] & c_bit_masks[6]) <<  0 );

			p+= 4;
		}
		else if( ( c & 0b11111100 ) == 0b11111000 )
		{
			if( p + 5 > end ) break;

			code=
				( (p[0] & c_bit_masks[2]) << 24 ) |
				( (p[1] & c_bit_masks[6]) << 18 ) |
				( (p[2] & c_bit_masks[6]) << 12 ) |
				( (p[3] & c_bit_masks[6]) <<  6 ) |
				( (p[4] & c_bit_masks[6]) <<  0 );

			p+= 5;
		}
		else if( ( c & 0b11111110 ) == 0b11111100 )
		{
			if( p + 6 > end ) break;

			code=
				( (p[0] & c_bit_masks[1]) << 30 ) |
				( (p[1] & c_bit_masks[6]) << 24 ) |
				( (p[2] & c_bit_masks[6]) << 18 ) |
				( (p[3] & c_bit_masks[6]) << 12 ) |
				( (p[4] & c_bit_masks[6]) <<  6 ) |
				( (p[5] & c_bit_masks[6]) <<  0 );

			p+= 6;
		}
		else
		{
			p++; // WTF - ?
		}

		result.push_back( sprache_char(code) );
	}

	return result;
}

ProgramString DecodeUTF8( const std::vector<char>& str )
{
	return DecodeUTF8( str.data(), str.data() + str.size() );
}

ProgramString DecodeUTF8( const std::string& str )
{
	return DecodeUTF8( str.data(), str.data() + str.size() );
}

} // namespace U
