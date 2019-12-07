#pragma once
#include "names_scope.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

struct Enum
{
	Enum( const ProgramString& name, NamesScope* parent_scope );

	NamesScope members;
	size_t element_count= 0u;
	FundamentalType underlaying_type; // must be integer

	const Synt::Enum* syntax_element= nullptr; // Null if completed
};

} //namespace CodeBuilderPrivate

} // namespace U
