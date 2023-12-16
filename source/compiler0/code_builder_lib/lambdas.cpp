#include "lambdas.hpp"

namespace U
{

size_t LambdaKey::Hash() const
{
	return llvm::hash_combine(
		size_t( reinterpret_cast<uintptr_t>( template_args_namespace.get() ) ),
		src_loc.Hash() );
}

bool operator==( const LambdaKey& l, const LambdaKey& r )
{
	return
		l.template_args_namespace == r.template_args_namespace &&
		l.src_loc == r.src_loc;
}

} // namespace U
