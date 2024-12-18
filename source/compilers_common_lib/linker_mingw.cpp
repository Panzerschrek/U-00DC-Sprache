#include <iostream>

#include "../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <lld/Common/Driver.h>
#include <llvm/ADT/Triple.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/Support/Filesystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/VersionTuple.h>
#include <llvm/Support/raw_os_ostream.h>
#include "../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "linker.hpp"

namespace U
{

namespace
{

std::string GetSubdirName( const std::string& sysroot, const llvm::Triple& triple )
{
	std::string subdirs[]
	{
		(triple.getArchName() + "-w64-mingw32").str(),
		"mingw32",
	};
	for( const std::string& subdir : subdirs )
	{
		if( llvm::sys::fs::exists( sysroot + "/" + subdir ) )
			return subdir;
	}
	return "";
}

std::string GetLibGCCPath( const std::string& sysroot, const std::string& subdir )
{
	const char* const lib_dirs[]{ "lib", "lib64" };

	for( const char* const lib_dir : lib_dirs )
	{
		const std::string lib_gcc_dir = sysroot + "/" + lib_dir + "/gcc/" + subdir;

		llvm::VersionTuple newest_version;
		std::string newest_version_path;

		std::error_code ec;
		for( llvm::sys::fs::directory_iterator it(lib_gcc_dir, ec), it_end;
			!ec && it != it_end;
			it = it.increment(ec))
		{
			llvm::VersionTuple version;
			if( !version.tryParse( llvm::sys::path::filename(it->path()) ) )
			{
				if( version > newest_version )
				{
					newest_version= version;
					newest_version_path= it->path();
				}
			}
		}

		if( !newest_version_path.empty() )
			return newest_version_path;
	}

	return "";
}

} // namespace

bool RunLinkerMinGW(
	const char* const argv0,
	const llvm::ArrayRef<std::string> additional_args,
	const std::string& sysroot,
	const llvm::Triple& triple,
	const std::string& input_temp_file_path,
	const std::string& output_file_path,
	const bool produce_shared_library,
	const bool remove_unreferenced_symbols )
{
	const std::string subdir= GetSubdirName( sysroot, triple );
	if( subdir.empty() )
	{
		std::cerr << "Failed to find MinGW installation in sysroot \"" << sysroot << "\" for architecture " << triple.getArchName().str() << "!" << std::endl;
		return false;
	}

	const std::string lib_gcc_path = GetLibGCCPath( sysroot, subdir );
	if( lib_gcc_path.empty() )
	{
		std::cerr << "Failed to find MinGW lib_gcc path in sysroot \"" << sysroot << "\" for architecture " << triple.getArchName().str() << "!" << std::endl;
		return false;
	}

	llvm::raw_os_ostream cout(std::cout);
	llvm::raw_os_ostream cerr(std::cerr);

	// TODO - check if this is correct.
	//const bool pic= llvm::codegen::getRelocModel() == llvm::Reloc::PIC_;

	llvm::SmallVector<const char*, 32> args;
	args.push_back( argv0 );
	args.push_back( input_temp_file_path.data() );

	if( produce_shared_library )
	{
		args.push_back( "--shared" );

		args.push_back("-e");
		if( triple.getArch() == llvm::Triple::x86 )
			args.push_back( "_DllMainCRTStartup@12" );
		else
			args.push_back( "DllMainCRTStartup" );
		args.push_back("--enable-auto-image-base");
	}

	//if( pic && !produce_shared_library )
	//	args.push_back( "-pie" );

	if( remove_unreferenced_symbols )
		args.push_back( "-s" );

	const std::string lib_path= sysroot + "/" + subdir + "/lib/";

	args.push_back( "-L" );
	args.push_back( lib_path.data() );

	args.push_back( "-L" );
	args.push_back( lib_gcc_path.data() );

	const std::string crt2= lib_path + ( produce_shared_library ? "dllcrt2.o" : "crt2.o" );
	const std::string crtbegin= lib_path + "crtbegin.o";
	const std::string crtend= lib_path + "crtend.o";

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
