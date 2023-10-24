#include "coroutine.hpp"

namespace U
{

size_t CoroutineTypeDescription::Hash() const
{
	auto result=
		llvm::hash_combine(
			kind,
			return_type.Hash(),
			return_value_type,
			non_sync );

	for( const InnerReferenceType k : inner_references )
		result= llvm::hash_combine( result, k );

	return result;
}

bool operator==( const CoroutineTypeDescription& l, const CoroutineTypeDescription& r )
{
	return
		l.kind == r.kind &&
		l.return_type == r.return_type &&
		l.return_value_type == r.return_value_type &&
		l.inner_references == r.inner_references &&
		l.non_sync == r.non_sync;
}

} // namespace U
