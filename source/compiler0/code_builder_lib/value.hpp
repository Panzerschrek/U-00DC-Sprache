#pragma once
#include "../../code_builder_lib_common/push_disable_llvm_warnings.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Constants.h>
#include "../../code_builder_lib_common/pop_llvm_warnings.hpp"

#include "../lex_synt_lib/syntax_elements.hpp"
#include "type.hpp"


namespace U
{

class NamesScope;
using NamesScopePtr= std::shared_ptr<NamesScope>;

struct TypeTemplate;
using TypeTemplatePtr= std::shared_ptr<const TypeTemplate>;

struct FunctionTemplate;
using FunctionTemplatePtr= std::shared_ptr<const FunctionTemplate>;

struct LazyLLVMFunction
{
	std::string name_mangled;
	llvm::Function* function= nullptr;

	LazyLLVMFunction()= delete;
	LazyLLVMFunction(std::string name) : name_mangled(std::move(name)) {}
	LazyLLVMFunction(const LazyLLVMFunction&)= delete;
	LazyLLVMFunction& operator=(const LazyLLVMFunction&)= delete;
};

using LazyLLVMFunctionPtr= std::shared_ptr<LazyLLVMFunction>;

struct FunctionVariable final
{
	const Synt::Function* syntax_element= nullptr;
	Synt::VirtualFunctionKind virtual_function_kind= Synt::VirtualFunctionKind::None;

	enum class ConstexprKind : uint8_t
	{
		NonConstexpr,
		ConstexprIncomplete,  // Can be used in body of constexpr functions, but result of call can not be constexpr.
		ConstexprComplete,
		ConstexprAuto, // May be, or may be not constexpr.
	};

	FunctionType type;

	uint32_t virtual_table_index= ~0u; // For virtual functions number in virtual functions table in class of first arg(this).
	bool have_body= false;
	bool is_this_call= false;
	bool is_generated= false;
	bool is_deleted= false;
	bool no_mangle= false;
	bool is_constructor= false;
	bool is_conversion_constructor= false;
	bool return_type_is_auto= false; // true, if return type must be deduced and not deduced yet.
	bool is_inherited= false;
	bool is_generator= false;
	mutable bool referenced= false;

	ConstexprKind constexpr_kind= ConstexprKind::NonConstexpr;

	// Laziliy-initialized in order to make llvm function type dependent on complete types of arguments.
	LazyLLVMFunctionPtr llvm_function;

	SrcLoc prototype_src_loc;
	SrcLoc body_src_loc;

	bool VirtuallyEquals( const FunctionVariable& other ) const;
};

struct OverloadedFunctionsSet;
using OverloadedFunctionsSetPtr= std::shared_ptr<OverloadedFunctionsSet>;
using OverloadedFunctionsSetConstPtr= std::shared_ptr<const OverloadedFunctionsSet>;

struct OverloadedFunctionsSet
{
	std::vector<FunctionVariable> functions;
	std::vector<FunctionTemplatePtr> template_functions;

	// Is incomplete, if there are some syntax elements in containers.
	std::vector<const Synt::Function*> syntax_elements;
	std::vector<const Synt::Function*> out_of_line_syntax_elements;
	std::vector<const Synt::FunctionTemplate*> template_syntax_elements;

	ClassPtr base_class= nullptr;

	bool have_nomangle_function= false;
};

struct TypeTemplatesSet
{
	std::vector<TypeTemplatePtr> type_templates;

	// Is incomplete, if there are some syntax elements in containers.
	std::vector<const Synt::TypeTemplate*> syntax_elements;
};

struct Variable;
using VariablePtr= std::shared_ptr<const Variable>;
using VariableMutPtr= std::shared_ptr<Variable>;
using VariableWeakPtr= std::weak_ptr<const Variable>;

struct Variable final
{
public:
	enum class Location : uint8_t
	{
		Pointer,
		LLVMRegister,
	};

	Type type;
	llvm::Value* llvm_value= nullptr;

	// Exists only for constant expressions.
	llvm::Constant* constexpr_value= nullptr;

	std::string name;

	ValueType value_type= ValueType::ReferenceImut;
	Location location= Location::Pointer;

	// May be non-null for struct or tuple member nodes.
	VariableWeakPtr parent;
	// May be non-empty for struct or tuple nodes. Field index is used to access field node. Nodes are created lazily.
	mutable std::vector<VariablePtr> children;

	// For variables of types with references inside.
	// Size is equal to number of reference tags of the variable type.
	// Null values are not possible.
	llvm::SmallVector<VariablePtr, 1> inner_reference_nodes;

	// Mark inner reference nodes of variables using this flag.
	// Do this in order to stop references graph search on these nodes while performing reference pollution.
	bool is_variable_inner_reference_node= false;

private:
	Variable(
		Type in_type,
		ValueType in_value_type,
		Location in_location,
		std::string in_name,
		llvm::Value* in_llvm_value,
		llvm::Constant* in_constexpr_value );
public:
	Variable()= delete;
	Variable(const Variable&)= delete;
	Variable(Variable&&)= default;

