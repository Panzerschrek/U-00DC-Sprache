#pragma once
#include <optional>
#include "../lex_synt_lib/syntax_elements.hpp"
#include "type.hpp"

namespace U
{

using CoroutineKind= Synt::CoroutineType::Kind;

struct CoroutineTypeDescription
{
public:
	// Specify return value (type, value type, reference notation) for coroutines,
	// since they have de-facto callable method "resume".

	// If this changed - mangling must be changed too!
	CoroutineKind kind= CoroutineKind::Generator;
	Type return_type;
	ValueType return_value_type= ValueType::Value;

	// Tags of returned reference.
	FunctionType::ReturnReferences return_references;

	// Tags for each inner reference node for returned value/reference.
	FunctionType::ReturnInnerReferences return_inner_references;

	// Inner references of coroutine type itself.
	llvm::SmallVector<InnerReferenceKind, 4> inner_references;

	bool non_sync= false;

public:
	size_t Hash() const;
};

bool operator==( const CoroutineTypeDescription& l, const CoroutineTypeDescription& r );

struct CoroutineTypeDescriptionHasher
{
	size_t operator()(const CoroutineTypeDescription& d) const { return d.Hash(); }
};

} // namespace U
