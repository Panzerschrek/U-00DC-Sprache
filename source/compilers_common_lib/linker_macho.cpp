#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <lld/Common/Driver.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "linker.hpp"

LLD_HAS_DRIVER(macho)

namespace U
{

bool RunLinkerMachO(
	const char* argv0,
	llvm::ArrayRef<std::string> additional_args,
	const std::string& sysroot,
	const llvm::Triple& triple,
	const std::string& input_temp_file_path,
	const std::string& output_file_path,
	bool produce_shared_library,
	bool remove_unreferenced_symbols,
	bool debug )
{
	(void)triple;
	(void)remove_unreferenced_symbols;
	(void)debug;

	llvm::raw_os_ostream cout(std::cout);
	llvm::raw_os_ostream cerr(std::cerr);

	llvm::SmallVector<const char*, 32> args;
	args.push_back( argv0 );
	args.push_back( input_temp_file_path.data() );

	if( produce_shared_library )
		args.push_back( "-dylib" );

	if( !sysroot.empty() )
	{
		args.push_back( "-syslibroot" );
		args.push_back( sysroot.data() );
	}

	args.push_back( "-o" );
	args.push_back( output_file_path.data() );

	if( !produce_shared_library )
		args.push_back( "-lcrt0.o" );

	for( const std::string& arg : additional_args )
		args.push_back( arg.data() );

	return lld::macho::link( args, cout, cerr, true, false );
}

} // namespace U
