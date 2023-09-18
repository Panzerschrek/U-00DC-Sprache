#pragma once
#include "../compiler0/code_builder_lib/code_builder.hpp"

namespace U
{

namespace LangServer
{

using CompletionSyntaxElement= std::variant<
	const Synt::RootNamespaceNameLookupCompletion*,
	const Synt::NameLookupCompletion*,
	const Synt::NamesScopeNameFetchCompletion*,
	const Synt::MemberAccessOperatorCompletion*,
	const Synt::StructNamedInitializer::MemberInitializer*,
	const Synt::MoveOperator*,
	const Synt::Function::NameComponent*,
	const Synt::CallOperatorSignatureHelp*>;

using GlobalItem= std::variant<const Synt::ProgramElement*, const Synt::ClassElement*>;

struct SyntaxTreeLookupResult
{
	std::vector<CodeBuilder::CompletionRequestPrefixComponent> prefix;
	CompletionSyntaxElement element;
	GlobalItem global_item;
};

using SyntaxTreeLookupResultOpt= std::optional<SyntaxTreeLookupResult>;

// Complexity is linear.
SyntaxTreeLookupResultOpt FindCompletionSyntaxElement( uint32_t line, uint32_t column, const Synt::ProgramElements& program_elements );

} // namespace LangServer

} // namespace U
