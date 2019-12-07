#pragma once
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "push_disable_llvm_warnings.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include "pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"
#include "references_graph.hpp"
#include "lang_types.hpp"
#include "small_types.hpp"
#include "value.hpp"

namespace U
{

namespace CodeBuilderPrivate
{

using Synt::ClassMemberVisibility;

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

using TemplateParameter= std::variant< Variable, Type >;

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
	std::optional<BaseTemplate> base_template;

	// If this class is typeinfo, contains source type.
	std::optional<Type> typeinfo_type;

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
	size_t element_count= 0u;
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

	enum class Kind
	{
		Class,
		Typedef,
	};

	Kind kind= Kind::Class;
	// Store syntax tree element for instantiation.
	// Syntax tree must live longer, than this struct.
	const Synt::TypeTemplateBase* syntax_element= nullptr;
};

using DeducibleTemplateParameter= std::variant< int, Type, Variable >; // int means not deduced
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
	std::variant<
		Invalid,
		Type,
		Variable,
		TemplateParameter,
		Array,
		Tuple,
		Function,
		Template> something_;
};

} //namespace CodeBuilderLLVMPrivate

} // namespace U
