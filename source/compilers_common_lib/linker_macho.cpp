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

namespace
{

std::string GetMachOArchName( const llvm::Triple& triple )
{
	switch( triple.getArch() )
	{
	case llvm::Triple::aarch64:
		return triple.isArm64e() ? "arm64e" : "arm64";
	case llvm::Triple::aarch64_32:
		return "arm64_32";
	case llvm::Triple::ppc:
		return "ppc";
	case llvm::Triple::ppcle:
		return "ppcle";
	case llvm::Triple::ppc64:
		return "ppc64";
	case llvm::Triple::ppc64le:
		return "ppc64le";
	case llvm::Triple::thumb:
	case llvm::Triple::arm:
		return "arm";
	default:
		return std::string( triple.getArchName() );
	}
}

} // namespace

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
	(void)remove_unreferenced_symbols;
	(void)debug;

	llvm::raw_os_ostream cout(std::cout);
	llvm::raw_os_ostream cerr(std::cerr);

	llvm::SmallVector<const char*, 32> args;
	args.push_back( argv0 );
	args.push_back( input_temp_file_path.data() );

	// Set some reasonable defaults.
	// This can be overriden with something like -Wl,-platform_version,macos,14.0.0,14.0.
	args.push_back( "-platform_version" );
	args.push_back( "macos" );
	args.push_back( "15.0.0" ); // min_version
	args.push_back( "15.0" ); // SDK version

	const std::string arch_name= GetMachOArchName( triple );
	args.push_back( "-arch" );
	args.push_back( arch_name.data() );

	if( produce_shared_library )
		args.push_back( "-dylib" );

	if( !sysroot.empty() )
	{
		args.push_back( "-syslibroot" );
		args.push_back( sysroot.data() );
	}

	args.push_back( "-o" );
	args.push_back( output_file_path.data() );

	// ustlib uses some libc and math library functions.
	args.push_back( "-lc" );
	args.push_back( "-lm" );

	for( const std::string& arg : additional_args )
		args.push_back( arg.data() );

	return lld::macho::link( args, cout, cerr, true, false );
}

} // namespace U
