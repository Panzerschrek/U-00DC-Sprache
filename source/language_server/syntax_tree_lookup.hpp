#pragma once
#include "../compiler0/code_builder_lib/code_builder.hpp"

namespace U
{

namespace LangServer
{

using GlobalItem= std::variant<
	RcPtr<Synt::Namespace>,
	RcPtr<Synt::VariablesDeclaration>,
	RcPtr<Synt::AutoVariableDeclaration>,
	RcPtr<Synt::StaticAssert>,
	RcPtr<Synt::TypeAlias>,
	RcPtr<Synt::Enum>,
	RcPtr<Synt::Function>,
	RcPtr<Synt::ClassField>,
	RcPtr<Synt::ClassVisibilityLabel>,
	RcPtr<Synt::Class>,
	RcPtr<Synt::TypeTemplate>,
	RcPtr<Synt::FunctionTemplate>,
	RcPtr<Synt::Mixin>>;

struct SyntaxTreeLookupResult
{
	std::vector<CodeBuilder::CompletionRequestPrefixComponent> prefix;
	GlobalItem global_item;
};

using SyntaxTreeLookupResultOpt= std::optional<SyntaxTreeLookupResult>;

// Complexity is linear.
SyntaxTreeLookupResultOpt FindCompletionSyntaxElement( uint32_t line, uint32_t column, const Synt::ProgramElementsList& program_elements );

} // namespace LangServer

} // namespace U
