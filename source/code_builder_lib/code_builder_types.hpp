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

#include "../lex_synt_lib/syntax_elements.hpp"
#include "references_graph.hpp"
#include "lang_types.hpp"
#include "small_types.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

using Synt::ClassMemberVisibility;

struct Function;
struct FunctionPointer;
struct Array;
struct Tuple;
class Class;
struct Enum;

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

class NamesScope;
using NamesScopePtr= std::shared_ptr<NamesScope>;

struct TemplateBase;
using TemplateBasePtr= std::shared_ptr<TemplateBase>;

struct TypeTemplate;
using TypeTemplatePtr= std::shared_ptr<TypeTemplate>;

struct FunctionTemplate;
using FunctionTemplatePtr= std::shared_ptr<FunctionTemplate>;

class DeducedTemplateParameter;

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

	bool ReferenceIsConvertibleTo( const Type& other ) const;

	bool IsDefaultConstructible() const;
	bool IsCopyConstructible() const;
	bool IsCopyAssignable() const;
	bool HaveDestructor() const;
	bool CanBeConstexpr() const;
	bool IsAbstract() const;
	size_t ReferencesTagsCount() const;

	llvm::Type* GetLLVMType() const;

	// Convert type name to human-readable format.
	ProgramString ToString() const;

private:
	friend bool operator==( const Type&, const Type&);

	using FunctionPtr= std::unique_ptr<Function>;
	using FunctionPointerPtr= std::unique_ptr<FunctionPointer>;
	using ArrayPtr= std::unique_ptr<Array>;

	boost::variant<
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

	struct InToOutReferences
	{
		// Numers of args.
		ArgsVector<size_t> args_references;
		// Pairs of arg number and tag number.
		ArgsVector< ArgReference> inner_args_references;
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

	bool PointerCanBeConvertedTo( const Function& other ) const;

public:
	// If this changed, virtual functions compare function must be changed too!
	Type return_type;
	bool return_value_is_reference= false;
	bool return_value_is_mutable= false;
	ArgsVector<Arg> args;
	bool unsafe= false;

	// for functions, returning references this is references of reference itslef.
	// For function, returning values, this is inner references.
	InToOutReferences return_references;
	std::unordered_set< ReferencePollution, ReferencePollutionHasher > references_pollution;

	llvm::FunctionType* llvm_function_type= nullptr;
};

struct FunctionPointer
{
	Function function;
	llvm::PointerType* llvm_function_pointer_type= nullptr;
};

bool operator==( const Function::InToOutReferences& l, const Function::InToOutReferences& r );
bool operator!=( const Function::InToOutReferences& l, const Function::InToOutReferences& r );
bool operator==( const Function::Arg& l, const Function::Arg& r );
bool operator!=( const Function::Arg& l, const Function::Arg& r );
bool operator==( const Function& l, const Function& r );
bool operator!=( const Function& l, const Function& r );
bool operator==( const FunctionPointer& l, const FunctionPointer& r );
bool operator!=( const FunctionPointer& l, const FunctionPointer& r );

struct Array final
{
	Type type;
	uint64_t size= 0u;

	llvm::ArrayType* llvm_type= nullptr;
};

bool operator==( const Array& r, const Array& l );
bool operator!=( const Array& r, const Array& l );

struct FunctionVariable final
{
	const Synt::Function* syntax_element= nullptr;
	Synt::VirtualFunctionKind virtual_function_kind= Synt::VirtualFunctionKind::None;

	enum class ConstexprKind
	{
		NonConstexpr,
		ConstexprIncomplete,  // Can be used in body of constexpr functions, but result of call can not be constexpr.
		ConstexprComplete,
		ConstexprAuto, // May be, or may be not constexpr.
	};

	Type type; // Function type 100%

	// For function templates is nonempty and have size of args. Needs for selection of better (more specialized) template function.
	std::vector<DeducedTemplateParameter> deduced_temlpate_parameters;

	unsigned int virtual_table_index= ~0u; // For virtual functions number in virtual functions table in class of first arg(this).
	bool have_body= false;
	bool is_this_call= false;
	bool is_generated= false;
	bool is_deleted= false;
	bool no_mangle= false;
	bool is_constructor= false;
	bool is_conversion_constructor= false;
	bool return_type_is_auto= false; // true, if return type must be deduced and not deduced yet.

	ConstexprKind constexpr_kind= ConstexprKind::NonConstexpr;

	llvm::Function* llvm_function= nullptr;

