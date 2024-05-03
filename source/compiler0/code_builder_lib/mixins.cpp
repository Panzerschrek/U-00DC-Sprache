#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/ADT/Hashing.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "mixins.hpp"

namespace U
{

size_t MixinExpansionKey::Hash() const
{
	return llvm::hash_combine( mixin_src_loc.Hash(), mixin_text );
}

bool operator==( const MixinExpansionKey& l, const MixinExpansionKey& r )
{
	return
		l.mixin_src_loc == r.mixin_src_loc &&
		l.mixin_text == r.mixin_text;
}

} // namespace U
