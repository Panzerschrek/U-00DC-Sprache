#include "cpp_test.hpp"

extern "C" int ExternCPrefixedFunc( const int x )
{
	return x * x + 6 * x - 3;
}

extern "C"
{

float ExternCBlockFunc0( const float x, const float y )
{
	return x * y * 3.25f - 0.75f;
}

unsigned int ExternCBlockFunc1()
{
	return 0xDEADBEEFu;
}

} // extern "C"