	FilePos prototype_file_pos= FilePos{ 0u, 0u, 0u };
	FilePos body_file_pos= FilePos{ 0u, 0u, 0u };

	bool VirtuallyEquals( const FunctionVariable& other ) const;
};

struct OverloadedFunctionsSet
{
	std::vector<FunctionVariable> functions;
	std::vector<FunctionTemplatePtr> template_functions;

	// Is incomplete, if there are some syntax elements in containers.
	std::vector<const Synt::Function*> syntax_elements;
	std::vector<const Synt::Function*> out_of_line_syntax_elements;
	std::vector<const Synt::FunctionTemplate*> template_syntax_elements;

	ClassProxyPtr base_class;

	bool have_nomangle_function= false;
};

struct TypeTemplatesSet
{
	std::vector<TypeTemplatePtr> type_templates;

	// Is incomplete, if there are some syntax elements in containers.
	std::vector<const Synt::TypeTemplateBase*> syntax_elements;
};

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

	ReferencesGraphNodePtr node; // May be null for global variables.

	Variable()= default;
	Variable(Type in_type,
		Location in_location= Location::Pointer, ValueType in_value_type= ValueType::ConstReference,
		llvm::Value* in_llvm_value= nullptr, llvm::Constant* in_constexpr_value= nullptr );
};

struct ClassField final
{
	Type type;
	ClassProxyWeakPtr class_;
	const Synt::ClassField* syntax_element= nullptr;
	unsigned int index= ~0u;
	unsigned int original_index= ~0u;
	bool is_mutable= true;
	bool is_reference= false;

	ClassField()= default;
	ClassField( const ClassProxyPtr& in_class, Type in_type, unsigned int in_index, bool in_is_mutable, bool in_is_reference );
};

// "this" + functions set of class of "this"
struct ThisOverloadedMethodsSet final
{
public:
	ThisOverloadedMethodsSet();
	ThisOverloadedMethodsSet( const ThisOverloadedMethodsSet& other );
	ThisOverloadedMethodsSet( ThisOverloadedMethodsSet&& other ) noexcept= default;

	ThisOverloadedMethodsSet& operator=( const ThisOverloadedMethodsSet& other );
	ThisOverloadedMethodsSet& operator=( ThisOverloadedMethodsSet&& other ) noexcept= default;

	OverloadedFunctionsSet& GetOverloadedFunctionsSet();
	const OverloadedFunctionsSet& GetOverloadedFunctionsSet() const;

public:
	Variable this_;

private:
	// Store "OverloadedFunctionsSet" indirectly, because it is too hevy, to put it in value together with "variable".
	std::unique_ptr<OverloadedFunctionsSet> overloaded_methods_set_;
};

struct StaticAssert
{
	const Synt::StaticAssert* syntax_element= nullptr; // Null if completed.
};

struct Typedef
{
	const Synt::Typedef* syntax_element= nullptr;
};

struct IncompleteGlobalVariable
{
	// Exists one of.
	const Synt::VariablesDeclaration* variables_declaration= nullptr;
	const Synt::AutoVariableDeclaration* auto_variable_declaration= nullptr;

	size_t element_index= ~0u; // For VariablesDeclaration - index of variable.
	ProgramString name;
};

struct YetNotDeducedTemplateArg final
{};

struct ErrorValue final
{};

class Value final
{
public:
	Value();
	Value( Variable variable, const FilePos& file_pos );
	Value( FunctionVariable function_variable );
	Value( OverloadedFunctionsSet functions_set );
	Value( Type type, const FilePos& file_pos );
	Value( ClassField class_field, const FilePos& file_pos );
	Value( ThisOverloadedMethodsSet class_field );
	Value( const NamesScopePtr& namespace_, const FilePos& file_pos );
	Value( TypeTemplatesSet type_templates, const FilePos& file_pos );
	Value( StaticAssert static_assert_, const FilePos& file_pos );
	Value( Typedef typedef_, const FilePos& file_pos );
	Value( IncompleteGlobalVariable incomplete_global_variable, const FilePos& file_pos );
	Value( YetNotDeducedTemplateArg yet_not_deduced_template_arg );
	Value( ErrorValue error_value );

	int GetKindIndex() const;
	ProgramString GetKindName() const;
	const FilePos& GetFilePos() const;

