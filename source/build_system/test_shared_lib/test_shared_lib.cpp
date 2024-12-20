#include <cstdint>

#ifdef __GNUC__
#define U_EXTERN_VISIBILITY __attribute__ ((visibility ("default")))
#endif // __GNUC__
#ifdef _MSC_VER
#define U_EXTERN_VISIBILITY __declspec(dllexport)
#endif // _MSC_VER

extern "C" U_EXTERN_VISIBILITY uint32_t SharedLibFunc( const uint32_t x, const uint32_t y )
{
	return x * 7 + y * y * 2;
}
