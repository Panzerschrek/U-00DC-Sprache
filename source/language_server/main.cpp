#include <fstream>
#include <iostream>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/InitLLVM.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../lex_synt_lib_common/assert.hpp"
#include "options.hpp"
#include "server.hpp"

// Messy stuff.
// Without it language server doesn't work on Windows.
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
void PlatformInit() {}
#endif

namespace U
{

namespace LangServer
{

namespace
{

int Main( int argc, const char* argv[] )
{
	const llvm::InitLLVM llvm_initializer(argc, argv);

	llvm::cl::HideUnrelatedOptions( Options::options_category );
	llvm::cl::ParseCommandLineOptions( argc, argv, "Ü-Sprache language server\n" );

	PlatformInit();

	std::ofstream log_file( Options::log_file_path );

	log_file << "Start language server" << std::endl;

	ServerHandler handler( log_file );
	Server server( Connection( std::cin, std::cout ), handler, log_file );
	server.Run();

	log_file << "End language server" << std::endl;

	return 0;
}

} // namespace

} // namespace LangServer

} // namespace U

int main( const int argc, const char* argv[] )
{
	return U::LangServer::Main( argc, argv );
}