	Variable& operator=(const Variable&)= delete;
	Variable& operator=(Variable&&)= default;

	static VariableMutPtr Create(
		Type type,
		ValueType value_type,
		Location location,
		std::string name= "",
		llvm::Value* llvm_value= nullptr,
		llvm::Constant* constexpr_value= nullptr );

	// Create child node but do not fill inner nodes (only resize it to proper size).
	static VariableMutPtr CreateChildNode(
		const VariablePtr& parent,
		Type type,
		ValueType value_type,
		Location location,
		std::string name= "",
		llvm::Value* llvm_value= nullptr,
		llvm::Constant* constexpr_value= nullptr );
};

// Used for displaying of template args.
struct TemplateVariableArg;
std::string ConstantVariableToString( const TemplateVariableArg& variable );

struct ClassField final
{
	Type type;
	ClassPtr class_= nullptr;
	const Synt::ClassField* syntax_element= nullptr;
	llvm::SmallVector<uint8_t, 4> inner_reference_tags; // For value fields with references inside - mapping of class inner reference tags to reference tags if this field.
	uint32_t index= ~0u;
	uint32_t original_index= ~0u;
	bool is_mutable= true;
	bool is_reference= false;
	uint8_t reference_tag= 0u; // For reference fields - mapping of class inner reference tag to reference tag.

	ClassField()= default;
	ClassField( ClassPtr in_class, Type in_type, uint32_t in_index, bool in_is_mutable, bool in_is_reference );

	const std::string& GetName() const { return syntax_element != nullptr ? syntax_element->name : c_generated_field_name; }

	static const std::string c_generated_field_name;
};

using ClassFieldPtr= std::shared_ptr<ClassField>;

// "this" + functions set of class of "this"
struct ThisOverloadedMethodsSet final
{
	VariablePtr this_;
	OverloadedFunctionsSetConstPtr overloaded_methods_set;
};

struct StaticAssert
{
	const Synt::StaticAssert* syntax_element= nullptr; // Null if completed.
};

struct TypeAlias
{
	const Synt::TypeAlias* syntax_element= nullptr;
};

struct IncompleteGlobalVariable
{
	// Exists one of.
	const Synt::VariablesDeclaration* variables_declaration= nullptr;
	const Synt::AutoVariableDeclaration* auto_variable_declaration= nullptr;

	size_t element_index= ~0u; // For VariablesDeclaration - index of variable.
};

struct YetNotDeducedTemplateArg final
{};

struct ErrorValue final
{};

class Value final
{
public:
	Value() = default;
	Value( VariablePtr variable );
	Value( VariableMutPtr variable );
	Value( OverloadedFunctionsSetPtr functions_set );
	Value( Type type );
	Value( ClassFieldPtr class_field );
	Value( ThisOverloadedMethodsSet class_field );
	Value( NamesScopePtr namespace_ );
	Value( TypeTemplatesSet type_templates );
	Value( StaticAssert static_assert_ );
	Value( TypeAlias type_alias );
	Value( IncompleteGlobalVariable incomplete_global_variable );
	Value( YetNotDeducedTemplateArg yet_not_deduced_template_arg );
	Value( ErrorValue error_value );

	size_t GetKindIndex() const;
	std::string_view GetKindName() const;

	VariablePtr GetVariable() const;
	// Function set
	OverloadedFunctionsSetPtr GetFunctionsSet() const;
	// Typename
	Type* GetTypeName();
	const Type* GetTypeName() const;
	// Class fields
	ClassFieldPtr GetClassField() const;
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
	// type alias
	TypeAlias* GetTypeAlias();
	const TypeAlias* GetTypeAlias() const;
	// incomplete global variable
	IncompleteGlobalVariable* GetIncompleteGlobalVariable();
	const IncompleteGlobalVariable* GetIncompleteGlobalVariable() const;
	// Yet not deduced template arg
	YetNotDeducedTemplateArg* GetYetNotDeducedTemplateArg();
	const YetNotDeducedTemplateArg* GetYetNotDeducedTemplateArg() const;
	// Error value
	const ErrorValue* GetErrorValue() const;

private:
	std::variant<
		VariablePtr,
		OverloadedFunctionsSetPtr,
		Type,
		ClassFieldPtr,
		ThisOverloadedMethodsSet,
		NamesScopePtr,
		TypeTemplatesSet,
		StaticAssert,
		TypeAlias,
		IncompleteGlobalVariable,
		YetNotDeducedTemplateArg,
		ErrorValue > something_;
};

// Store additional information for value in NamesScope.
struct NamesScopeValue
{
	Value value;

	// SrcLoc used as unique id for entry, needed for imports merging.
	// Two values are 100% same, if their src_loc are identical.
	// Not for all values SrcLoc required, so, fill it with zeros for it.
	SrcLoc src_loc;

	// Used to detect unused code.
	bool referenced= false;

	NamesScopeValue()= default;
	NamesScopeValue( Value in_value, const SrcLoc& in_src_loc, const bool in_referenced= false )
		: value(std::move(in_value)), src_loc(in_src_loc), referenced(in_referenced) {}
};

} // namespace U
