#pragma once
#include "../compiler0/lex_synt_lib/syntax_elements.hpp"

namespace U
{

namespace LangServer
{

using DefinitionRequestPrefixComponent= std::variant<
	const Synt::Namespace*,
	const Synt::Class*,
	const Synt::TypeTemplate*>;

using GetDefinitionRequestItem= std::variant<
	const Synt::NameLookup*,
	const Synt::NameLookupCompletion*,
	const Synt::RootNamespaceNameLookup*,
	const Synt::NamesScopeNameFetch*,
	const Synt::MemberAccessOperator*>;


using GlobalItem= std::variant<const Synt::ProgramElement*, const Synt::ClassElement*>;

struct SyntaxTreeLookupResult
{
	std::vector<DefinitionRequestPrefixComponent> prefix;
	GetDefinitionRequestItem item;
	std::optional<GlobalItem> global_item;
};

using SyntaxTreeLookupResultOpt= std::optional<SyntaxTreeLookupResult>;

// Complexity is linear.
// TODO - return also path (namespace/class/class template + function + (maybe) blocks).
SyntaxTreeLookupResultOpt FindSyntaxElementForPosition( uint32_t line, uint32_t column, const Synt::ProgramElements& program_elements );

} // namespace LangServer

} // namespace U
