#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace U
{

// Returns uint32_t(-1) if failed to parse.
template<uint32_t base>
uint32_t TryParseDigit( const char c )
{
	if constexpr( base == 2 )
	{
		if( c >= '0' && c <= '1' )
			return uint32_t(c - '0');
	}
	else if constexpr( base == 8 )
	{
		if( c >= '0' && c <= '7' )
			return uint32_t(c - '0');
	}
	else if constexpr( base == 10 )
	{
		if( c >= '0' && c <= '9' )
			return uint32_t(c - '0');
	}
	else if constexpr( base == 16 )
	{
		if( c >= '0' && c <= '9' )
			return uint32_t(c - '0');
		else if( c >= 'a' && c <= 'f' )
			return uint32_t(c - 'a' + 10);
		else if( c >= 'A' && c <= 'F' )
			return uint32_t(c - 'A' + 10);
	}

	return uint32_t(-1);
}

// Our own replacement for 128-bit integers.
// We can't use native 128-bit integer type, since many C++ compilers (especially for 32-bit platforms) don't support it.
// We can't also use llvm::APInt here, since lex/synt code shouldn't depend on LLVM libraries.
struct Int128
{
	uint64_t lo= 0u;
	uint64_t hi= 0u;
};

template<uint32_t base>
std::optional<Int128> DecodeParsedInteger( const std::string_view s )
{
	// This function parses a number into 128-bit integer.
	// Since C++ has no standard 128-bit integers, implement here our own long arithmetic using four 32-bit integers.

	uint32_t words[4]{ 0, 0, 0, 0 };

	for( const char c : s )
	{
		const uint32_t digit= TryParseDigit<base>(c);
		if( digit == uint32_t(-1) )
			return std::nullopt;

		uint32_t remainder= digit;

		for( size_t i= 0; i < 4; ++i )
		{
			const uint64_t v= uint64_t( words[i] ) * base + remainder;
			words[i]= uint32_t(v);
			remainder= uint32_t( v >> 32 );
		}

		if( remainder != 0u )
			return std::nullopt; // Overflow detected.
	}

	return Int128
	{
		uint64_t( words[0] ) | ( uint64_t( words[1] ) << 32 ),
		uint64_t( words[2] ) | ( uint64_t( words[3] ) << 32 ),
	};
}

} // namespace U
