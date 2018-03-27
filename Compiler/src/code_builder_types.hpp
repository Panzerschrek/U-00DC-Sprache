#pragma once
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
class Class;
struct Enum;

struct ClassProxy
{
	ClassProxy( Class* in_class ) // Poiter must be new-allocated. Takes ownership.
		: class_( in_class )
	{}
	std::shared_ptr<Class> class_;
};
typedef std::shared_ptr<ClassProxy> ClassProxyPtr;
typedef std::weak_ptr<ClassProxy> ClassProxyWeakPtr;

typedef std::shared_ptr<Enum> EnumPtr;

class NamesScope;
typedef std::shared_ptr<NamesScope> NamesScopePtr;

struct TypeTemplate;
typedef std::shared_ptr<TypeTemplate> TypeTemplatePtr;

struct FundamentalType final
{
	U_FundamentalType fundamental_type;
	llvm::Type* llvm_type;

	FundamentalType( U_FundamentalType fundamental_type= U_FundamentalType::Void, llvm::Type* llvm_type= nullptr );
};

struct TemplateDependentType
{
	size_t index;
	llvm::Type* llvm_type;

	TemplateDependentType( size_t in_index, llvm::Type* in_llvm_type );
};

// Stub for type of non-variable "Values".
enum class NontypeStub
{
	OverloadedFunctionsSet,
	ThisOverloadedMethodsSet,
	TypeName,
	Namespace,
	TypeTemplate,
	TemplateDependentValue,
	YetNotDeducedTemplateArg,
	ErrorValue,
	VariableStorage,
};

bool operator==( const FundamentalType& r, const FundamentalType& l );
bool operator!=( const FundamentalType& r, const FundamentalType& l );

bool operator==( const TemplateDependentType& r, const TemplateDependentType& l );
bool operator!=( const TemplateDependentType& r, const TemplateDependentType& l );

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
	Type( ClassProxyPtr class_type );
	Type( EnumPtr enum_type );
	Type( NontypeStub nontype_strub );
	Type( TemplateDependentType template_dependent_type );

	// Get different type kinds.
	FundamentalType* GetFundamentalType();
	const FundamentalType* GetFundamentalType() const;
	Function* GetFunctionType();
	const Function* GetFunctionType() const;
	Array* GetArrayType();
	const Array* GetArrayType() const;
	ClassProxyPtr GetClassTypeProxy() const;
	Class* GetClassType() const;
	Enum* GetEnumType() const;
	TemplateDependentType* GetTemplateDependentType();
	const TemplateDependentType* GetTemplateDependentType() const;

	// TODO - does this method needs?
	SizeType SizeOf() const;

	bool IsIncomplete() const;
	bool IsDefaultConstructible() const;
	bool IsCopyConstructible() const;
	bool IsCopyAssignable() const;
	bool HaveDestructor() const;
	bool CanBeConstexpr() const;
	size_t ReferencesTagsCount() const;

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
		ClassProxyPtr,
		EnumPtr,
		NontypeStub,
		TemplateDependentType> something_;
};

bool operator==( const Type& r, const Type& l );
bool operator!=( const Type& r, const Type& l );

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
	typedef std::pair< size_t, size_t > ArgReference;

	struct InToOutReferences
	{
		// Numers of args.
		std::vector<size_t> args_references;
		// Pairs of arg number and tag number.
		std::vector< ArgReference > inner_args_references;
	};

	struct ReferencePollution
	{
		ArgReference dst;
		ArgReference src; // second = ~0, if reference itself, else - inner reference.
		bool src_is_mutable= true;
		bool operator==( const ReferencePollution& other ) const;
	};

	struct ReferencePollutionHasher
	{
		size_t operator()( const ReferencePollution& r ) const;
	};

public:
	Type return_type;
	bool return_value_is_reference= false;
	bool return_value_is_mutable= false;
	std::vector<Arg> args;

	// for functions, returning references this is references of reference itslef.
	// For function, returning values, this is inner references.
	InToOutReferences return_references;
	std::unordered_set< ReferencePollution, ReferencePollutionHasher > references_pollution;

	llvm::FunctionType* llvm_function_type= nullptr;
};

bool operator==( const Function::InToOutReferences& l, const Function::InToOutReferences& r );
bool operator!=( const Function::InToOutReferences& l, const Function::InToOutReferences& r );
bool operator==( const Function::Arg& r, const Function::Arg& l );
bool operator!=( const Function::Arg& r, const Function::Arg& l );
bool operator==( const Function& r, const Function& l );
bool operator!=( const Function& r, const Function& l );

struct Array final
{
	// "size" in case, when size is not known yet, when size depends on template parameter, for example.
	static constexpr SizeType c_undefined_size= std::numeric_limits<SizeType>::max();

	Type type;
	SizeType size= c_undefined_size;

	llvm::ArrayType* llvm_type= nullptr;

	SizeType ArraySizeOrZero() const { return size == c_undefined_size ? 0u : size; }
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

