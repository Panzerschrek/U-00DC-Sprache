#include "linker.hpp"

namespace U
{

bool RunLinker(
	const char* const argv0,
	const llvm::ArrayRef<std::string> additional_args,
	const llvm::Triple& triple,
	const std::string& input_temp_file_path,
	const std::string& output_file_path,
	const bool produce_shared_library )
{
	// TODO - support other targets.
	if( triple.getOS() == llvm::Triple::Win32 )
	{
		// TODO - support MinGW
		return RunLinkerCOFF( argv0, additional_args, triple, input_temp_file_path, output_file_path, produce_shared_library );
	}
	else
		return RunLinkerELF( argv0, additional_args, triple, input_temp_file_path, output_file_path, produce_shared_library );
}

} // namespace U
