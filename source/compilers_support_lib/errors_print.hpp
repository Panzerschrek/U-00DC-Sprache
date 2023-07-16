#pragma once
#include "../code_builder_lib_common/code_builder_errors.hpp"
#include "../compiler0/lex_synt_lib/source_graph_loader.hpp"

namespace U
{

void PrintErrors( const std::vector<IVfs::Path>& source_files, const CodeBuilderErrorsContainer& errors, ErrorsFormat format );
void PrintErrorsForTests( const std::vector<IVfs::Path>& source_files, const CodeBuilderErrorsContainer& errors );

} // namespace U
