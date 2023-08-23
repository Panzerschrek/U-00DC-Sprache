#pragma once
#include <memory>

#include "../compiler0/code_builder_lib/code_builder.hpp"

namespace U
{

std::vector<CodeBuilder::Symbol> BuildSymbols( const Synt::ProgramElements& program_elements );

} // namespace U
