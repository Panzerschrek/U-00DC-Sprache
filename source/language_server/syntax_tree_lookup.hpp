#pragma once
#include "../compiler0/lex_synt_lib/syntax_elements.hpp"

namespace U
{

namespace LangServer
{

using NamedSyntaxElement= std::variant<
	Synt::EmptyVariant,
	const Synt::NameLookup*,
	const Synt::RootNamespaceNameLookup*,
	const Synt::NamesScopeNameFetch*,
	const Synt::MemberAccessOperator* >;

// Complexity is linear.
// TODO - return also path (namespace/class/class template + function + (maybe) blocks).
NamedSyntaxElement FindSyntaxElementForPosition( uint32_t line, uint32_t column, const Synt::ProgramElements& program_elements );

} // namespace LangServer

} // namespace U