	bool IsTemplateParameter() const;
	void SetIsTemplateParameter( bool is_template_parameter );

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
	ClassField* GetClassField();
	const ClassField* GetClassField() const;
	// This + methods set
	ThisOverloadedMethodsSet* GetThisOverloadedMethodsSet();
	const ThisOverloadedMethodsSet* GetThisOverloadedMethodsSet() const;
	// Namespace
	NamesScopePtr GetNamespace() const;
	// Type templates sel
	TypeTemplatesSet* GetTypeTemplatesSet();
	const TypeTemplatesSet* GetTypeTemplatesSet() const;
	// static assert
	StaticAssert* GetStaticAssert();
	const StaticAssert* GetStaticAssert() const;
	// typedef
	Typedef* GetTypedef();
	const Typedef* GetTypedef() const;
	// incomplete global variable
	IncompleteGlobalVariable* GetIncompleteGlobalVariable();
	const IncompleteGlobalVariable* GetIncompleteGlobalVariable() const;
	// Yet not deduced template arg
	YetNotDeducedTemplateArg* GetYetNotDeducedTemplateArg();
	const YetNotDeducedTemplateArg* GetYetNotDeducedTemplateArg() const;
	// Error value
	ErrorValue* GetErrorValue();
	const ErrorValue* GetErrorValue() const;

private:
	boost::variant<
		Variable,
		FunctionVariable,
		OverloadedFunctionsSet,
		Type,
		ClassField,
		ThisOverloadedMethodsSet,
		NamesScopePtr,
		TypeTemplatesSet,
		StaticAssert,
		Typedef,
		IncompleteGlobalVariable,
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
	NamesScope( ProgramString name, NamesScope* parent );

	NamesScope( const NamesScope&)= delete;
	NamesScope& operator=( const NamesScope&)= delete;

	bool IsAncestorFor( const NamesScope& other ) const;
	const ProgramString& GetThisNamespaceName() const;
	void SetThisNamespaceName( ProgramString name );

	// Get full name (with enclosing namespaces) un human-readable format.
	ProgramString ToString() const;

	// Returns nullptr, if name already exists in this scope.
	Value* AddName( const ProgramString& name, Value value );

	// Resolve simple name only in this scope.
	Value* GetThisScopeValue( const ProgramString& name );
	const Value* GetThisScopeValue( const ProgramString& name ) const;

	NamesScope* GetParent();
	const NamesScope* GetParent() const;
	NamesScope* GetRoot();
	const NamesScope* GetRoot() const;
	void SetParent( NamesScope* parent );

	void AddAccessRightsFor( const ClassProxyPtr& class_, ClassMemberVisibility visibility );
	ClassMemberVisibility GetAccessFor( const ClassProxyPtr& class_ ) const;
	void CopyAccessRightsFrom( const NamesScope& src );

	void SetErrors( CodeBuilderErrorsContainer& errors );
	CodeBuilderErrorsContainer& GetErrors() const;

	template<class Func>
	void ForEachInThisScope( const Func& func )
	{
		++iterating_;
		ProgramString name;
		name.reserve(max_key_size_);
		for( auto& inserted_name : names_map_ )
		{
			name.assign(
				reinterpret_cast<const sprache_char*>(inserted_name.getKeyData()),
				inserted_name.getKeyLength() / sizeof(sprache_char) );
			func( const_cast<const ProgramString&>(name), inserted_name.second );
		}
		--iterating_;
	}

	template<class Func>
	void ForEachInThisScope( const Func& func ) const
	{
		++iterating_;
		ProgramString name;
		name.reserve(max_key_size_);
		for( const auto& inserted_name : names_map_ )
		{
			name.assign(
				reinterpret_cast<const sprache_char*>(inserted_name.getKeyData()),
				inserted_name.getKeyLength() / sizeof(sprache_char) );
			func( const_cast<const ProgramString&>(name), inserted_name.second );
		}
		--iterating_;
	}

	template<class Func>
	void ForEachValueInThisScope( const Func& func )
	{
		++iterating_;
		for( auto& inserted_name : names_map_ )
			func( inserted_name.second );
		--iterating_;
	}

	template<class Func>
	void ForEachValueInThisScope( const Func& func ) const
	{
		++iterating_;
		for( const auto& inserted_name : names_map_ )
			func( inserted_name.second );
		--iterating_;
	}

private:
	ProgramString name_;
	NamesScope* parent_;

	// Use StringMap here, with "const char*" key.
	// interpritate ProgramString bytes as chars.
	// TODO - maybe replace "ProgramString" with UTF-8 std::string?
	llvm::StringMap< Value > names_map_;
	size_t max_key_size_= 0u;

