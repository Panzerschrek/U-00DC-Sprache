#include <iostream>
#include "linker.hpp"

namespace U
{

bool RunLinker(
	const char* const argv0,
	const llvm::ArrayRef<std::string> additional_args,
	const llvm::Triple& triple,
	const std::string& input_temp_file_path,
	const std::string& output_file_path,
	const bool produce_shared_library,
	const bool remove_unreferenced_symbols )
{
	(void)argv0;
	(void)additional_args;
	(void)triple;
	(void)input_temp_file_path;
	(void)output_file_path;
	(void)produce_shared_library;
	(void)remove_unreferenced_symbols;

	std::cerr << "LLD is not included!" << std::endl;
	return false;
}

} // namespace U
