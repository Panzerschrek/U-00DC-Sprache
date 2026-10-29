#pragma once
#include "../../code_builder_lib_common/code_builder_errors.hpp"
#include "../../lex_synt_lib_common/errors_format.hpp"
#include "i_vfs.hpp"

namespace U
{

void PrintErrors( const std::vector<IVfs::Path>& source_files, const CodeBuilderErrorsContainer& errors, ErrorsFormat format );
void PrintErrorsForTests( const std::vector<IVfs::Path>& source_files, const CodeBuilderErrorsContainer& errors );

} // namespace U
