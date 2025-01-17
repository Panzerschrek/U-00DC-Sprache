#include <cstdint>

extern "C" int __some_cpp_function( const int x )
{
	return x * x;
}

extern "C" uint64_t if_coro_advance( const uint64_t x, const uint64_t y )
{
	return x - y;
}
