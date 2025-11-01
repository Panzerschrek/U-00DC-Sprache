#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <lld/Common/Driver.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "linker.hpp"

LLD_HAS_DRIVER(wasm)

namespace U
{

bool RunLinkerWasm(
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
	(void)produce_shared_library;
	(void)remove_unreferenced_symbols;
	(void)debug;

	llvm::raw_os_ostream cout(std::cout);
	llvm::raw_os_ostream cerr(std::cerr);

	llvm::SmallVector<const char*, 32> args;
	args.push_back( argv0 );
	args.push_back( input_temp_file_path.data() );

	args.push_back( "-o" );
	args.push_back( output_file_path.data() );

	for( const std::string& arg : additional_args )
		args.push_back( arg.data() );

	return lld::wasm::link( args, cout, cerr, true, false );
}

} // namespace U
