#pragma once
#include "names_scope.hpp"

namespace U
{

struct Enum
{
	Enum( std::string name, NamesScope* parent_scope );

	NamesScope members;
	size_t element_count= 0u;
	FundamentalType underlying_type; // must be integer

	const Synt::Enum* syntax_element= nullptr; // Null if completed
};

} // namespace U
