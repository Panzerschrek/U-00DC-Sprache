#pragma once
#include "../compiler0/lex_synt_lib/source_graph_loader.hpp"

namespace U
{

std::unique_ptr<IVfs> CreateVfsOverSystemFS( const std::vector<std::string>& include_dirs );

} // namespace U
