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

void RunLinkerCOFF( const char* const argv0, const llvm::Triple& triple, const std::string& input_temp_file_path, const std::string& output_file_path )
{
	llvm::raw_os_ostream cout(std::cout);
	llvm::raw_os_ostream cerr(std::cerr);


	llvm::SmallVector<const char*, 32> args;
	args.push_back( argv0 );
	args.push_back( input_temp_file_path.data() );

	const std::string out_str= "-out:" + output_file_path;
	args.push_back( out_str.data() );

	lld::coff::link( args, cout, cerr, true, false );
}

} // namespace U
