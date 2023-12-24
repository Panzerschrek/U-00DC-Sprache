#include "lambdas.hpp"

namespace U
{

size_t LambdaKey::Hash() const
{
	return llvm::hash_combine( parent_scope, src_loc.Hash() );
}

bool operator==( const LambdaKey& l, const LambdaKey& r )
{
	return
		l.parent_scope == r.parent_scope &&
		l.src_loc == r.src_loc;
}

} // namespace U
