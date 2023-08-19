#pragma once
#include "../compiler0/code_builder_lib/code_builder.hpp"

namespace U
{

namespace LangServer
{

using NamedSyntaxElement= CodeBuilder::GetDefinitionRequestItem;

// Complexity is linear.
// TODO - return also path (namespace/class/class template + function + (maybe) blocks).
NamedSyntaxElement FindSyntaxElementForPosition( uint32_t line, uint32_t column, const Synt::ProgramElements& program_elements );

} // namespace LangServer

} // namespace U
