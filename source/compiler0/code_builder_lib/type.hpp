#pragma once
#include <set>
#include <variant>

#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/DerivedTypes.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../../code_builder_lib_common/small_types.hpp"
#include "lang_types.hpp"


namespace U
{

enum class ValueType : uint8_t
{
	Value,
	ReferenceMut,
	ReferenceImut,
};

struct FunctionType;
struct FunctionPointerType;
struct ArrayType;
struct TupleType;
struct RawPointerType;
class Class;
struct Enum;
class Type;

enum class InnerReferenceKind : uint8_t
{
	Imut, // Type has immutable inner reference
	Mut, // Type has mutable inner reference
};

enum class SecondOrderInnerReferenceKind : uint8_t
{
	None, // No second order reference.
	Imut,
	Mut,
};

struct InnerReference
{
	InnerReferenceKind kind;
	SecondOrderInnerReferenceKind second_order_kind;

	InnerReference()= delete;

	explicit InnerReference(
		const InnerReferenceKind in_kind,
		const SecondOrderInnerReferenceKind in_second_order_kind= SecondOrderInnerReferenceKind::None )
		: kind(in_kind), second_order_kind(in_second_order_kind)
	{}
};

// Observer pointer to class.
// Class itself stored in class table.
using ClassPtr= Class*;

// Observer pointer to enum.
// Enum itself stored in enum table.
using EnumPtr= Enum*;

struct FundamentalType
{
	U_FundamentalType fundamental_type;
	llvm::Type* llvm_type;

	FundamentalType( U_FundamentalType fundamental_type= U_FundamentalType::void_, llvm::Type* llvm_type= nullptr );
};

bool operator==( const FundamentalType& l, const FundamentalType& r );
inline bool operator!=( const FundamentalType& l, const FundamentalType& r ) { return !( l == r ); }

struct TupleType
{
	std::vector<Type> element_types;
	llvm::StructType* llvm_type= nullptr;
};

bool operator==( const TupleType& l, const TupleType& r );
inline bool operator!=( const TupleType& l, const TupleType& r ) { return !( l == r ); }

class Type
{
public:
	Type()= default;
	Type( const Type& other )= default;
	Type( Type&& )= default;

	Type& operator=( const Type& other )= default;
	Type& operator=( Type&& )= default;

	// Construct from different type kinds.
	Type( FundamentalType fundamental_type );
	Type( FunctionPointerType function_pointer_type );
	Type( ArrayType array_type );
	Type( RawPointerType raw_pointer_type );
	Type( TupleType tuple_type );
	Type( ClassPtr class_type );
	Type( EnumPtr enum_type );

	// Get different type kinds.
	const FundamentalType* GetFundamentalType() const;
	const FunctionPointerType* GetFunctionPointerType() const;
	const ArrayType* GetArrayType() const;
	const RawPointerType* GetRawPointerType() const;
	const TupleType* GetTupleType() const;
	ClassPtr GetClassType() const;
	EnumPtr GetEnumType() const;

	bool ReferenceIsConvertibleTo( const Type& other ) const;

	bool IsDefaultConstructible() const;
	bool IsCopyConstructible() const;
	bool IsCopyAssignable() const;
	bool IsEqualityComparable() const;
	bool HasDestructor() const;
	bool CanBeConstexpr() const;
	bool IsAbstract() const;
	bool IsValidForTemplateVariableArgument() const;

	size_t ReferenceTagCount() const;
	InnerReferenceKind GetInnerReferenceKind(size_t index) const;
	SecondOrderInnerReferenceKind GetSecondOrderInnerReferenceKind( size_t index ) const;
	size_t GetReferenceIndirectionDepth() const;
	bool ContainsMutableReferences() const;

	llvm::Type* GetLLVMType() const;

	// Convert type name to human-readable format.
	std::string ToString() const;

	size_t Hash() const;

private:
	friend bool operator==( const Type&, const Type&);

	using FunctionPointerPtr= std::shared_ptr<const FunctionPointerType>;
	using ArrayPtr= std::shared_ptr<const ArrayType>;
	using RawPointerPtr= std::shared_ptr<const RawPointerType>;
	using TupleTypePtr= std::shared_ptr<const TupleType>;

	using Variant= std::variant<
		FundamentalType,
		ArrayPtr,
		RawPointerPtr,
		ClassPtr,
		EnumPtr,
		FunctionPointerPtr,
		TupleTypePtr >;

private:
	Variant something_;
};

bool operator==( const Type& l, const Type& r );
inline bool operator!=( const Type& l, const Type& r ) { return !( l == r ); }

struct ArrayType
{
	Type element_type;
	uint64_t element_count= 0u;

	llvm::ArrayType* llvm_type= nullptr;
};

bool operator==( const ArrayType& r, const ArrayType& l );
inline bool operator!=( const ArrayType& l, const ArrayType& r ) { return !( l == r ); }

struct RawPointerType
{
	Type element_type;
	llvm::PointerType* llvm_type= nullptr;
};

bool operator==( const RawPointerType& r, const RawPointerType& l );
inline bool operator!=( const RawPointerType& l, const RawPointerType& r ) { return !( l == r ); }

struct FunctionType
{
public:
	struct Param
	{
		Type type;
		ValueType value_type= ValueType::Value;
	};

	// "first" - arg number, "second" is inner tag number or ~0, if it is reference itself
	static constexpr uint8_t c_param_reference_number= 255u;
	using ParamReference= std::pair< uint8_t, uint8_t >;

	using ReturnReferences= std::set<ParamReference>;
	using ReturnInnerReferences= std::vector<ReturnReferences>;

	struct ReferencePollution
	{
		ParamReference dst;
		ParamReference src; // second = ~0, if reference itself, else - inner reference.
		bool operator==( const ReferencePollution& other ) const;
		bool operator<( const ReferencePollution& other ) const;
	};

	using ReferencesPollution= std::set<ReferencePollution>;

	bool PointerCanBeConvertedTo( const FunctionType& other ) const;
	bool ReturnsCompositeValue() const;

public:
	// If this changed, virtual functions compare function must be changed too!
	ArgsVector<Param> params;
	Type return_type;
	ValueType return_value_type= ValueType::Value;
	bool unsafe= false;
	llvm::CallingConv::ID calling_convention= llvm::CallingConv::C;

	// Use "std::set" for references description, because we needs stable order for function type mangling.

	// Tags of returned reference.
	ReturnReferences return_references;

	// Tags for each inner reference node for returned value/reference.
	ReturnInnerReferences return_inner_references;

	ReferencesPollution references_pollution;

	// Do not store llvm type here, because calculating exact llvm type requires complete types of arguments and return value.
};

bool operator==( const FunctionType::Param& l, const FunctionType::Param& r );
inline bool operator!=( const FunctionType::Param& l, const FunctionType::Param& r ) { return !( l == r ); }

bool operator==( const FunctionType& l, const FunctionType& r );
inline bool operator!=( const FunctionType& l, const FunctionType& r ) { return !( l == r ); }

std::string FunctionParamsToString( llvm::ArrayRef<FunctionType::Param> params );

struct FunctionPointerType
{
	FunctionType function_type;
	llvm::PointerType* llvm_type= nullptr;
};

bool operator==( const FunctionPointerType& l, const FunctionPointerType& r );
inline bool operator!=( const FunctionPointerType& l, const FunctionPointerType& r ) { return !( l == r ); }

struct TypeHasher
{
	size_t operator()(const Type& t) const { return t.Hash(); }
};

} // namespace U
