#include <cstdint>
#include <cstddef>

extern "C" uint32_t StaticLibFunc( const uint32_t x, const uint32_t y )
{
	return x * x + 3 * y;
}
