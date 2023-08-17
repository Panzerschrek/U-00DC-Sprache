#include <iostream>
#include <fstream>
#include "../lex_synt_lib_common/assert.hpp"
#include "connection.hpp"

namespace U
{

#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h>
#include <io.h>

void PlatformInit()
{
	auto res = _setmode( _fileno(stdin), _O_BINARY );
	U_ASSERT(res != -1);
	res = _setmode( _fileno(stdout), _O_BINARY );
	U_ASSERT(res != -1);
}
#else
void platform_init() { }
#endif

} // namespace U

int main()
{
	U::PlatformInit();

	std::ofstream log_file( "C:/Users/user/Documents/Projects/other/U-00DC-Sprache/other/sprache_lang_server.txt", std::ios::app );

	U::Connection connection( std::cin, std::cout );
	while(true)
	{
		log_file << "Message: " << connection.Read() << std::endl;
	}
}
