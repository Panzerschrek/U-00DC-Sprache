#include <fstream>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/InitLLVM.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../lex_synt_lib_common/assert.hpp"
#include "options.hpp"
#include "async_server.hpp"

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

	std::ofstream log_file( Options::log_file_path );
	Logger logger( log_file );

	logger << "Start language server" << endl;

	RunAsyncServer( logger );

	logger << "End language server" << endl;

	return 0;
}

} // namespace

} // namespace LangServer

} // namespace U

int main( const int argc, const char* argv[] )
{
	return U::LangServer::Main( argc, argv );
}
