#include "linker.hpp"

namespace U
{

void RunLinker( const char* const argv0, const std::string& input_temp_file_path, const std::string& output_file_path )
{
	// TODO - select proper linker.
	return RunLinkerELF( argv0, input_temp_file_path, output_file_path );
}

} // namespace U
