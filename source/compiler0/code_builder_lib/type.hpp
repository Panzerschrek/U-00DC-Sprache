#pragma once
#include <set>
#include <variant>

#include "../../compilers_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/DerivedTypes.h>
#include "../../compilers_common/pop_llvm_warnings.hpp"

#include "../../compilers_common/small_types.hpp"
#include "lang_types.hpp"


namespace U
{

namespace CodeBuilderPrivate
{

struct Function;
struct FunctionPointer;
struct Array;
struct Tuple;
class Class;
struct Enum;
class Type;

enum class InnerReferenceType
{
	None, // Type have no innere references
	Imut, // Type have immutable inner ereference
	Mut, // Type have mutable inner reference
};

struct ClassProxy
{
	// Observer pointer to class.
	// Class itself stored in class table.
	Class* class_= nullptr;
};
using ClassProxyPtr= std::shared_ptr<ClassProxy>;
using ClassProxyWeakPtr= std::weak_ptr<ClassProxy>;

// Observer pointer to class.
// Enum itself stored in class table.
using EnumPtr= Enum*;

enum class TypeCompleteness
{
	Incomplete, // Known nothing
	ReferenceTagsComplete, // Known fields, parents.
	Complete, // Known also member functions
};

struct FundamentalType final
{
	U_FundamentalType fundamental_type;
	llvm::Type* llvm_type;

	FundamentalType( U_FundamentalType fundamental_type= U_FundamentalType::Void, llvm::Type* llvm_type= nullptr );
	uint64_t GetSize() const;
};

bool operator==( const FundamentalType& l, const FundamentalType& r );
bool operator!=( const FundamentalType& l, const FundamentalType& r );

struct Tuple final
{
	std::vector<Type> elements;
	llvm::StructType* llvm_type= nullptr;
};

bool operator==( const Tuple& l, const Tuple& r );
bool operator!=( const Tuple& l, const Tuple& r );

class Type final
{
public:
	Type()= default;
	Type( const Type& other );
	Type( Type&& )= default;

	Type& operator=( const Type& other );
	Type& operator=( Type&& )= default;

	// Construct from different type kinds.
	Type( FundamentalType fundamental_type );
	Type( const Function& function_type );
	Type( const FunctionPointer& function_pointer_type );
	Type( Function&& function_type );
	Type( const Array& array_type );
	Type( Array&& array_type );
	Type( const Tuple& tuple_type );
	Type( Tuple&& tuple_type );
	Type( ClassProxyPtr class_type );
	Type( EnumPtr enum_type );

	// Get different type kinds.
	FundamentalType* GetFundamentalType();
	const FundamentalType* GetFundamentalType() const;
	Function* GetFunctionType();
	const Function* GetFunctionType() const;
	FunctionPointer* GetFunctionPointerType();
	const FunctionPointer* GetFunctionPointerType() const;
	Array* GetArrayType();
	const Array* GetArrayType() const;
	Tuple* GetTupleType();
	const Tuple* GetTupleType() const;
	ClassProxyPtr GetClassTypeProxy() const;
	Class* GetClassType() const;
	Enum* GetEnumType() const;
	EnumPtr GetEnumTypePtr() const;

	bool ReferenceIsConvertibleTo( const Type& other ) const;

	bool IsDefaultConstructible() const;
	bool IsCopyConstructible() const;
	bool IsCopyAssignable() const;
	bool HaveDestructor() const;
	bool CanBeConstexpr() const;
	bool IsAbstract() const;

	size_t ReferencesTagsCount() const;
	InnerReferenceType GetInnerReferenceType() const;

	llvm::Type* GetLLVMType() const;

	// Convert type name to human-readable format.
	std::string ToString() const;

	size_t Hash() const;

private:
	friend bool operator==( const Type&, const Type&);

	using FunctionPtr= std::unique_ptr<Function>;
	using FunctionPointerPtr= std::unique_ptr<FunctionPointer>;
	using ArrayPtr= std::unique_ptr<Array>;

	std::variant<
		FundamentalType,
		FunctionPtr,
		ArrayPtr,
		ClassProxyPtr,
		EnumPtr,
		FunctionPointerPtr,
		Tuple > something_;
};

bool operator==( const Type& l, const Type& r );
bool operator!=( const Type& l, const Type& r );

struct Array final
{
	Type type;
	uint64_t size= 0u;

	llvm::ArrayType* llvm_type= nullptr;
};

bool operator==( const Array& r, const Array& l );
bool operator!=( const Array& r, const Array& l );

struct Function final
{
public:
	struct Arg
	{
		Type type;
		bool is_reference;
		bool is_mutable;
	};

	// "first" - arg number, "second" is inner tag number or ~0, if it is reference itself
	static constexpr size_t c_arg_reference_tag_number= ~0u;
	using ArgReference= std::pair< size_t, size_t >;

	struct ReferencePollution
	{
		ArgReference dst;
		ArgReference src; // second = ~0, if reference itself, else - inner reference.
		bool operator==( const ReferencePollution& other ) const;
		bool operator<( const ReferencePollution& other ) const;
	};

	bool PointerCanBeConvertedTo( const Function& other ) const;

public:
	// If this changed, virtual functions compare function must be changed too!
	ArgsVector<Arg> args;
	Type return_type;
	bool return_value_is_reference= false;
	bool return_value_is_mutable= false;
	bool unsafe= false;

	// Use "std::set" for references description, because we needs stable order for function type mangling.

	// for functions, returning references this is references of reference itslef.
	// For function, returning values, this is inner references.
	std::set<ArgReference> return_references;
	std::set<ReferencePollution> references_pollution;

	llvm::FunctionType* llvm_function_type= nullptr;
};

bool operator==( const Function::Arg& l, const Function::Arg& r );
bool operator!=( const Function::Arg& l, const Function::Arg& r );
bool operator==( const Function& l, const Function& r );
bool operator!=( const Function& l, const Function& r );

struct FunctionPointer
{
	Function function;
	llvm::PointerType* llvm_function_pointer_type= nullptr;
};

bool operator==( const FunctionPointer& l, const FunctionPointer& r );
bool operator!=( const FunctionPointer& l, const FunctionPointer& r );

struct TypeHasher
{
	size_t operator()(const Type& t) const { return t.Hash(); }
};

} //namespace CodeBuilderLLVMPrivate

} // namespace U
