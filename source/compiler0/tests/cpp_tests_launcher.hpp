#pragma once
#include "../../tests/cpp_tests/tests.hpp"
#include "../code_builder_lib/code_builder.hpp"

namespace U
{

std::unique_ptr<CodeBuilder> BuildProgramForIdeHelpersTest( const char* const text );

} // namespace U
