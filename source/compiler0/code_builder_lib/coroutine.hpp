#pragma once
#include "type.hpp"

namespace U
{

enum class CoroutineKind : uint8_t
{
	Generator,
};

struct CoroutineTypeDescription
{
public:
	// If this changed - mangling must be changed too!
	CoroutineKind kind= CoroutineKind::Generator;
	Type return_type;
	ValueType return_value_type= ValueType::Value;

	std::optional<InnerReferenceType> inner_reference_type;
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
