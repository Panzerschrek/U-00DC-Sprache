#include <iostream>
#include <fstream>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
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

int Main( int argc, const char* argv[] )
{
	const llvm::InitLLVM llvm_initializer(argc, argv);

	namespace cl= llvm::cl;
	cl::OptionCategory options_category( "Ü language server options" );

	cl::opt<std::string> log_file_path(
		"log-file",
		cl::desc("Log file name"),
		cl::value_desc("filename"),
		cl::Optional,
		cl::cat(options_category) );

	llvm::cl::HideUnrelatedOptions( options_category );
	llvm::cl::ParseCommandLineOptions( argc, argv, "Ü-Sprache language server\n" );

	PlatformInit();

	std::ofstream log_file( log_file_path, std::ios::app );

	ServerHandler handler( log_file );
	Server server( Connection( std::cin, std::cout ), handler, log_file );
	server.Run();

	return 0;
}

} // namespace LangServer

} // namespace U

int main( const int argc, const char* argv[] )
{
	return U::LangServer::Main( argc, argv );
}
