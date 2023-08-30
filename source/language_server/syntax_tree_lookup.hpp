#pragma once
#include "../compiler0/code_builder_lib/code_builder.hpp"

namespace U
{

namespace LangServer
{


using GetDefinitionRequestItem= std::variant<
	const Synt::NameLookup*,
	const Synt::NameLookupCompletion*,
	const Synt::RootNamespaceNameLookup*,
	const Synt::NamesScopeNameFetch*,
	const Synt::NamesScopeNameFetchCompletion*,
	const Synt::MemberAccessOperator*,
	const Synt::MemberAccessOperatorCompletion*,
	const Synt::StructNamedInitializer::MemberInitializer*>;

using GlobalItem= std::variant<const Synt::ProgramElement*, const Synt::ClassElement*>;

struct SyntaxTreeLookupResult
{
	std::vector<CodeBuilder::CompletionRequestPrefixComponent> prefix;
	GetDefinitionRequestItem item;
	std::optional<GlobalItem> global_item;
};

using SyntaxTreeLookupResultOpt= std::optional<SyntaxTreeLookupResult>;

// Complexity is linear.
// TODO - return also path (namespace/class/class template + function + (maybe) blocks).
SyntaxTreeLookupResultOpt FindSyntaxElementForPosition( uint32_t line, uint32_t column, const Synt::ProgramElements& program_elements );

} // namespace LangServer

} // namespace U
