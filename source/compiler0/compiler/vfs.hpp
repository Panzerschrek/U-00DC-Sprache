#pragma once
#include "../lex_synt_lib/source_graph_loader.hpp"

namespace U
{

std::shared_ptr<IVfs> CreateVfsOverSystemFS( const std::vector<std::string>& include_dirs );

} // namespace U
