#include "lambdas.hpp"

namespace U
{

size_t LambdaKey::Hash() const
{
	size_t result= llvm::hash_combine( parent_scope, src_loc.Hash() );

	for( const uint32_t index : tuple_for_indices )
		result= llvm::hash_combine( result, index );

	return result;
}

bool operator==( const LambdaKey& l, const LambdaKey& r )
{
	return
		l.parent_scope == r.parent_scope &&
		l.src_loc == r.src_loc &&
		l.tuple_for_indices == r.tuple_for_indices;
}

} // namespace U
