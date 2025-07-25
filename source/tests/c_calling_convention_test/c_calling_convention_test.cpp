#include <cstdint>
#include <iostream>

#define TEST_ASSERT(x) { if( !(x) ) { std::cerr << "Line " << __LINE__ << " assertion failed: " << #x << std::endl; std::abort(); } }

extern "C"
{

void Pass_i8_Test0( const int8_t x )
{
	TEST_ASSERT( x == 27 );
}

void Pass_i8_Test1( const int8_t x )
{
	TEST_ASSERT( x == -58 );
}

void Pass_u8_Test0( const uint8_t x )
{
	TEST_ASSERT( x == 117 );
}

void Pass_u8_Test1( const uint8_t x )
{
	TEST_ASSERT( x == 134 );
}

void Pass_u8_Test2( const uint8_t x )
{
	TEST_ASSERT( x == 249 );
}

void Pass_i16_Test0( const int16_t x )
{
	TEST_ASSERT( x == 27346 );
}

void Pass_i16_Test1( const int16_t x )
{
	TEST_ASSERT( x == -15343 );
}

} // extern "C"
