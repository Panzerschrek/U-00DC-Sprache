#include <cstdint>

extern "C" int __some_cpp_function( const int x )
{
	return x * x;
}

extern "C" uint64_t if_coro_advance( const uint64_t x, const uint64_t y )
{
	return x - y;
}

extern const float __cpp_global_constant= 2.375f;

uint64_t generator= 77u;

extern "C" uint64_t GetGeneratorValue()
{
	return generator;
}