	FilePos prototype_file_pos= FilePos{ 0u, 0u, 0u };
	FilePos body_file_pos= FilePos{ 0u, 0u, 0u };
};

// Set of functions with same name, but different signature.
typedef std::vector<FunctionVariable> OverloadedFunctionsSet;

class StoredVariable;
typedef std::shared_ptr<StoredVariable> StoredVariablePtr;
typedef std::shared_ptr<void> VariableStorageUseCounter;

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
	// Undef, if value is template-dependent.
	llvm::Constant* constexpr_value= nullptr;

	std::unordered_set<StoredVariablePtr> referenced_variables;
};

class StoredVariable
{
public:
	struct ReferencedVariable
	{
		StoredVariablePtr variable;
		VariableStorageUseCounter use_counter;
		bool IsMutable() const{ return use_counter == variable->mut_use_counter; }
	};

	enum class Kind
	{
		Variable,
		Reference,
		ArgInnerVariable,
	};

	const ProgramString name; // needs for error messages
	Variable content;
	const VariableStorageUseCounter  mut_use_counter= std::make_shared<int>();
	const VariableStorageUseCounter imut_use_counter= std::make_shared<int>();

	const Kind kind;
	const bool is_global_constant;

	StoredVariable( ProgramString iname, Variable icontent, Kind ikind= Kind::Variable, bool iis_global_constant= false );
};

class VariablesState
{
public:
	struct Reference
	{
		VariableStorageUseCounter use_counter;
		bool is_mutable= true;
		bool is_arg_inner_variable= false;
		bool IsMutable() const { return is_mutable; }
	};
	using VariableReferences= std::unordered_map<StoredVariablePtr, Reference>;

	struct VariableEntry
	{
		VariableReferences inner_references;
		bool is_moved= false;
	};

	struct AchievableVariables
	{
		std::unordered_set<StoredVariablePtr> variables;
		bool any_variable_is_mutable= false;
	};

	using VariablesContainer= std::unordered_map<StoredVariablePtr, VariableEntry>;

public:
	VariablesState()= default;
	explicit VariablesState( VariablesContainer variables );

	void AddVariable( const StoredVariablePtr& var );
	void RemoveVariable( const StoredVariablePtr& var );
	bool AddPollution( const StoredVariablePtr& dst, const StoredVariablePtr& src, bool is_mutable ); // returns true, if ok
	void AddPollutionForArgInnerVariable( const StoredVariablePtr& arg, const StoredVariablePtr& inner_variable );
	void Move( const StoredVariablePtr& var ); // returns true, if ok
	bool VariableIsMoved( const StoredVariablePtr& var ) const;

	const VariablesContainer& GetVariables() const;
	const VariableReferences& GetVariableReferences( const StoredVariablePtr& var ) const;
	AchievableVariables RecursiveGetAllReferencedVariables( const StoredVariablePtr& stored_variable ) const;

	// For merging of 'if-else' and 'while' we needs deactivate and reactivate locks.
	void ActivateLocks();
	void DeactivateLocks();

private:
	VariablesContainer variables_;
};

struct VaraibleReferencesCounter
{
	unsigned int  mut= 0u;
	unsigned int imut= 0u;
};

struct ClassField final
{
	Type type;
	unsigned int index= 0u;
	ClassProxyWeakPtr class_;
	bool is_reference= false;
	bool is_mutable= true;
};

// "this" + functions set of class of "this"
struct ThisOverloadedMethodsSet final
{
	Variable this_;
	OverloadedFunctionsSet overloaded_methods_set;
};

struct TemplateDependentValue final
{};

struct YetNotDeducedTemplateArg final
{};

struct ErrorValue final
{};

class Value final
{
public:
	Value();
	Value( Variable variable, const FilePos& file_pos );
	Value( StoredVariablePtr stored_variable, const FilePos& file_pos  );
	Value( FunctionVariable function_variable );
	Value( OverloadedFunctionsSet functions_set );
	Value( Type type, const FilePos& file_pos );
	Value( ClassField class_field, const FilePos& file_pos );
	Value( ThisOverloadedMethodsSet class_field );
	Value( const NamesScopePtr& namespace_, const FilePos& file_pos );
	Value( const TypeTemplatePtr& type_template, const FilePos& file_pos );
	Value( TemplateDependentValue template_dependent_value );
	Value( YetNotDeducedTemplateArg yet_not_deduced_template_arg );
	Value( ErrorValue error_value );

	const Type& GetType() const;
	int GetKindIndex() const;
	const FilePos& GetFilePos() const;

	bool IsTemplateParameter() const;
	void SetIsTemplateParameter( bool is_template_parameter );

