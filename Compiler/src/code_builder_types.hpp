#pragma once
#include <memory>
#include <vector>
#include <map>

#include <boost/variant.hpp>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Function.h>
#include "pop_llvm_warnings.hpp"

#include "lang_types.hpp"
#include "program_string.hpp"
#include "syntax_elements.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

struct Function;
typedef std::unique_ptr<Function> FunctionPtr;

struct Array;
typedef std::unique_ptr<Array> ArrayPtr;

struct Class;
typedef std::shared_ptr<Class> ClassPtr;
typedef std::weak_ptr<Class> ClassWeakPtr;

class NamesScope;
typedef std::shared_ptr<NamesScope> NamesScopePtr;

struct FundamentalType final
{
	U_FundamentalType fundamental_type;
	llvm::Type* llvm_type;

	FundamentalType( U_FundamentalType fundamental_type= U_FundamentalType::Void, llvm::Type* llvm_type= nullptr );
};

// Stub for type of non-variable "Variables".
enum class NontypeStub
{
	OverloadedFunctionsSet,
	ThisOverloadedMethodsSet,
	ClassName,
	Namespace,
};

bool operator==( const FundamentalType& r, const FundamentalType& l );
bool operator!=( const FundamentalType& r, const FundamentalType& l );

struct Type final
{
	boost::variant<
		FundamentalType,
		FunctionPtr,
		ArrayPtr,
		ClassPtr,
		NontypeStub> one_of_type_kind;

	Type()= default;
	Type( const Type& other );
	Type( Type&& )= default;

	Type& operator=( const Type& other );
	Type& operator=( Type&& )= default;

	// TODO - does this method needs?
	size_t SizeOf() const;

	bool IsIncomplete() const;

	llvm::Type* GetLLVMType() const;
	ProgramString ToString() const;
};

bool operator==( const Type& r, const Type& l );
bool operator!=( const Type& r, const Type& l );

struct Function final
{
	struct Arg
	{
		Type type;
		bool is_reference;
		bool is_mutable;
	};

	Type return_type;
	bool return_value_is_reference= false;
	bool return_value_is_mutable= false;
	std::vector<Arg> args;

	llvm::FunctionType* llvm_function_type= nullptr;
};

bool operator==( const Function::Arg& r, const Function::Arg& l );
bool operator!=( const Function::Arg& r, const Function::Arg& l );
bool operator==( const Function& r, const Function& l );
bool operator!=( const Function& r, const Function& l );

struct Array final
{
	Type type;
	size_t size= 0u;

	llvm::ArrayType* llvm_type= nullptr;
};

bool operator==( const Array& r, const Array& l );
bool operator!=( const Array& r, const Array& l );

struct FunctionVariable final
{
	Type type;
	bool have_body= true;
	bool is_this_call= false;

	llvm::Function* llvm_function= nullptr;
};

// Set of functions with same name, but different signature.
typedef std::vector<FunctionVariable> OverloadedFunctionsSet;

enum class ValueType
{
	Value,
	Reference,
	ConstReference,
};

struct Variable final
{
	enum class Location
	{
		Pointer,
		LLVMRegister,
	};

	Type type;
	Location location= Location::Pointer;
	ValueType value_type= ValueType::ConstReference;
	llvm::Value* llvm_value= nullptr;
};

struct ClassField final
{
	Type type;
	unsigned int index= 0u;
	ClassWeakPtr class_;
};

// "this" + functions set of class of "this"
struct ThisOverloadedMethodsSet final
{
	Variable this_;
	OverloadedFunctionsSet overloaded_methods_set;
};

class Value final
{
public:
	Value();
	Value( Variable variable );
	Value( FunctionVariable function_variable );
	Value( OverloadedFunctionsSet functions_set );
	Value( const ClassPtr& class_ );
	Value( ClassField class_field );
	Value( ThisOverloadedMethodsSet class_field );
	Value( const NamesScopePtr& namespace_ );

	const Type& GetType() const;

