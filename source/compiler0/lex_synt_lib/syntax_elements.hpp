#pragma once
#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "lexical_analyzer.hpp"
#include "operators.hpp"

namespace U
{

namespace Synt
{

struct EmptyVariant{};

struct ComplexName;

struct ArrayTypeName;
struct TypeofTypeName;
struct FunctionType;
struct TupleType;
struct RawPointerType;
struct GeneratorType;

struct UnaryPlus;
struct UnaryMinus;
struct LogicalNot;
struct BitwiseNot;

struct CallOperator;
struct IndexationOperator;
struct MemberAccessOperator;

struct BinaryOperator;
struct TernaryOperator;
struct ReferenceToRawPointerOperator;
struct RawPointerToReferenceOperator;
struct NumericConstant;
struct BooleanConstant;
struct StringLiteral;
struct MoveOperator;
struct TakeOperator;
struct CastMut;
struct CastImut;
struct CastRef;
struct CastRefUnsafe;
struct TypeInfo;
struct NonSyncExpression;
struct SafeExpression;
struct UnsafeExpression;

struct SequenceInitializer;
struct StructNamedInitializer;
struct ConstructorInitializer;
struct ZeroInitializer;
struct UninitializedInitializer;

struct Block;
struct VariablesDeclaration;
struct AutoVariableDeclaration;
struct ReturnOperator;
struct YieldOperator;
struct WhileOperator;
struct RangeForOperator;
struct CStyleForOperator;
struct BreakOperator;
struct ContinueOperator;
struct WithOperator;
struct IfOperator;
struct StaticIfOperator;
struct IfCoroAdvanceOperator;
struct SingleExpressionOperator;
struct AssignmentOperator;
struct AdditiveAssignmentOperator;
struct IncrementOperator;
struct DecrementOperator;
struct StaticAssert;
struct Halt;
struct HaltIf;

struct Function;
struct TypeAlias;
struct Enum;
struct Class;
struct ClassField;
struct ClassVisibilityLabel;
struct TypeTemplate;
struct FunctionTemplate;

struct Namespace;

using FunctionTypePtr= std::unique_ptr<const FunctionType>;
using GeneratorTypePtr= std::unique_ptr<const GeneratorType>;
using BlockPtr= std::unique_ptr<const Block>;
using ClassPtr= std::unique_ptr<const Class>;
using FunctionPtr= std::unique_ptr<const Function>;
using NamespacePtr= std::unique_ptr<const Namespace>;

using TypeName= std::variant<
	EmptyVariant,
	ArrayTypeName,
	ComplexName,
	FunctionTypePtr,
	TupleType,
	RawPointerType,
	GeneratorTypePtr >;

using TypeNamePtr= std::unique_ptr<const TypeName>;

using Expression= std::variant<
	EmptyVariant,
	// Postfix operators
	CallOperator,
	IndexationOperator,
	MemberAccessOperator,
	// Prefix operators
	UnaryPlus,
	UnaryMinus,
	LogicalNot,
	BitwiseNot,
	// Main components
	ComplexName,
	BinaryOperator,
	TernaryOperator,
	ReferenceToRawPointerOperator,
	RawPointerToReferenceOperator,
	NumericConstant,
	BooleanConstant,
	StringLiteral,
	MoveOperator,
	TakeOperator,
	CastMut,
	CastImut,
	CastRef,
	CastRefUnsafe,
	TypeInfo,
	NonSyncExpression,
	SafeExpression,
	UnsafeExpression,
	// Type name in expression context
	ArrayTypeName,
	FunctionTypePtr,
	TupleType,
	RawPointerType,
	GeneratorTypePtr
	>;

using ExpressionPtr= std::unique_ptr<const Expression>;

using Initializer= std::variant<
	EmptyVariant,
	SequenceInitializer,
	StructNamedInitializer,
	ConstructorInitializer,
	Expression,
	ZeroInitializer,
	UninitializedInitializer >;

using BlockElement= std::variant<
	Block,
	VariablesDeclaration,
	AutoVariableDeclaration,
	ReturnOperator,
	YieldOperator,
	WhileOperator,
	RangeForOperator,
	CStyleForOperator,
	BreakOperator,
	ContinueOperator,
	WithOperator,
	IfOperator,
	StaticIfOperator,
	IfCoroAdvanceOperator,
	SingleExpressionOperator,
	AssignmentOperator,
	AdditiveAssignmentOperator,
	IncrementOperator,
	DecrementOperator,
	StaticAssert,
	TypeAlias,
	Halt,
	HaltIf
>;

using BlockElements= std::vector<BlockElement>;

using ClassElement= std::variant<
	VariablesDeclaration,
	AutoVariableDeclaration,
	StaticAssert,
	TypeAlias,
	Enum,
	FunctionPtr,
	ClassField,
	ClassVisibilityLabel,
	ClassPtr,
	TypeTemplate,
	FunctionTemplate >;

using ClassElements= std::vector<ClassElement>;

using ProgramElement= std::variant<
	VariablesDeclaration,
	AutoVariableDeclaration,
	StaticAssert,
	TypeAlias,
	Enum,
	FunctionPtr,
	ClassPtr,
	TypeTemplate,
	FunctionTemplate,
	NamespacePtr >;

using ProgramElements= std::vector<ProgramElement>;

struct NonSyncTagNone{};
struct NonSyncTagTrue{};
using NonSyncTag= std::variant<NonSyncTagNone, NonSyncTagTrue, ExpressionPtr>;

struct SyntaxElementBase
{
	explicit SyntaxElementBase( const SrcLoc& src_loc );
	// WARNING! This struct have NO virtual destructor for, size optimization.
	// Do not like this:  SyntaxElementBase* x= new Derived();

