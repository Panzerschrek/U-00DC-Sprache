#pragma once
#include "../../lex_synt_lib_common/errors_format.hpp"
#include "source_graph_loader.hpp"

namespace U
{

void PrintLexSyntErrors( const SourceGraph& source_graph, ErrorsFormat format= ErrorsFormat::GCC, std::ostream& errors_stream= std::cerr );

} // namespace U
