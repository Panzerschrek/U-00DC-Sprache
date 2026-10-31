#pragma once
#include "names_scope.hpp"

namespace U
{

struct Enum
{
	Enum( std::string name, NamesScope* parent_scope );

	NamesScope members;
	uint32_t element_count= 0u;
	bool no_discard= false;
	FundamentalType underlying_type; // must be integer

	RcPtr<Synt::Enum> syntax_element; // Null if completed

	// Convert type name to human-readable format.
	void AppendToString( std::string& s ) const;
};

} // namespace U