	SrcLoc src_loc_;
};

enum class MutabilityModifier : uint8_t
{
	None,
	Mutable,
	Immutable,
	Constexpr,
};

enum class ReferenceModifier : uint8_t
{
	None,
	Reference,
	// SPRACE_TODO - add "move" references here
};

struct TypeofTypeName
{
	ExpressionPtr expression;
};

struct ComplexName final : public SyntaxElementBase
{
	explicit ComplexName( const SrcLoc& src_loc );

	std::variant<
		EmptyVariant, // ::
		TypeofTypeName, // typeof(x)
		std::string // name
		> start_value;

	struct Component
	{
		std::variant<
			std::string,
			std::vector<Expression>
			> name_or_template_paramenters;
		std::unique_ptr<const Component> next;
	};

	std::unique_ptr<const Component> tail;
};

struct ArrayTypeName final : public SyntaxElementBase
{
	explicit ArrayTypeName( const SrcLoc& src_loc );

	TypeNamePtr element_type;
	ExpressionPtr size;
};

struct TupleType final : public SyntaxElementBase
{
	TupleType( const SrcLoc& src_loc );

	std::vector<TypeName> element_types_;
};

struct RawPointerType final : public SyntaxElementBase
{
	RawPointerType( const SrcLoc& src_loc );

	TypeNamePtr element_type;
};

struct GeneratorType final : public SyntaxElementBase
{
public:
	GeneratorType( const SrcLoc& src_loc );

	struct InnerReferenceTag
	{
		std::string name;
		MutabilityModifier mutability_modifier= MutabilityModifier::None;
	};

public:
	std::optional<MutabilityModifier> inner_reference_mutability_modifier;
	NonSyncTag non_sync_tag;
	TypeName return_type;
	std::unique_ptr<const InnerReferenceTag> inner_reference_tag; // Make array when multiple inner reference tags will be implemented.
	std::string return_value_reference_tag; // Inner tag for values, reference tag for references.
	MutabilityModifier return_value_mutability_modifier= MutabilityModifier::None;
	ReferenceModifier return_value_reference_modifier= ReferenceModifier::None;
};

using FunctionReferencesPollution= std::pair< std::string, std::string >;
using FunctionReferencesPollutionList= std::vector<FunctionReferencesPollution>;

struct FunctionParam;
using FunctionParams= std::vector<FunctionParam>;

struct FunctionType final : public SyntaxElementBase
{
	FunctionType( const SrcLoc& src_loc );

