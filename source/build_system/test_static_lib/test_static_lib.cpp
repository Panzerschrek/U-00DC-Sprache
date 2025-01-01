#include <cstdint>
#include <cstddef>
#include <cstdio>

extern "C" uint32_t StaticLibFunc( const uint32_t x, const uint32_t y )
{
	return x * x + 3 * y;
}

extern "C" bool CreateFileWithContents(
	const char* const name_null_terminated,
	const std::byte* const contents,
	const size_t contents_size )
{
	const auto f= std::fopen( name_null_terminated, "wb" );
	if( f == nullptr )
		return false;

	// TODO - read in loop.
	const size_t written= std::fwrite( contents, 1, contents_size, f );
	if( written != contents_size )
	{
		std::fclose(f);
		return false;
	}

	std::fclose(f);

	return true;
}
