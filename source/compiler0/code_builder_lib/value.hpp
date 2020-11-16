#pragma once
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"
#include "references_graph.hpp"
#include "type.hpp"


namespace U
{

namespace CodeBuilderPrivate
{

class TemplateSignatureParam;

class NamesScope;
using NamesScopePtr= std::shared_ptr<NamesScope>;

struct TemplateBase;
using TemplateBasePtr= std::shared_ptr<TemplateBase>;

struct TypeTemplate;
using TypeTemplatePtr= std::shared_ptr<TypeTemplate>;

struct FunctionTemplate;
using FunctionTemplatePtr= std::shared_ptr<FunctionTemplate>;

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

	// For functions generated from templates.
	std::shared_ptr<FunctionTemplate> base_template;

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

	SrcLoc prototype_file_pos;
	SrcLoc body_file_pos;

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
	std::vector<const Synt::TypeTemplate*> syntax_elements;
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

	// Exists only for constant expressions.
	llvm::Constant* constexpr_value= nullptr;

	ReferencesGraphNodePtr node; // May be null for global variables.

	Variable()= default;
	Variable(Type in_type,
		Location in_location= Location::Pointer, ValueType in_value_type= ValueType::ConstReference,
		llvm::Value* in_llvm_value= nullptr, llvm::Constant* in_constexpr_value= nullptr );
};

// Used for displaying of template args.
std::string ConstantVariableToString( const Variable& variable );

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
	std::string name;
};

struct YetNotDeducedTemplateArg final
{};

struct ErrorValue final
{};

class Value final
{
public:
	Value() = default;
	Value( Variable variable, const SrcLoc& file_pos );
	Value( OverloadedFunctionsSet functions_set );
	Value( Type type, const SrcLoc& file_pos );
	Value( ClassField class_field, const SrcLoc& file_pos );
	Value( ThisOverloadedMethodsSet class_field );
	Value( const NamesScopePtr& namespace_, const SrcLoc& file_pos );
	Value( TypeTemplatesSet type_templates, const SrcLoc& file_pos );
	Value( StaticAssert static_assert_, const SrcLoc& file_pos );
	Value( Typedef typedef_, const SrcLoc& file_pos );
	Value( IncompleteGlobalVariable incomplete_global_variable, const SrcLoc& file_pos );
	Value( YetNotDeducedTemplateArg yet_not_deduced_template_arg );
	Value( ErrorValue error_value );

	size_t GetKindIndex() const;
	std::string GetKindName() const;
	const SrcLoc& GetFilePos() const;

	Variable* GetVariable();
	const Variable* GetVariable() const;
	// Function set
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
	// Type templates set
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
	std::variant<
		Variable,
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
	SrcLoc file_pos_;
};

} //namespace CodeBuilderPrivate

} // namespace U
