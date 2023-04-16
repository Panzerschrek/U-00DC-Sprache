#include "coroutine.hpp"

namespace U
{

size_t CoroutineTypeDescription::Hash() const
{
	return
		llvm::hash_combine(
			kind,
			return_type.Hash(),
			return_value_type,
			inner_reference_type,
			non_sync );
}

bool operator==( const CoroutineTypeDescription& l, const CoroutineTypeDescription& r )
{
	return
		l.kind == r.kind &&
		l.return_type == r.return_type &&
		l.return_value_type == r.return_value_type &&
		l.inner_reference_type == r.inner_reference_type &&
		l.non_sync == r.non_sync;
}

} // namespace U