	std::optional<std::string> calling_convention_;
	TypeNamePtr return_type_;
	std::string return_value_reference_tag_;
	FunctionReferencesPollutionList references_pollution_list_;
	FunctionParams params_;
	std::string return_value_inner_reference_tag_;

	MutabilityModifier return_value_mutability_modifier_= MutabilityModifier::None;
	ReferenceModifier return_value_reference_modifier_= ReferenceModifier::None;
	bool unsafe_= false;
};

struct FunctionParam final : public SyntaxElementBase
{
	FunctionParam( const SrcLoc& src_loc );

	std::string name_;
	TypeName type_;
	std::string reference_tag_;
	std::string inner_arg_reference_tag_;
	MutabilityModifier mutability_modifier_= MutabilityModifier::None;
	ReferenceModifier reference_modifier_= ReferenceModifier::None;
};

struct BinaryOperator final : public SyntaxElementBase
{
	explicit BinaryOperator( const SrcLoc& src_loc );

	BinaryOperatorType operator_type_;
	ExpressionPtr left_;
	ExpressionPtr right_;
};

struct TernaryOperator final : public SyntaxElementBase
{
	explicit TernaryOperator( const SrcLoc& src_loc );

	ExpressionPtr condition;
	ExpressionPtr true_branch;
	ExpressionPtr false_branch;
};

struct ReferenceToRawPointerOperator final : public SyntaxElementBase
{
	explicit ReferenceToRawPointerOperator( const SrcLoc& src_loc );

	ExpressionPtr expression;
};

struct RawPointerToReferenceOperator final : public SyntaxElementBase
{
	explicit RawPointerToReferenceOperator( const SrcLoc& src_loc );

	ExpressionPtr expression;
};

struct MoveOperator final : public SyntaxElementBase
{
	MoveOperator( const SrcLoc& src_loc );

	std::string var_name_;
};

struct TakeOperator final : public SyntaxElementBase
{
	TakeOperator( const SrcLoc& src_loc );

	ExpressionPtr expression_;
};

struct CastRef final : public SyntaxElementBase
{
	CastRef( const SrcLoc& src_loc );

	TypeNamePtr type_;
	ExpressionPtr expression_;
};

struct CastRefUnsafe : public SyntaxElementBase
{
	CastRefUnsafe( const SrcLoc& src_loc );

	TypeNamePtr type_;
	ExpressionPtr expression_;
};

struct CastImut final : public SyntaxElementBase
{
	CastImut( const SrcLoc& src_loc );

	ExpressionPtr expression_;
};

struct CastMut final : public SyntaxElementBase
{
	CastMut( const SrcLoc& src_loc );

	ExpressionPtr expression_;
};

struct TypeInfo final : public SyntaxElementBase
{
	TypeInfo( const SrcLoc& src_loc );

	TypeNamePtr type_;
};

struct NonSyncExpression final : public SyntaxElementBase
{
	NonSyncExpression( const SrcLoc& src_loc );

	TypeNamePtr type_;
};

struct SafeExpression final : public SyntaxElementBase
{
	SafeExpression( const SrcLoc& src_loc );

	ExpressionPtr expression_;
};

struct UnsafeExpression final : public SyntaxElementBase
{
	UnsafeExpression( const SrcLoc& src_loc );

	ExpressionPtr expression_;
};

struct BooleanConstant final : public SyntaxElementBase
{
	BooleanConstant( const SrcLoc& src_loc, bool value );

	bool value_;
};

using TypeSuffix= std::array<char, 7>;

struct NumericConstant final : public SyntaxElementBase, public NumberLexemData
{
	NumericConstant( const SrcLoc& src_loc );
};

struct StringLiteral final : public SyntaxElementBase
{
	StringLiteral( const SrcLoc& src_loc );

