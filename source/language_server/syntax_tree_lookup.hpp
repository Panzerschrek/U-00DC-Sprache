#pragma once
#include "../compiler0/code_builder_lib/code_builder.hpp"

namespace U
{

namespace LangServer
{

struct SyntaxTreeLookupResult
{
	llvm::SmallVector<CodeBuilder::DefinitionRequestPrefixComponent, 4> prefix;
	CodeBuilder::GetDefinitionRequestItem item;
};

using SyntaxTreeLookupResultOpt= std::optional<SyntaxTreeLookupResult>;

// Complexity is linear.
// TODO - return also path (namespace/class/class template + function + (maybe) blocks).
SyntaxTreeLookupResultOpt FindSyntaxElementForPosition( uint32_t line, uint32_t column, const Synt::ProgramElements& program_elements );

} // namespace LangServer

} // namespace U
