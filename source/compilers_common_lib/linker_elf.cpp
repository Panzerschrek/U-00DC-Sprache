#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <lld/Common/Driver.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "linker.hpp"

namespace U
{

void RunLinkerELF( const char* const argv0, const std::string& input_temp_file_path, const std::string& output_file_path )
{
	llvm::raw_os_ostream cout(std::cout);
	llvm::raw_os_ostream cerr(std::cerr);

	// TODO - check if this is correct.
	const bool pic= llvm::codegen::getRelocModel() == llvm::Reloc::PIC_;

	llvm::SmallVector<const char*, 32> args;
	args.push_back( argv0 );
	args.push_back( input_temp_file_path.data() );

	if( pic )
		args.push_back( "-pie" );

	args.push_back( "-z" );
	args.push_back( "relro" );

	args.push_back( "--eh-frame-hdr" );

	args.push_back( "-L" );
	args.push_back( "/usr/lib/x86_64-linux-gnu/" );

	args.push_back( "--dynamic-linker" );
	args.push_back( "/lib64/ld-linux-x86-64.so.2" );

	// ustlib uses some libc and math library functions.
	args.push_back( "-lc" );
	args.push_back( "-lm" );

	// Link against CRT files in order to obtain _start, _init, etc.
	if( pic )
		args.push_back( "/usr/lib/x86_64-linux-gnu/Scrt1.o" );
	else
		args.push_back( "/usr/lib/x86_64-linux-gnu/crt1.o" );
	args.push_back( "/usr/lib/x86_64-linux-gnu/crti.o" );
	args.push_back( "/usr/lib/x86_64-linux-gnu/crtn.o" );

	// TODO - link also against crtbegin.o and crtend.o that are shipped together with GCC.

	args.push_back( "-o" );
	args.push_back( output_file_path.data() );

	lld::elf::link( args, cout, cerr, true, false );
}

} // namespace U