	std::string value_;
	TypeSuffix type_suffix_;
};

struct UnaryPlus final : public SyntaxElementBase
{
	explicit UnaryPlus( const SrcLoc& src_loc );

	ExpressionPtr expression_;
};

struct UnaryMinus final : public SyntaxElementBase
{
	explicit UnaryMinus( const SrcLoc& src_loc );

	ExpressionPtr expression_;
};

struct LogicalNot final : public SyntaxElementBase
{
	explicit LogicalNot( const SrcLoc& src_loc );

	ExpressionPtr expression_;
};

struct BitwiseNot final : public SyntaxElementBase
{
	explicit BitwiseNot( const SrcLoc& src_loc );

	ExpressionPtr expression_;
};

struct CallOperator final : public SyntaxElementBase
{
	CallOperator( const SrcLoc& src_loc );

	ExpressionPtr expression_;
	std::vector<Expression> arguments_;
};

struct IndexationOperator final : public SyntaxElementBase
{
	explicit IndexationOperator( const SrcLoc& src_loc );

	ExpressionPtr expression_;
	ExpressionPtr index_;
};

struct MemberAccessOperator final : public SyntaxElementBase
{
	MemberAccessOperator( const SrcLoc& src_loc );

	ExpressionPtr expression_;
	std::string member_name_;
	std::optional<std::vector<Expression>> template_parameters;
};

struct SequenceInitializer final : public SyntaxElementBase
{
	explicit SequenceInitializer( const SrcLoc& src_loc );

	std::vector<Initializer> initializers;
};

struct StructNamedInitializer final : public SyntaxElementBase
{
	explicit StructNamedInitializer( const SrcLoc& src_loc );

	struct MemberInitializer;

	std::vector<MemberInitializer> members_initializers;
};

struct ConstructorInitializer final : public SyntaxElementBase
{
	ConstructorInitializer( const SrcLoc& src_loc );

	std::vector<Expression> arguments;
};

struct ZeroInitializer final : public SyntaxElementBase
{
	explicit ZeroInitializer( const SrcLoc& src_loc );
};

struct UninitializedInitializer final : public SyntaxElementBase
{
	explicit UninitializedInitializer( const SrcLoc& src_loc );
};

struct StructNamedInitializer::MemberInitializer
{
	std::string name;
	Initializer initializer;
};

struct Block final : public SyntaxElementBase
{
	Block( const SrcLoc& start_src_loc );

	enum class Safety : uint8_t
	{
		None,
		Safe,
		Unsafe,
	};

	SrcLoc end_src_loc_;
	BlockElements elements_;
	Safety safety_= Safety::None;
};

struct VariablesDeclaration final : public SyntaxElementBase
{
	VariablesDeclaration( const SrcLoc& src_loc );

	struct VariableEntry
	{
		SrcLoc src_loc;
		std::string name;
		std::unique_ptr<const Initializer> initializer; // May be null for types with default constructor.
		MutabilityModifier mutability_modifier= MutabilityModifier::None;
		ReferenceModifier reference_modifier= ReferenceModifier::None;
	};

	std::vector<VariableEntry> variables;
	TypeName type;
};

struct AutoVariableDeclaration final : public SyntaxElementBase
{
	explicit AutoVariableDeclaration( const SrcLoc& src_loc );

	std::string name;
	Expression initializer_expression;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
};

struct ReturnOperator final : public SyntaxElementBase
{
	ReturnOperator( const SrcLoc& src_loc );

	Expression expression_;
};

struct YieldOperator final : public SyntaxElementBase
{
	YieldOperator( const SrcLoc& src_loc );

	Expression expression;
};

struct Label final : public SyntaxElementBase
{
	Label( const SrcLoc& src_loc );

	std::string name;
};

struct WhileOperator final : public SyntaxElementBase
{
	WhileOperator( const SrcLoc& src_loc );

