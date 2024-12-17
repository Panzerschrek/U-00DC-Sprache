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

bool RunLinkerMinGW(
	const char* const argv0,
	const llvm::ArrayRef<std::string> additional_args,
	const llvm::Triple& triple,
	const std::string& input_temp_file_path,
	const std::string& output_file_path,
	const bool produce_shared_library,
	const bool remove_unreferenced_symbols )
{
	(void)triple; // TODO - use it.

	llvm::raw_os_ostream cout(std::cout);
	llvm::raw_os_ostream cerr(std::cerr);

	// TODO - check if this is correct.
	const bool pic= llvm::codegen::getRelocModel() == llvm::Reloc::PIC_;

	llvm::SmallVector<const char*, 32> args;
	args.push_back( argv0 );
	args.push_back( input_temp_file_path.data() );

	if( produce_shared_library )
		args.push_back( "--shared" );

	if( pic && !produce_shared_library )
		args.push_back( "-pie" );

	if( remove_unreferenced_symbols )
		args.push_back( "-s" );

	std::string toolchain_file_path= "C:/QtInstall/Tools/mingw730_64/x86_64-w64-mingw32/lib/";

	args.push_back( "-L" );
	args.push_back( toolchain_file_path.data() );

	args.push_back( "-L" );
	args.push_back( "C:/QtInstall/Tools/mingw730_64/lib/gcc/x86_64-w64-mingw32/7.3.0/" );

	std::string crt2= toolchain_file_path + "crt2.o";
	std::string crtbegin= toolchain_file_path + "crtbegin.o";
	std::string crtend= toolchain_file_path + "crtend.o";

	args.push_back( crt2.data() );
	args.push_back( crtbegin.data() );
	args.push_back( crtend.data() );

	args.push_back( "-lmingw32" );
	args.push_back( "-lgcc_s" );
	args.push_back( "-lgcc" );
	args.push_back( "-lmoldname" );
	args.push_back( "-lmingwex" );
	args.push_back( "-lmsvcrt" );

	args.push_back("-lkernel32");

	args.push_back( "-o" );
	args.push_back( output_file_path.data() );

	for( const std::string& arg : additional_args )
		args.push_back( arg.data() );

	return lld::mingw::link( args, cout, cerr, true, false );
}

} // namespace U
