#pragma once
#include "../../code_builder_lib_common/code_builder_errors.hpp"
#include "../../lex_synt_lib_common/errors_format.hpp"
#include "source_graph_loader.hpp"

namespace U
{

void PrintErrors( const SourceGraph& source_graph, const CodeBuilderErrorsContainer& errors, ErrorsFormat format );
void PrintErrorsForTests( const SourceGraph& source_graph, const CodeBuilderErrorsContainer& errors );

} // namespace U
