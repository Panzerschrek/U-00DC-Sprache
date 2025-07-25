#include <cstdint>
#include <iostream>

#define TEST_ASSERT(x) { if( !(x) ) { std::cerr << "Line " << __LINE__ << " assertion failed: " << #x << std::endl; std::abort(); } }

extern "C"
{

void PassI8_Test0( const int8_t x )
{
	TEST_ASSERT( x == 27 );
}

} // extern "C"
