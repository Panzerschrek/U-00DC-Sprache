#include <fstream>
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/Signals.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"
#include "../../lex_synt_lib_common/assert.hpp"
#include "../options.hpp"
#include "../async_server.hpp"

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
		file_stream << "SHIT HAPPENS";
		llvm::sys::PrintStackTrace(file_stream);
		file_stream.flush();
	}
}

int Main( int argc, const char* argv[] )
{
	try
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

		logger() << "Start language server" << std::endl;

		if( !Options::error_log_file_path.empty() )
			logger() << "Error log file path: " << Options::error_log_file_path << std::endl;

		RunAsyncServer( logger );

		logger() << "End language server" << std::endl;
	}
	catch( const std::exception& ex )
	{
		const std::string path= Options::error_log_file_path;
		if( !path.empty() )
		{
			std::error_code file_error_code;
			llvm::raw_fd_ostream file_stream( path, file_error_code );
			file_stream << "Crashed with exception: " << ex.what();
			file_stream.flush();
		}
	}

	return 0;
}

} // namespace

} // namespace LangServer

} // namespace U

int main( const int argc, const char* argv[] )
{
	return U::LangServer::Main( argc, argv );
}
