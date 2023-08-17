#include <iostream>
#include <fstream>
#include <sstream>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/JSON.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../lex_synt_lib_common/assert.hpp"
#include "server.hpp"

namespace U
{

namespace LangServer
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

int Main()
{
	PlatformInit();

	std::ofstream log_file( "C:/Users/user/Documents/Projects/other/U-00DC-Sprache/other/sprache_lang_server.txt", std::ios::app  );

	ServerHandler handler( log_file );
	Server server( Connection( std::cin, std::cout ), handler, log_file );
	server.Run();

	return 0;
}

} // namespace LangServer

} // namespace U

int main()
{
	return U::LangServer::Main();
}
