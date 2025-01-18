#include <cstdint>

extern "C"
{

int __some_cpp_function( const int x )
{
	return x * x;
}

uint64_t if_coro_advance( const uint64_t x, const uint64_t y )
{
	return x - y;
}

extern const float __cpp_global_constant= 2.375f;

uint64_t generator= 77u;

uint64_t GetGeneratorValue()
{
	return generator;
}

using RetInt= int(*)();

static int Get42()
{
	return 42;
}

extern const RetInt __get_42_fn= Get42;

} // extern "C"
