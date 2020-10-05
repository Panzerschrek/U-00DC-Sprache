#pragma once
#include <string>
#include "../lex_synt_lib/source_graph_loader.hpp"

namespace U
{

namespace DepFile
{

// Built-in dependency management.
// Allows to skip main compiler functionality invokation if notging is changed in source file and its dependencies.

bool NothingChanged( const std::string& out_file_path, int argc, const char* const argv[] );
void Write( const std::string& out_file_path, int argc, const char* const argv[], const std::vector<IVfs::Path>& deps_list );

} // namespace DepFile

} // namespace U
