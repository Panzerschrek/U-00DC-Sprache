#pragma once
#include "../compiler0/lex_synt_lib/source_graph_loader.hpp"

namespace U
{

void DeduplicateAndFilterDepsList( std::vector<IVfs::Path>& deps_list );

bool WriteDepFile(
	const std::string& out_file_path,
	const std::vector<IVfs::Path>& deps_list, // Files list should not contain duplicates.
	const std::string& dep_file_path );

} // namespace U
