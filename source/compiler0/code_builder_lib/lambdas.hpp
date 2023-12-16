#pragma once
#include "names_scope.hpp"

namespace U
{

struct LambdaKey
{
	// Non-null if this is a lambda inside some template context.
	NamesScopePtr template_args_namespace;
	// SrcLoc of lambda syntax element.
	SrcLoc src_loc;

	size_t Hash() const;
};

bool operator==( const LambdaKey& l, const LambdaKey& r );
inline bool operator!=( const LambdaKey& l, const LambdaKey& r ) { return !( l == r ); }

struct LambdaKeyHasher
{
	size_t operator()( const LambdaKey& k ) const { return k.Hash(); }
};

struct LambdaClassData
{
};

} // namespace U
