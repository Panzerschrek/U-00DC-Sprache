#pragma once
#include <string>

namespace U
{

void RunLinker( const char* argv0, const std::string& input_temp_file_path, const std::string& output_file_path );

void RunLinkerELF( const char* argv0, const std::string& input_temp_file_path, const std::string& output_file_path );

} // namespace U
