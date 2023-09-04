#include "../../tests/cpp_tests/tests.hpp"
#include "../lex_synt_lib/program_string.hpp"

namespace U
{

namespace
{

U_TEST( ReadNextUTF8Char_Test0 )
{
	static const char text[]= "abc лес-☯♂☭💀";
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

} // namespace

} // namespace U
