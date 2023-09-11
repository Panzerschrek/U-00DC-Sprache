#include <cstring>
#include "../../tests/tests_lib/funcs_registrator.hpp"
#include "../../tests/tests_lib/tests.hpp"
#include "../lex_synt_lib/program_string.hpp"

namespace U
{

namespace
{

// Use common string for all tests (for simplicity).
static const char text[]= "abc лес-☯♂☭💀";

U_TEST( ReadNextUTF8Char_Test )
{
	const auto length= std::strlen(text);
	U_TEST_ASSERT( length == 24 );

	const char* s= text;
	const char* const s_end= s + std::strlen(text);

	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == 'a' );
	U_TEST_ASSERT( s - text ==  1 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == 'b' );
	U_TEST_ASSERT( s - text ==  2 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == 'c' );
	U_TEST_ASSERT( s - text ==  3 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == ' ' );
	U_TEST_ASSERT( s - text ==  4 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == U'л' );
	U_TEST_ASSERT( s - text ==  6 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == U'е' );
	U_TEST_ASSERT( s - text ==  8 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == U'с' );
	U_TEST_ASSERT( s - text == 10 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == '-' );
	U_TEST_ASSERT( s - text == 11 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == U'☯' );
	U_TEST_ASSERT( s - text == 14 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == U'♂' );
	U_TEST_ASSERT( s - text == 17 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == U'☭' );
	U_TEST_ASSERT( s - text == 20 );
	U_TEST_ASSERT( ReadNextUTF8Char( s, s_end ) == U'💀' );
	U_TEST_ASSERT( s - text == 24 );
}

U_TEST( PushCharToUTF8String_Test )
{
	std::string str;
	PushCharToUTF8String( 'a', str );
	U_TEST_ASSERT( str.size() ==  1 );
	PushCharToUTF8String( 'b', str );
	U_TEST_ASSERT( str.size() ==  2 );
	PushCharToUTF8String( 'c', str );
	U_TEST_ASSERT( str.size() ==  3 );
	PushCharToUTF8String( ' ', str );
	U_TEST_ASSERT( str.size() ==  4 );
	PushCharToUTF8String( U'л', str );
	U_TEST_ASSERT( str.size() ==  6 );
	PushCharToUTF8String( U'е', str );
	U_TEST_ASSERT( str.size() ==  8 );
	PushCharToUTF8String( U'с', str );
	U_TEST_ASSERT( str.size() == 10 );
	PushCharToUTF8String( '-', str );
	U_TEST_ASSERT( str.size() == 11 );
	PushCharToUTF8String( U'☯', str );
	U_TEST_ASSERT( str.size() == 14 );
	PushCharToUTF8String( U'♂', str );
	U_TEST_ASSERT( str.size() == 17 );
	PushCharToUTF8String( U'☭', str );
	U_TEST_ASSERT( str.size() == 20 );
	PushCharToUTF8String( U'💀', str );
	U_TEST_ASSERT( str.size() == 24 );

	U_TEST_ASSERT( str == text );
}

U_TEST( Utf8PositionToUtf16Position_Test )
{
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text,  0 ) == uint32_t(  0 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text,  1 ) == uint32_t(  1 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text,  2 ) == uint32_t(  2 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text,  3 ) == uint32_t(  3 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text,  4 ) == uint32_t(  4 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text,  6 ) == uint32_t(  5 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text,  8 ) == uint32_t(  6 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text, 10 ) == uint32_t(  7 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text, 11 ) == uint32_t(  8 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text, 14 ) == uint32_t(  9 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text, 17 ) == uint32_t( 10 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text, 20 ) == uint32_t( 11 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text, 24 ) == uint32_t( 13 ) );
	U_TEST_ASSERT( Utf8PositionToUtf16Position( text, 999 ) == std::nullopt );
}

U_TEST( Utf8PositionToUtf32Position_Test )
{
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text,  0 ) == uint32_t(  0 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text,  1 ) == uint32_t(  1 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text,  2 ) == uint32_t(  2 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text,  3 ) == uint32_t(  3 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text,  4 ) == uint32_t(  4 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text,  6 ) == uint32_t(  5 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text,  8 ) == uint32_t(  6 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text, 10 ) == uint32_t(  7 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text, 11 ) == uint32_t(  8 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text, 14 ) == uint32_t(  9 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text, 17 ) == uint32_t( 10 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text, 20 ) == uint32_t( 11 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text, 24 ) == uint32_t( 12 ) );
	U_TEST_ASSERT( Utf8PositionToUtf32Position( text, 999 ) == std::nullopt );
}

U_TEST( Utf16PositionToUtf8Position_Test )
{
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text,  0 ) == uint32_t(  0 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text,  1 ) == uint32_t(  1 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text,  2 ) == uint32_t(  2 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text,  3 ) == uint32_t(  3 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text,  4 ) == uint32_t(  4 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text,  5 ) == uint32_t(  6 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text,  6 ) == uint32_t(  8 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text,  7 ) == uint32_t( 10 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text,  8 ) == uint32_t( 11 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text,  9 ) == uint32_t( 14 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text, 10 ) == uint32_t( 17 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text, 11 ) == uint32_t( 20 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text, 13 ) == uint32_t( 24 ) );
	U_TEST_ASSERT( Utf16PositionToUtf8Position( text, 999 ) == std::nullopt );
}

U_TEST( Utf32PositionToUtf8Position_Test )
{
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text,  0 ) == uint32_t(  0 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text,  1 ) == uint32_t(  1 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text,  2 ) == uint32_t(  2 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text,  3 ) == uint32_t(  3 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text,  4 ) == uint32_t(  4 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text,  5 ) == uint32_t(  6 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text,  6 ) == uint32_t(  8 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text,  7 ) == uint32_t( 10 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text,  8 ) == uint32_t( 11 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text,  9 ) == uint32_t( 14 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text, 10 ) == uint32_t( 17 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text, 11 ) == uint32_t( 20 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text, 12 ) == uint32_t( 24 ) );
	U_TEST_ASSERT( Utf32PositionToUtf8Position( text, 999 ) == std::nullopt );
}

} // namespace

} // namespace U
