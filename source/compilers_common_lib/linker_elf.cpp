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

namespace
{

// This code was borrowed from clang and made slightly ugly.
std::string GetLinuxDynamicLinker( const llvm::Triple& triple )
{
	std::string lib_dir;
	std::string loader;

	switch( triple.getArch() )
	{
	default:
		llvm_unreachable("unsupported architecture");

	case llvm::Triple::aarch64:
		lib_dir= "lib";
		loader = "ld-linux-aarch64.so.1";
		break;

	case llvm::Triple::aarch64_be:
		lib_dir= "lib";
		loader= "ld-linux-aarch64_be.so.1";
		break;

	case llvm::Triple::arm:
	case llvm::Triple::thumb:
	case llvm::Triple::armeb:
	case llvm::Triple::thumbeb:
		lib_dir= "lib";
		loader="ld-linux.so.3";
		break;

	case llvm::Triple::sparc:
	case llvm::Triple::sparcel:
		lib_dir= "lib";
		loader= "ld-linux.so.2";
		break;

	case llvm::Triple::sparcv9:
		lib_dir= "lib64";
		loader= "ld-linux.so.2";
		break;

	case llvm::Triple::systemz:
		lib_dir= "lib";
		loader= "ld64.so.1";
		break;

	case llvm::Triple::x86:
		lib_dir= "lib";
		loader= "ld-linux.so.2";
		break;

	case llvm::Triple::x86_64:
		lib_dir= triple.isX32() ? "libx32" : "lib64";
		loader= triple.isX32() ? "ld-linux-x32.so.2" : "ld-linux-x86-64.so.2";
		break;

	case llvm::Triple::ve:
		return "/opt/nec/ve/lib/ld-linux-ve.so.1";

	case llvm::Triple::csky:
		lib_dir= "lib";
		loader= "ld.so.1";
		break;
	}

	return "/" + lib_dir + "/" + loader;
}

// This code was borrowed from clang and made slightly ugly.
std::string GetLinuxMultiarchTriple( const llvm::Triple& triple )
{
	// For most architectures, just use whatever we have rather than trying to be clever.
	switch( triple.getArch() )
	{
	default:
		break;

	case llvm::Triple::arm:
	case llvm::Triple::thumb:
		if(triple.isAndroid() )
			return "arm-linux-androideabi";
		if( triple.getEnvironment() == llvm::Triple::GNUEABIHF )
			return "arm-linux-gnueabihf";
		return "arm-linux-gnueabi";

	case llvm::Triple::armeb:
	case llvm::Triple::thumbeb:
		if( triple.getEnvironment() == llvm::Triple::GNUEABIHF )
			return "armeb-linux-gnueabihf";
		return "armeb-linux-gnueabi";

	case llvm::Triple::x86:
	if( triple.isAndroid() )
		return "i686-linux-android";
	return "i386-linux-gnu";

	case llvm::Triple::x86_64:
		if( triple.isAndroid() )
			return "x86_64-linux-android";
		if( triple.getEnvironment() == llvm::Triple::GNUX32 )
			return "x86_64-linux-gnux32";
		return "x86_64-linux-gnu";

	case llvm::Triple::aarch64:
		if( triple.isAndroid() )
			return "aarch64-linux-android";
		return "aarch64-linux-gnu";

	case llvm::Triple::aarch64_be:
		return "aarch64_be-linux-gnu";

	case llvm::Triple::m68k:
		return "m68k-linux-gnu";

	case llvm::Triple::riscv64:
		return "riscv64-linux-gnu";

	case llvm::Triple::sparc:
		return "sparc-linux-gnu";

	case llvm::Triple::sparcv9:
		return "sparc64-linux-gnu";

	case llvm::Triple::systemz:
		return "s390x-linux-gnu";
	}

	return triple.str();
}

} // namespace

void RunLinkerELF( const char* const argv0, const llvm::Triple& triple, const std::string& input_temp_file_path, const std::string& output_file_path )
{
	llvm::raw_os_ostream cout(std::cout);
	llvm::raw_os_ostream cerr(std::cerr);

	// TODO - check if this is correct.
	const bool pic= llvm::codegen::getRelocModel() == llvm::Reloc::PIC_;

	const std::string toolchain_file_path= "/usr/lib/" + GetLinuxMultiarchTriple( triple ) + "/";

	llvm::SmallVector<const char*, 32> args;
	args.push_back( argv0 );
	args.push_back( input_temp_file_path.data() );

	if( pic )
		args.push_back( "-pie" );

	args.push_back( "-z" );
	args.push_back( "relro" );

	args.push_back( "--eh-frame-hdr" );

	args.push_back( "-L" );
	args.push_back( toolchain_file_path.data() );

	const std::string dynamic_linker= GetLinuxDynamicLinker( triple );
	args.push_back( "--dynamic-linker" );
	args.push_back( dynamic_linker.data() );

	// ustlib uses some libc and math library functions.
	args.push_back( "-lc" );
	args.push_back( "-lm" );

	// Link against CRT files in order to obtain _start, _init, etc.
	const std::string crt1= toolchain_file_path + (pic ? "Scrt1.o" : "crt1.o");
	const std::string crti= toolchain_file_path + "crti.o";
	const std::string crtn= toolchain_file_path + "crtn.o";

	args.push_back( crt1.data() );
	args.push_back( crti.data() );
	args.push_back( crtn.data() );

	// TODO - link also against crtbegin.o and crtend.o that are shipped together with GCC.

	args.push_back( "-o" );
	args.push_back( output_file_path.data() );

	lld::elf::link( args, cout, cerr, true, false );
}

} // namespace U
