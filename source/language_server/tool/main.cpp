#include <fstream>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Signals.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "../options.hpp"
#include "../async_server.hpp"

int main( const int argc, const char* argv[] );

namespace U
{

namespace LangServer
{

namespace
{

void PrintStackTraceSignalHandler(void*)
{
	const std::string path= Options::error_log_file_path;
	if( !path.empty() )
	{
		std::error_code file_error_code;
		llvm::raw_fd_ostream file_stream( path, file_error_code );
		llvm::sys::PrintStackTrace(file_stream);
	}
}

std::string GetInstallationDirectory( const char* const argv0 )
{
	const std::string executable= llvm::sys::fs::getMainExecutable( argv0, reinterpret_cast<void*>( &::main ) );
	// Remove executable file name and "bin" directory.
	return llvm::sys::path::parent_path( llvm::sys::path::parent_path( executable ) ).str();
}

int Main( int argc, const char* argv[] )
{
	const llvm::InitLLVM llvm_initializer(argc, argv);

	llvm::sys::AddSignalHandler( PrintStackTraceSignalHandler, nullptr );

	llvm::cl::HideUnrelatedOptions( Options::options_category );
	llvm::cl::ParseCommandLineOptions( argc, argv, "Ü-Sprache language server\n" );

	const std::string log_file_path= Options::log_file_path;

	std::ofstream log_file;
	if( !log_file_path.empty() )
		log_file.open( log_file_path );

	Logger logger( log_file );

	std::string installation_directory= GetInstallationDirectory( argv[0] );

	logger() << "Installation directory: " << installation_directory << std::endl;

	logger() << "Start language server" << std::endl;

	RunAsyncServer( logger, std::move(installation_directory) );

	logger() << "End language server" << std::endl;

	return 0;
}

} // namespace

} // namespace LangServer

} // namespace U

int main( const int argc, const char* argv[] )
{
	return U::LangServer::Main( argc, argv );
}
