#include "linker.hpp"

namespace U
{

bool RunLinker(
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
	// TODO - support other linker drivers - MachO, Wasm.
	if( triple.getOS() == llvm::Triple::Win32 )
	{
		if( triple.getEnvironment() == llvm::Triple::GNU )
			return RunLinkerMinGW(
				argv0,
				additional_args,
				sysroot,
				triple,
				input_temp_file_path,
				output_file_path,
				produce_shared_library,
				remove_unreferenced_symbols,
				debug );
		else
			return RunLinkerCOFF(
				argv0,
				additional_args,
				sysroot,
				triple,
				input_temp_file_path,
				output_file_path,
				produce_shared_library,
				remove_unreferenced_symbols,
				debug );
	}
	else if( triple.getOS() == llvm::Triple::MacOSX )
		return RunLinkerMachO(
			argv0,
			additional_args,
			sysroot,
			triple,
			input_temp_file_path,
			output_file_path,
			produce_shared_library,
			remove_unreferenced_symbols,
			debug );
	else if( triple.getOS() == llvm::Triple::FreeBSD )
		return RunLinkerELFFreeBSD(
			argv0,
			additional_args,
			sysroot,
			triple,
			input_temp_file_path,
			output_file_path,
			produce_shared_library,
			remove_unreferenced_symbols,
			debug );
	else
		return RunLinkerELFLinux(
			argv0,
			additional_args,
			sysroot,
			triple,
			input_temp_file_path,
			output_file_path,
			produce_shared_library,
			remove_unreferenced_symbols,
			debug );
}

} // namespace U
