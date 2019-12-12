#include "push_disable_boost_warnings.hpp"
#include <boost/functional/hash.hpp>
#include "pop_boost_warnings.hpp"

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

std::string ToUTF8( const ProgramString& str )
{
	/*
	// TODO - check this.
	std::string result;
	for( const sprache_char& c : str )
	{
		if( c <= 0x7Fu )
			result.push_back( static_cast<char>(c) );
		else if( c <= 0x7FFu )
		{
			result.push_back( static_cast<char>( 0b11000000u | (c >>  6u) ) );
			result.push_back( static_cast<char>( 0b10000000u | (c &  63u) ) );
		}
		else
		{
			result.push_back( static_cast<char>( 0b11100000u |  (c >> 12u) ) );
			result.push_back( static_cast<char>( 0b11000000u | ((c >> 6u) & 63u) ) );
			result.push_back( static_cast<char>( 0b10000000u |  (c  & 63u) ) );
		}
	}

	return result;
	*/
	return str;
}

ProgramString DecodeUTF8( const char* start, const char* end )
{
	/*
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
		uint32_t code;
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
			continue;
		}

		result.push_back( sprache_char(code) );
	}

	return result;
	*/
	return ProgramString( start, end );
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

ProgramString DecodeUTF8( const std::vector<char>& str )
{
	return DecodeUTF8( str.data(), str.data() + str.size() );
}

ProgramString DecodeUTF8( const std::string& str )
{
	return DecodeUTF8( str.data(), str.data() + str.size() );
}

size_t ProgramStringHasher::operator()( const ProgramString& str ) const
{
	return boost::hash_range( str.begin(), str.end() );
}

} // namespace U
