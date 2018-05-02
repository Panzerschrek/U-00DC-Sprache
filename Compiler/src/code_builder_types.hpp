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

using Synt::ClassMemberVisibility;

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

struct TemplateBase;
typedef std::shared_ptr<TemplateBase> TemplateBasePtr;

struct TypeTemplate;
typedef std::shared_ptr<TypeTemplate> TypeTemplatePtr;

struct FunctionTemplate;
typedef std::shared_ptr<FunctionTemplate> FunctionTemplatePtr;

class DeducedTemplateParameter;

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

	bool ReferenceIsConvertibleTo( const Type& other ) const;

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
	// If this changed, virtual functions compare function must be changed too!
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

	// For function templates is nonempty and have size of args. Needs for selection of better (more specialized) template function.
	std::vector<DeducedTemplateParameter> deduced_temlpate_parameters;

	unsigned int virtual_table_index= ~0u; // For virtual functions number in virtual functions table in class of first arg(this).
	bool have_body= true;
	bool is_this_call= false;
	bool is_generated= false;

	llvm::Function* llvm_function= nullptr;

	FilePos prototype_file_pos= FilePos{ 0u, 0u, 0u };
	FilePos body_file_pos= FilePos{ 0u, 0u, 0u };

	bool VirtuallyEquals( const FunctionVariable& other ) const;
};

struct OverloadedFunctionsSet
{
	std::vector<FunctionVariable> functions;

	std::vector<FunctionTemplatePtr> template_functions;
};

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
		ReferenceArg,
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

	void AddAccessRightsFor( const ClassProxyPtr& class_, ClassMemberVisibility visibility );
	ClassMemberVisibility GetAccessFor( const ClassProxyPtr& class_ ) const;

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
	std::unordered_map<ClassProxyPtr, ClassMemberVisibility> access_rights_;
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

class Class final
{
public:
	Class( const ProgramString& name, const NamesScope* parent_scope );
	~Class();

	Class( const Class& )= delete;
	Class( Class&& )= delete;

	Class& operator=( const Class& )= delete;
	Class& operator=( Class&& )= delete;

	ClassMemberVisibility GetMemberVisibility( const ProgramString& member_name ) const;
	void SetMemberVisibility( const ProgramString& member_name, ClassMemberVisibility visibility );

public:
	struct BaseTemplate
	{
		TypeTemplatePtr class_template;
		std::vector<TemplateParameter> template_parameters;
	};

	enum class Kind
	{
		Struct,
		NonPolymorph,
		Interface,
		Abstract,
		PolymorphNonFinal,
		PolymorphFinal,
	};

	struct VirtualTableEntry
	{
		ProgramString name;
		FunctionVariable function_variable;
		bool is_pure= false;
		bool is_final= false;
	};

public:
	// If you change this, you must change CodeBuilder::CopyClass too!

	NamesScope members;

	// have no visibility for member, means it is public.
	// TODO - maybe use unordered_map?
	std::map< ProgramString, ClassMemberVisibility > members_visibility;

	size_t field_count= 0u;
	size_t references_tags_count= 0u;
	bool is_incomplete= true;
	bool have_explicit_noncopy_constructors= false;
	bool is_default_constructible= false;
	bool is_copy_constructible= false;
	bool have_destructor= false;
	bool is_copy_assignable= false;
	bool have_template_dependent_parents= false;

	FilePos forward_declaration_file_pos= FilePos{ 0u, 0u, 0u };
	FilePos body_file_pos= FilePos{ 0u, 0u, 0u };

	llvm::StructType* llvm_type;

	// Exists only for classes, generated from class templates.
	boost::optional<BaseTemplate> base_template;

	Kind kind= Kind::Struct;
	ClassProxyPtr base_class; // We can have single non-interface base class.
	unsigned int base_class_field_number= 0u;
	std::vector<ClassProxyPtr> parents; // Class have fields with numbers 0-N for parents.
	std::vector<unsigned int> parents_fields_numbers;

	std::vector<VirtualTableEntry> virtual_table;
	llvm::StructType* virtual_table_llvm_type= nullptr;
	llvm::GlobalVariable* this_class_virtual_table= nullptr; // May be null for interfaces and abstract classes.
	unsigned int virtual_table_field_number= 0u;

	// Key - sequence of classes from child to parent. This class not included.
	// Virtual table destination is lats key element.
	std::map< std::vector<ClassProxyPtr>, llvm::GlobalVariable* > ancestors_virtual_tables;
};

struct Enum
{
	Enum( const ProgramString& name, const NamesScope* parent_scope );

	NamesScope members;
	FundamentalType underlaying_type; // must be integer
};

struct TemplateBase
{
	struct TemplateParameter
	{
		ProgramString name;
		const Synt::ComplexName* type_name= nullptr; // Exists for value parameters.
	};

	std::vector< TemplateParameter > template_parameters;

	ResolvingCache resolving_cache;
	NamesScope* parent_namespace= nullptr; // Changes after import.

	FilePos file_pos;
};

struct TypeTemplate final : TemplateBase
{
	std::vector< const Synt::IExpressionComponent* > signature_arguments;
	std::vector< const Synt::IExpressionComponent* > default_signature_arguments;
	size_t first_optional_signature_argument= ~0u;

	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::TypeTemplateBase* syntax_element= nullptr;

};

typedef boost::variant< int, Type, Variable > DeducibleTemplateParameter; // int means not deduced
typedef std::vector<DeducibleTemplateParameter> DeducibleTemplateParameters;

struct FunctionTemplate final : public TemplateBase
{
	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::FunctionTemplate* syntax_element= nullptr;

	ClassProxyPtr base_class;

	std::vector< std::pair< ProgramString, Value > > known_template_parameters;
};

class DeducedTemplateParameter
{
public:
	struct Invalid{};
	struct Type{};
	struct Variable{};
	struct TemplateParameter{};

	struct Array
	{
		std::unique_ptr<DeducedTemplateParameter> size;
		std::unique_ptr<DeducedTemplateParameter> type;

		Array()= default;
		Array(Array&&)= default;
		Array& operator=(Array&&)= default;

		Array( const Array& other );
		Array& operator=( const Array& other );
	};

	struct Template
	{
		std::vector<DeducedTemplateParameter> args;
	};

public:
	DeducedTemplateParameter( Invalid invalid= Invalid() );
	DeducedTemplateParameter( Type type );
	DeducedTemplateParameter( Variable variable );
	DeducedTemplateParameter( TemplateParameter template_parameter );
	DeducedTemplateParameter( Array array );
	DeducedTemplateParameter( Template template_ );

	bool IsInvalid() const;
	bool IsType() const;
	bool IsVariable() const;
	bool IsTemplateParameter() const;
	const Array* GetArray() const;
	const Template* GetTemplate() const;

private:
	boost::variant<
		Invalid,
		Type,
		Variable,
		TemplateParameter,
		Array,
		Template> something_;
};

const ProgramString& GetFundamentalTypeName( U_FundamentalType fundamental_type );

} //namespace CodeBuilderLLVMPrivate

} // namespace U
