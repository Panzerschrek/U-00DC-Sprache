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

	InnerReferenceType inner_reference_type= InnerReferenceType::None;

public:
	size_t Hash() const;
};

bool operator==( const CoroutineTypeDescription& l, const CoroutineTypeDescription& r );

struct CoroutineTypeDescriptionHasher
{
	size_t operator()(const CoroutineTypeDescription& d) const { return d.Hash(); }
};

} // namespace U
