#pragma once

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Constants.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"

namespace U
{

struct Mixin
{
	SrcLoc src_loc;
	Synt::ClassMemberVisibility visibility= Synt::ClassMemberVisibility::Public;
	const Synt::Mixin* syntax_element= nullptr; // Zeroed after mixin expression evaluation.

	// If non-null - it's proper constexpr string. Null if mixin is already expanded.
	// Also is null if this mixin requires no expansion (if it is empty, for example).
	llvm::ConstantDataArray* string_constant= nullptr;
};

using Mixins= std::vector<Mixin>;

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

} // namespace U