	Expression condition_;
	std::optional<Label> label_;
	BlockPtr block_;
};

struct RangeForOperator final : public SyntaxElementBase
{
	RangeForOperator( const SrcLoc& src_loc );

	ReferenceModifier reference_modifier_= ReferenceModifier::None;
	MutabilityModifier mutability_modifier_= MutabilityModifier::None;
	std::string loop_variable_name_;
	Expression sequence_;
	std::optional<Label> label_;
	BlockPtr block_;
};

struct CStyleForOperator final : public SyntaxElementBase
{
	CStyleForOperator( const SrcLoc& src_loc );

	std::unique_ptr<
		const std::variant<
			VariablesDeclaration,
			AutoVariableDeclaration > >
	variable_declaration_part_;

	Expression loop_condition_;

	std::vector<
		std::variant<
			SingleExpressionOperator,
			AssignmentOperator,
			AdditiveAssignmentOperator,
			IncrementOperator,
			DecrementOperator > >
	iteration_part_elements_;

	std::optional<Label> label_;
	BlockPtr block_;
};

struct BreakOperator final : public SyntaxElementBase
{
	explicit BreakOperator( const SrcLoc& src_loc );

	std::optional<Label> label_;
};

struct ContinueOperator final : public SyntaxElementBase
{
	explicit ContinueOperator( const SrcLoc& src_loc );

	std::optional<Label> label_;
};

struct WithOperator final : public SyntaxElementBase
{
	WithOperator( const SrcLoc& src_loc );

	ReferenceModifier reference_modifier_= ReferenceModifier::None;
	MutabilityModifier mutability_modifier_= MutabilityModifier::None;
	std::string variable_name_;
	Expression expression_;
	Block block_;
};

struct IfOperator final : public SyntaxElementBase
{
	struct Branch
	{
		// Condition - nullptr for last if.
		Expression condition;
		Block block;
	};

	IfOperator( const SrcLoc& start_src_loc );

	std::vector<Branch> branches_; // else if()
	SrcLoc end_src_loc_;
};

struct StaticIfOperator final : public SyntaxElementBase
{
	StaticIfOperator( const SrcLoc& src_loc );

	IfOperator if_operator_;
};

struct IfCoroAdvanceOperator final : public SyntaxElementBase
{
	IfCoroAdvanceOperator( const SrcLoc& src_loc );

	ReferenceModifier reference_modifier= ReferenceModifier::None;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	std::string variable_name;
	Expression expression;
	Block block;
};

struct SingleExpressionOperator final : public SyntaxElementBase
{
	SingleExpressionOperator( const SrcLoc& src_loc );

	Expression expression_;
};

struct AssignmentOperator final : public SyntaxElementBase
{
	AssignmentOperator( const SrcLoc& src_loc );

	Expression l_value_;
	Expression r_value_;
};

struct AdditiveAssignmentOperator final : public SyntaxElementBase
{
	explicit AdditiveAssignmentOperator( const SrcLoc& src_loc );

	BinaryOperatorType additive_operation_;
	Expression l_value_;
	Expression r_value_;
};

struct IncrementOperator final : public SyntaxElementBase
{
	explicit IncrementOperator( const SrcLoc& src_loc );

	Expression expression;
};

struct DecrementOperator final : public SyntaxElementBase
{
	explicit DecrementOperator( const SrcLoc& src_loc );

	Expression expression;
};

struct StaticAssert final : public SyntaxElementBase
{
	explicit StaticAssert( const SrcLoc& src_loc );

	Expression expression;
};

struct Halt final : public SyntaxElementBase

{
	explicit Halt( const SrcLoc& src_loc );
};

struct HaltIf final : public SyntaxElementBase

{
	explicit HaltIf( const SrcLoc& src_loc );

	Expression condition;
};

struct TypeAlias final : public SyntaxElementBase
{
	explicit TypeAlias( const SrcLoc& src_loc );

	std::string name;
	TypeName value;
};

struct Enum final : public SyntaxElementBase
{
	explicit Enum( const SrcLoc& src_loc );

