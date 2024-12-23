#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <lld/Common/Driver.h>
#include <llvm/ADT/Triple.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "linker.hpp"

namespace U
{

bool RunLinkerCOFF(
	const char* const argv0,
	const llvm::ArrayRef<std::string> additional_args,
	const std::string& sysroot,
	const llvm::Triple& triple,
	const std::string& input_temp_file_path,
	const std::string& output_file_path,
	const bool produce_shared_library,
	const bool remove_unreferenced_symbols,
	const bool debug )
{
	(void)sysroot;
	(void)triple;

	llvm::raw_os_ostream cout(std::cout);
	llvm::raw_os_ostream cerr(std::cerr);

	llvm::SmallVector<const char*, 32> args;
	args.push_back( argv0 );
	args.push_back( input_temp_file_path.data() );

	const std::string out_str= "-out:" + output_file_path;
	args.push_back( out_str.data() );

	if( produce_shared_library )
		args.push_back( "/dll" );

	if( remove_unreferenced_symbols )
		args.push_back( "/opt:ref" );

	if( debug )
		args.push_back( "/debug:full" );

	const bool static_link_crt= false; // TODO - allow to specify it.
	args.push_back( static_link_crt ? "-defaultlib:libcmt" : "-defaultlib:msvcrt" );

	args.push_back( "-defaultlib:oldnames" );

	for( const std::string& arg : additional_args )
		args.push_back( arg.data() );

	return lld::coff::link( args, cout, cerr, true, false );
}

} // namespace U
