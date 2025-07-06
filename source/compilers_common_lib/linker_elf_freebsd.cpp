#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <lld/Common/Driver.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "linker.hpp"

LLD_HAS_DRIVER(elf)

namespace U
{

bool RunLinkerELFFreeBSD(
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
	(void)triple;
	(void)debug;

	llvm::raw_os_ostream cout(std::cout);
	llvm::raw_os_ostream cerr(std::cerr);

	// TODO - check if this is correct.
	const bool pic= llvm::codegen::getRelocModel() == llvm::Reloc::PIC_;

	const std::string toolchain_file_path= sysroot + "/usr/lib/";

	llvm::SmallVector<const char*, 32> args;
	args.push_back( argv0 );
	args.push_back( input_temp_file_path.data() );

	if( produce_shared_library )
	{
		args.push_back( "--shared" );

		// Disable undefined symbols in shared libraries.
		// By-default "ld" and compatible linkers silently assume undefined symbols will be resolved in runtime.
		// We don't need such crappy behavior, so, prevent undefined symbols.
		// Also, there is a place in Hell reserved for the person who decided to make such behavior default.
		args.push_back( "--no-undefined" );
	}

	if( pic && !produce_shared_library )
		args.push_back( "-pie" );

	if( remove_unreferenced_symbols )
		args.push_back( "-s" );

	args.push_back( "-z" );
	args.push_back( "relro" );

	args.push_back( "--eh-frame-hdr" );

	args.push_back( "-L" );
	args.push_back( toolchain_file_path.data() );

	const std::string dynamic_linker= "/libexec/ld-elf.so.1";
	args.push_back( "--dynamic-linker" );
	args.push_back( dynamic_linker.data() );

	// Push this flag before specifying libraries to link only needed shared libraries.
	args.push_back( "--as-needed" );

	// ustlib uses some libc and math library functions.
	args.push_back( "-lc" );
	args.push_back( "-lm" );

	// Link against CRT files in order to obtain _start, _init, etc.
	const std::string crt1= toolchain_file_path + (pic ? "Scrt1.o" : "crt1.o");
	const std::string crti= toolchain_file_path + "crti.o";
	const std::string crtn= toolchain_file_path + "crtn.o";

	if( !produce_shared_library )
		args.push_back( crt1.data() );
	args.push_back( crti.data() );
	args.push_back( crtn.data() );

	// TODO - link also against crtbegin.o and crtend.o that are shipped together with GCC.

	args.push_back( "-o" );
	args.push_back( output_file_path.data() );

	for( const std::string& arg : additional_args )
		args.push_back( arg.data() );

	return lld::elf::link( args, cout, cerr, true, false );
}

} // namespace U
