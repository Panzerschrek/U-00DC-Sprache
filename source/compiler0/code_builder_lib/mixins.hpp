#pragma once
#include "../lex_synt_lib/syntax_elements.hpp"

namespace U
{

// Mixins how they are stored in names map.
struct Mixins
{
	// This vector should be emptied after mixins preparation.
	// Use vector to keep mixins expansion order stable.
	std::vector<const Synt::Mixin*> syntax_elements;
};

struct MixinExpansionKey
{
	SrcLoc mixin_src_loc;
	std::string mixin_text;

	size_t Hash() const;
};

bool operator==( const MixinExpansionKey& l, const MixinExpansionKey& r );
inline bool operator!=( const MixinExpansionKey& l, const MixinExpansionKey& r ) { return !( l == r ); }

struct MixinExpansionKeyHasher
{
	size_t operator()( const MixinExpansionKey& k ) const { return k.Hash(); }
};

struct MixinExpansionResult
{
	// TODO - store different kind of expansion elements for mixins in different contexts.
	Synt::ProgramElementsList program_elements;
};

} // namespace U