	// Fundamental, class, array types
	Variable* GetVariable();
	const Variable* GetVariable() const;
	// Stored variable
	StoredVariablePtr GetStoredVariable() const;
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
	TypeTemplatePtr GetTypeTemplate() const;
	// Template-dependent value
	TemplateDependentValue* GetTemplateDependentValue();
	const TemplateDependentValue* GetTemplateDependentValue() const;
	// Yet not deduced template arg
	YetNotDeducedTemplateArg* GetYetNotDeducedTemplateArg();
	const YetNotDeducedTemplateArg* GetYetNotDeducedTemplateArg() const;
	// Error value
	ErrorValue* GetErrorValue();
	const ErrorValue* GetErrorValue() const;

private:
	boost::variant<
		Variable,
		StoredVariablePtr,
		FunctionVariable,
		OverloadedFunctionsSet,
		Type,
		ClassField,
		ThisOverloadedMethodsSet,
		NamesScopePtr,
		TypeTemplatePtr,
		TemplateDependentValue,
		YetNotDeducedTemplateArg,
		ErrorValue > something_;

	// File_pos used as unique id for entry, needed for imports merging.
	// Two values are 100% same, if their file_pos are identical.
	// Not for all values file_pos required, so, fill it with zeros for it.
	FilePos file_pos_= { 0u, 0u, 0u };
	bool is_template_parameter_= false;
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

	bool IsAncestorFor( const NamesScope& other ) const;
	const ProgramString& GetThisNamespaceName() const;
	void SetThisNamespaceName( ProgramString name );

	// Returns nullptr, if name already exists in this scope.
	InsertedName* AddName( const ProgramString& name, Value value );

	// Resolve simple name only in this scope.
	InsertedName* GetThisScopeName( const ProgramString& name ) const;
	InsertedName& GetTemplateDependentValue();

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
	ProgramString name_;
	const NamesScope* parent_;
	NamesMap names_map_;
};

struct NameResolvingKey final
{
	const Synt::ComplexName::Component* components;
	size_t component_count;
};

struct NameResolvingKeyHasher
{
	size_t operator()( const NameResolvingKey& key ) const;
	bool operator()( const NameResolvingKey& a, const NameResolvingKey& b ) const;
};

struct ResolvingCacheValue final
{
	NamesScope::InsertedName name;
	size_t name_components_cut;
};

typedef
	std::unordered_map<
		NameResolvingKey,
		ResolvingCacheValue,
		NameResolvingKeyHasher,
		NameResolvingKeyHasher > ResolvingCache;

typedef boost::variant< Variable, Type > TemplateParameter;

struct TemplateClassKey
{
	TypeTemplatePtr template_;
	ProgramString class_name_encoded;
};

struct TemplateClassKeyHasher
{
	size_t operator()( const TemplateClassKey& key ) const;
	bool operator()( const TemplateClassKey& a, const TemplateClassKey& b ) const;
};

typedef std::unordered_map< TemplateClassKey, ClassProxyPtr, TemplateClassKeyHasher, TemplateClassKeyHasher > TemplateClassesCache;

struct Class final
{
	Class( const ProgramString& name, const NamesScope* parent_scope );
	~Class();

	Class( const Class& )= delete;
	Class( Class&& )= delete;

	Class& operator=( const Class& )= delete;
	Class& operator=( Class&& )= delete;

	// If you change this, you must change CodeBuilder::CopyClass too!

	NamesScope members;
	size_t field_count= 0u;
	size_t references_tags_count= 0u;
	bool is_incomplete= true;
	bool have_explicit_noncopy_constructors= false;
	bool is_default_constructible= false;
	bool is_copy_constructible= false;
	bool have_destructor= false;
	bool is_copy_assignable= false;

	FilePos forward_declaration_file_pos= FilePos{ 0u, 0u, 0u };
	FilePos body_file_pos= FilePos{ 0u, 0u, 0u };

	llvm::StructType* llvm_type;

	struct BaseTemplate
	{
		TypeTemplatePtr class_template;
		std::vector<TemplateParameter> template_parameters;
	};

	// Exists only for classes, generated from class templates.
	boost::optional<BaseTemplate> base_template;
};

struct Enum
{
	Enum( const ProgramString& name, const NamesScope* parent_scope );

	NamesScope members;
	FundamentalType underlaying_type; // must be integer
};

struct TypeTemplate final
{
	struct TemplateParameter
	{
		ProgramString name;
		const Synt::ComplexName* type_name= nullptr; // Exists for value parameters.
	};

	// Sorted in order of first parameter usage in signature.
	std::vector< TemplateParameter > template_parameters;

	std::vector< const Synt::IExpressionComponent* > signature_arguments;
	std::vector< const Synt::IExpressionComponent* > default_signature_arguments;
	size_t first_optional_signature_argument= ~0u;

	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::TemplateBase* syntax_element= nullptr;

	ResolvingCache resolving_cache;
	NamesScope* parent_namespace= nullptr; // Changes after import.
};

typedef boost::variant< int, Type, Variable > DeducibleTemplateParameter; // int means not deduced
typedef std::vector<DeducibleTemplateParameter> DeducibleTemplateParameters;

const ProgramString& GetFundamentalTypeName( U_FundamentalType fundamental_type );

} //namespace CodeBuilderLLVMPrivate

} // namespace U
