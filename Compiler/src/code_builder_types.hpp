#pragma once
#include <memory>
#include <vector>
#include <map>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include "pop_llvm_warnings.hpp"

#include "lang_types.hpp"
#include "program_string.hpp"
#include "syntax_elements.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

struct Function;
struct Array;

struct Class;
typedef std::shared_ptr<Class> ClassPtr;
typedef std::weak_ptr<Class> ClassWeakPtr;

class NamesScope;
typedef std::shared_ptr<NamesScope> NamesScopePtr;

struct ClassTemplate;
typedef std::shared_ptr<ClassTemplate> ClassTemplatePtr;

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
	TypeName,
	Namespace,
	ClassTemplate,
};

bool operator==( const FundamentalType& r, const FundamentalType& l );
bool operator!=( const FundamentalType& r, const FundamentalType& l );

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
	Type( Function&& function_type );
	Type( const Array& array_type );
	Type( Array&& array_type );
	Type( ClassPtr class_type );
	Type( NontypeStub nontype_strub );

	// Get different type kinds.
	FundamentalType* GetFundamentalType();
	const FundamentalType* GetFundamentalType() const;
	Function* GetFunctionType();
	const Function* GetFunctionType() const;
	Array* GetArrayType();
	const Array* GetArrayType() const;
	ClassPtr GetClassType() const;

	// TODO - does this method needs?
	size_t SizeOf() const;

	bool IsIncomplete() const;
	bool IsDefaultConstructible() const;
	bool IsCopyConstructible() const;
	bool HaveDestructor() const;
	bool CanBeConstexpr() const;

	llvm::Type* GetLLVMType() const;
	ProgramString ToString() const;

private:
	friend bool operator==( const Type&, const Type&);

	typedef std::unique_ptr<Function> FunctionPtr;
	typedef std::unique_ptr<Array> ArrayPtr;

	boost::variant<
		FundamentalType,
		FunctionPtr,
		ArrayPtr,
		ClassPtr,
		NontypeStub> something_;
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
	Type type; // Function type 100%
	bool have_body= true;
	bool is_this_call= false;
	bool is_generated= false;
	bool return_value_is_sret= false;

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

	// Exists only for constant expressions of fundamental types.
	llvm::Constant* constexpr_value= nullptr;
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
	Value( Type type );
	Value( ClassField class_field );
	Value( ThisOverloadedMethodsSet class_field );
	Value( const NamesScopePtr& namespace_ );
	Value( const ClassTemplatePtr& class_template );

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
	// Typename
	Type* GetTypeName();
	const Type* GetTypeName() const;
	// Class fields
	const ClassField* GetClassField() const;
	// This + methods set
	ThisOverloadedMethodsSet* GetThisOverloadedMethodsSet();
	const ThisOverloadedMethodsSet* GetThisOverloadedMethodsSet() const;
	// Namespace
	NamesScopePtr GetNamespace() const;
	// Class Template
	ClassTemplatePtr GetClassTemplate() const;

private:
	boost::variant<
		Variable,
		FunctionVariable,
		OverloadedFunctionsSet,
		Type,
		ClassField,
		ThisOverloadedMethodsSet,
		NamesScopePtr,
		ClassTemplatePtr> something_;
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

	// Resolve simple name only in this scope.
	InsertedName* GetThisScopeName( const ProgramString& name ) const;

	const NamesScope* GetParent() const;
	const NamesScope* GetRoot() const;
	void SetParent( const NamesScope* parent );

	template<class Func>
	void ForEachInThisScope( const Func& func ) const
	{
		for( const InsertedName& inserted_name : names_map_ )
			func( inserted_name );
	}

	// TODO - maybe add for_each in all scopes?

private:
	const ProgramString name_;
	const NamesScope* parent_;
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

typedef boost::variant< Variable, Type > TemplateParameter;

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
	bool have_explicit_noncopy_constructors= false;
	bool is_default_constructible= false;
	bool is_copy_constructible= false;
	bool have_destructor= false;

	llvm::StructType* llvm_type;

	struct BaseTemplate
	{
		ClassTemplatePtr class_template;
		std::vector<TemplateParameter> template_parameters;
	};

	// Exists only for classes, generated from class templates.
	boost::optional<BaseTemplate> base_template;
};

struct ClassTemplate final
{
	struct TemplateParameter
	{
		ProgramString name;
		const ComplexName* type_name= nullptr; // Exists for value parameters.
	};


	// Sorted in order of first parameter usage in signature.
	std::vector< TemplateParameter > template_parameters;

	std::vector< const ComplexName* > signature_arguments;

	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const ClassDeclaration* class_syntax_element= nullptr;
};

typedef boost::variant< int, Type, Variable > DeducibleTemplateParameter; // int means not deduced
typedef std::vector<DeducibleTemplateParameter> DeducibleTemplateParameters;

const ProgramString& GetFundamentalTypeName( U_FundamentalType fundamental_type );
const char* GetFundamentalTypeNameASCII( U_FundamentalType fundamental_type );

} //namespace CodeBuilderLLVMPrivate

} // namespace U