	struct Member
	{
		SrcLoc src_loc;
		std::string name;
	};

	std::string name;
	ComplexName underlaying_type_name;
	std::vector<Member> members;
};

enum class VirtualFunctionKind : uint8_t
{
	None, // Regular, non-virtual
	DeclareVirtual,
	VirtualOverride,
	VirtualFinal,
	VirtualPure,
};

struct Function final : public SyntaxElementBase
{
	Function( const SrcLoc& src_loc );

	enum class BodyKind : uint8_t
	{
		None,
		BodyGenerationRequired,
		BodyGenerationDisabled,
	};

	enum class Kind : uint8_t
	{
		Regular,
		Generator,
	};

	std::vector<std::string> name_; // A, A::B, A::B::C::D, ::A, ::A::B
	Expression condition_;
	FunctionType type_;
	std::unique_ptr<const StructNamedInitializer> constructor_initialization_list_;
	BlockPtr block_;
	OverloadedOperator overloaded_operator_= OverloadedOperator::None;
	VirtualFunctionKind virtual_function_kind_= VirtualFunctionKind::None;
	BodyKind body_kind= BodyKind::None;
	NonSyncTag coroutine_non_sync_tag; // Non-empty for generators
	Kind kind= Kind::Regular;
	bool no_mangle_= false;
	bool is_conversion_constructor_= false;
	bool constexpr_= false;
};

struct ClassField final : public SyntaxElementBase
{
	explicit ClassField( const SrcLoc& src_loc );

	TypeName type;
	std::string name;
	std::unique_ptr<const Initializer> initializer; // May be null.
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
};

enum class ClassKindAttribute : uint8_t
{
	Struct,
	Class,
	Final,
	Polymorph,
	Interface,
	Abstract,
};

enum class ClassMemberVisibility : uint8_t
{
	// Must be ordered from less access to more access.
	Public,
	Protected,
	Private,
};

struct ClassVisibilityLabel final : public SyntaxElementBase
{
	ClassVisibilityLabel( const SrcLoc& src_loc, ClassMemberVisibility visibility );

	const ClassMemberVisibility visibility_;
};


struct Class final : public SyntaxElementBase
{
	explicit Class( const SrcLoc& src_loc );

	ClassElements elements_;
	std::string name_;
	std::vector<ComplexName> parents_;
	ClassKindAttribute kind_attribute_ = ClassKindAttribute::Struct;
	NonSyncTag non_sync_tag_;
	bool keep_fields_order_= false;
};

struct TemplateBase : public SyntaxElementBase
{
	explicit TemplateBase( const SrcLoc& src_loc );

	struct Param
	{
		std::optional<ComplexName> param_type; // For variable params.
		std::string name;
	};

	std::vector<Param> params_;
};

struct TypeTemplate : public TemplateBase
{
	explicit TypeTemplate( const SrcLoc& src_loc );

	// Argument in template signature.
	struct SignatureParam
	{
		Expression name;
		Expression default_value;
	};

	std::vector<SignatureParam> signature_params_;
	std::string name_;

	// Short form means that template argumenst are also signature arguments.
	bool is_short_form_= false;

	std::variant<ClassPtr, std::unique_ptr<const TypeAlias>> something_;
};

struct FunctionTemplate final : public TemplateBase
{
	explicit FunctionTemplate( const SrcLoc& src_loc );

	FunctionPtr function_;
};

struct Namespace final : public SyntaxElementBase
{
	explicit Namespace( const SrcLoc& src_loc );

	std::string name_;
	ProgramElements elements_;
};

struct Import final : public SyntaxElementBase
{
	explicit Import( const SrcLoc& src_loc );

	std::string import_name;
};

// Utility functions for manipulations with variants.

SrcLoc GetExpressionSrcLoc( const Expression& expression );
SrcLoc GetInitializerSrcLoc( const Initializer& initializer );
SrcLoc GetBlockElementSrcLoc( const BlockElement& block_element );

} // namespace Synt

} // namespace U
