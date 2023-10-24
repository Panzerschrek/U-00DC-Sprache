#include "coroutine.hpp"

namespace U
{

size_t CoroutineTypeDescription::Hash() const
{
	auto hash=
		llvm::hash_combine(
			kind,
			return_type.Hash(),
			return_value_type,
			non_sync );

	for( const FunctionType::ParamReference& param_reference : return_references )
		hash= llvm::hash_combine( hash, param_reference );

	for( const auto& tags_set : return_inner_references )
		for( const FunctionType::ParamReference& param_reference : tags_set )
			hash= llvm::hash_combine( hash, param_reference );

	for( const InnerReferenceType k : inner_references )
		hash= llvm::hash_combine( hash, k );

	return hash;
}

bool operator==( const CoroutineTypeDescription& l, const CoroutineTypeDescription& r )
{
	return
		l.kind == r.kind &&
		l.return_type == r.return_type &&
		l.return_value_type == r.return_value_type &&
		l.return_references == r.return_references &&
		l.return_inner_references == r.return_inner_references &&
		l.inner_references == r.inner_references &&
		l.non_sync == r.non_sync;
}

} // namespace U