	// Fundamental, class, array types
	Variable* GetVariable();
	const Variable* GetVariable() const;
	// Function types
	FunctionVariable* GetFunctionVariable();
	const FunctionVariable* GetFunctionVariable() const;
	// Function set stub type
	OverloadedFunctionsSet* GetFunctionsSet();
	const OverloadedFunctionsSet* GetFunctionsSet() const;
	// Class stub type
	ClassPtr GetClass() const;
	// Class fields
	const ClassField* GetClassField() const;
	// This + methods set
	ThisOverloadedMethodsSet* GetThisOverloadedMethodsSet();
	const ThisOverloadedMethodsSet* GetThisOverloadedMethodsSet() const;
	// Namespace
	NamesScopePtr GetNamespace() const;

private:
	struct OverloadedFunctionsSetWithTypeStub
	{
		OverloadedFunctionsSetWithTypeStub();

		Type type;
		OverloadedFunctionsSet set;
	};
	struct ThisOverloadedMethodsSetWithTypeStub
	{
		ThisOverloadedMethodsSetWithTypeStub();

		Type type;
		ThisOverloadedMethodsSet set;
	};
	struct ClassWithTypeStub
	{
		ClassWithTypeStub();

		Type type;
		ClassPtr class_;
	};
	struct NamespaceWithTypeStub
	{
		 NamespaceWithTypeStub();

		Type type;
		NamesScopePtr namespace_;
	};

private:
	boost::variant<
		Variable,
		FunctionVariable,
		OverloadedFunctionsSetWithTypeStub,
		ClassWithTypeStub,
		ClassField,
		ThisOverloadedMethodsSetWithTypeStub,
		NamespaceWithTypeStub > something_;
};

// "Class" of function argument in terms of overloading.
enum class ArgOverloadingClass
{
	// Value-args (both mutable and immutable), immutable references.
	ImmutableReference,
	// Mutable references.
	MutalbeReference,
	// SPRACHE_TODO - add class for move-references here
};

ArgOverloadingClass GetArgOverloadingClass( bool is_reference, bool is_mutable );
ArgOverloadingClass GetArgOverloadingClass( ValueType value_type );
ArgOverloadingClass GetArgOverloadingClass( const Function::Arg& arg );

class NamesScope final
{
public:

	typedef std::map< ProgramString, Value > NamesMap;
	typedef NamesMap::value_type InsertedName;

	NamesScope(
		ProgramString name,
		const NamesScope* parent );

	NamesScope( const NamesScope&)= delete;
	NamesScope& operator=( const NamesScope&)= delete;

	const ProgramString& GetThisNamespaceName() const;

	// Returns nullptr, if name already exists in this scope.
	InsertedName* AddName( const ProgramString& name, Value value );

	// Performs full name resolving.
	InsertedName* ResolveName( const ComplexName& name ) const;
	// Resolve simple name only in this scope.
	InsertedName* GetThisScopeName( const ProgramString& name ) const;

	template<class Func>
	void ForEachInThisScope( const Func& func ) const
	{
		for( const InsertedName& inserted_name : names_map_ )
			func( inserted_name );
	}

	ProgramString GetFunctionMangledName( const ProgramString& func_name ) const;

	// TODO - maybe add for_each in all scopes?

private:
	void GetNamespacePrefix_r( ProgramString& out_name ) const;
	InsertedName* ResolveName_r( const ProgramString* components, size_t component_count ) const;

private:
	const ProgramString name_;
	const NamesScope* const parent_;
	NamesMap names_map_;
};

class ProgramError final : public std::exception
{
public:
	virtual ~ProgramError() override{}

	virtual const char* what() const noexcept override
	{
		return "ProgramError";
	}
};

struct Class final
{
	Class( const ProgramString& name, const NamesScope* parent_scope );
	~Class();

	Class( const Class& )= delete;
	Class( Class&& )= delete;

	Class& operator=( const Class& )= delete;
	Class& operator=( Class&& )= delete;

	NamesScope members;
	size_t field_count= 0u;
	bool is_incomplete= true;

	llvm::StructType* llvm_type;
};

const ProgramString& GetFundamentalTypeName( U_FundamentalType fundamental_type );
const char* GetFundamentalTypeNameASCII( U_FundamentalType fundamental_type );

} //namespace CodeBuilderLLVMPrivate

} // namespace U
