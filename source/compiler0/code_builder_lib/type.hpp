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

enum class InnerReferenceType : uint8_t
{
	None, // Type have no innere references
	Imut, // Type have immutable inner ereference
	Mut, // Type have mutable inner reference
};

// Observer pointer to class.
// Class itself stored in class table.
using ClassPtr= Class*;

// Observer pointer to enum.
// Enum itself stored in enum table.
using EnumPtr= Enum*;

struct FundamentalType final
{
	U_FundamentalType fundamental_type;
	llvm::Type* llvm_type;

	FundamentalType( U_FundamentalType fundamental_type= U_FundamentalType::void_, llvm::Type* llvm_type= nullptr );
	uint64_t GetSize() const;
};

bool operator==( const FundamentalType& l, const FundamentalType& r );
bool operator!=( const FundamentalType& l, const FundamentalType& r );

struct TupleType final
{
	std::vector<Type> element_types;
	llvm::StructType* llvm_type= nullptr;
};

bool operator==( const TupleType& l, const TupleType& r );
bool operator!=( const TupleType& l, const TupleType& r );

class Type final
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
	bool HaveDestructor() const;
	bool CanBeConstexpr() const;
	bool IsAbstract() const;

	size_t ReferencesTagsCount() const;
	InnerReferenceType GetInnerReferenceType(size_t index) const;

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
bool operator!=( const Type& l, const Type& r );

struct ArrayType final
{
	Type element_type;
	uint64_t element_count= 0u;

	llvm::ArrayType* llvm_type= nullptr;
};

bool operator==( const ArrayType& r, const ArrayType& l );
bool operator!=( const ArrayType& r, const ArrayType& l );

struct RawPointerType final
{
	Type element_type;
	llvm::PointerType* llvm_type= nullptr;
};

bool operator==( const RawPointerType& r, const RawPointerType& l );
bool operator!=( const RawPointerType& r, const RawPointerType& l );

struct FunctionType final
{
public:
	struct Param
	{
		Type type;
		ValueType value_type= ValueType::Value;
	};

	// "first" - arg number, "second" is inner tag number or ~0, if it is reference itself
	static constexpr uint8_t c_arg_reference_tag_number= 255u;
	using ParamReference= std::pair< uint8_t, uint8_t >;

	struct ReferencePollution
	{
		ParamReference dst;
		ParamReference src; // second = ~0, if reference itself, else - inner reference.
		bool operator==( const ReferencePollution& other ) const;
		bool operator<( const ReferencePollution& other ) const;
	};

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
	std::set<ParamReference> return_references;

	// Tags for each inner reference node for returned value/reference.
	std::vector<std::set<ParamReference>> return_inner_references;

	std::set<ReferencePollution> references_pollution;

	// Do not store llvm type here, because calculating exact llvm type requires complete types of arguments and return value.
};

bool operator==( const FunctionType::Param& l, const FunctionType::Param& r );
bool operator!=( const FunctionType::Param& l, const FunctionType::Param& r );
bool operator==( const FunctionType& l, const FunctionType& r );
bool operator!=( const FunctionType& l, const FunctionType& r );

std::string FunctionParamsToString( llvm::ArrayRef<FunctionType::Param> params );

struct FunctionPointerType
{
	FunctionType function_type;
	llvm::PointerType* llvm_type= nullptr;
};

bool operator==( const FunctionPointerType& l, const FunctionPointerType& r );
bool operator!=( const FunctionPointerType& l, const FunctionPointerType& r );

struct TypeHasher
{
	size_t operator()(const Type& t) const { return t.Hash(); }
};

} // namespace U