	mutable size_t iterating_= 0u;
	std::unordered_map<ClassProxyPtr, ClassMemberVisibility> access_rights_;

	CodeBuilderErrorsContainer* errors_= nullptr;
};

using TemplateParameter= boost::variant< Variable, Type >;

class Class final
{
public:
	Class( const ProgramString& name, NamesScope* parent_scope );
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
		std::vector<TemplateParameter> signature_parameters;
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
	ProgramStringMap< ClassMemberVisibility > members_visibility;

	const Synt::Class* syntax_element= nullptr;

	size_t field_count= 0u;
	size_t references_tags_count= 0u;
	TypeCompleteness completeness= TypeCompleteness::Incomplete;
	bool is_typeinfo= false;
	bool have_explicit_noncopy_constructors= false;
	bool is_default_constructible= false;
	bool is_copy_constructible= false;
	bool have_destructor= false;
	bool is_copy_assignable= false;
	bool can_be_constexpr= false;
	bool have_shared_state= false;

	FilePos forward_declaration_file_pos= FilePos{ 0u, 0u, 0u };
	FilePos body_file_pos= FilePos{ 0u, 0u, 0u };

	llvm::StructType* llvm_type;

	// Exists only for classes, generated from class templates.
	boost::optional<BaseTemplate> base_template;

	Kind kind= Kind::Struct;

	struct Parent
	{
		ClassProxyPtr class_;
		unsigned int field_number= ~0u; // Allways 0 for base class.
	};
	ClassProxyPtr base_class;
	std::vector<Parent> parents; // Parents, include base class.

	std::vector<VirtualTableEntry> virtual_table;
	llvm::StructType* virtual_table_llvm_type= nullptr;
	llvm::GlobalVariable* this_class_virtual_table= nullptr; // May be null for interfaces and abstract classes.
	llvm::GlobalVariable* polymorph_type_id= nullptr; // Exists in polymorph classes.

	// Key - sequence of classes from child to parent. This class not included.
	// Virtual table destination is lats key element.
	std::map< std::vector<ClassProxyPtr>, llvm::GlobalVariable* > ancestors_virtual_tables;
};

struct Enum
{
	Enum( const ProgramString& name, NamesScope* parent_scope );

	NamesScope members;
	uint64_t element_count= 0u;
	FundamentalType underlaying_type; // must be integer

	const Synt::Enum* syntax_element= nullptr; // Null if completed
};

struct TemplateBase
{
	virtual ~TemplateBase()= default;

	struct TemplateParameter
	{
		ProgramString name;
		const Synt::ComplexName* type_name= nullptr; // Exists for value parameters.
	};

	std::vector< TemplateParameter > template_parameters;

	NamesScope* parent_namespace= nullptr; // NamesScope, where defined. NOT changed after import.

	FilePos file_pos;
};

struct TypeTemplate final : TemplateBase
{
	std::vector< const Synt::Expression* > signature_arguments;
	std::vector< const Synt::Expression* > default_signature_arguments;
	size_t first_optional_signature_argument= ~0u;

	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::TypeTemplateBase* syntax_element= nullptr;
};

using DeducibleTemplateParameter= boost::variant< int, Type, Variable >; // int means not deduced
using DeducibleTemplateParameters= std::vector<DeducibleTemplateParameter>;

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

	struct Tuple
	{
		std::vector<DeducedTemplateParameter> element_types;
	};

	struct Function
	{
		std::unique_ptr<DeducedTemplateParameter> return_type;
		std::vector<DeducedTemplateParameter> argument_types;

		Function()= default;
		Function(Function&&)= default;
		Function& operator=(Function&&)= default;

		Function( const Function& other );
		Function& operator=( const Function& other );
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
	DeducedTemplateParameter( Tuple tuple );
	DeducedTemplateParameter( Function function );
	DeducedTemplateParameter( Template template_ );

	bool IsInvalid() const;
	bool IsType() const;
	bool IsVariable() const;
	bool IsTemplateParameter() const;
	const Array* GetArray() const;
	const Tuple* GetTuple() const;
	const Function* GetFunction() const;
	const Template* GetTemplate() const;

private:
	boost::variant<
		Invalid,
		Type,
		Variable,
		TemplateParameter,
		Array,
		Tuple,
		Function,
		Template> something_;
};

const ProgramString& GetFundamentalTypeName( U_FundamentalType fundamental_type );

} //namespace CodeBuilderLLVMPrivate

} // namespace U
