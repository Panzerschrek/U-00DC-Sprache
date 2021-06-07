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

struct ArrayTypeName;
struct TypeofTypeName;
struct NamedTypeName;
struct FunctionType;
struct TupleType;
struct RawPointerType;

struct UnaryPlus;
struct UnaryMinus;
struct LogicalNot;
struct BitwiseNot;

struct CallOperator;
struct IndexationOperator;
struct MemberAccessOperator;

struct BinaryOperator;
struct NamedOperand;
struct TernaryOperator;
struct ReferenceToRawPointerOperator;
struct RawPointerToReferenceOperator;
struct TypeNameInExpression;
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

struct ArrayInitializer;
struct StructNamedInitializer;
struct ConstructorInitializer;
struct ExpressionInitializer;
struct ZeroInitializer;
struct UninitializedInitializer;

struct Block;
struct VariablesDeclaration;
struct AutoVariableDeclaration;
struct ReturnOperator;
struct WhileOperator;
struct ForOperator;
struct CStyleForOperator;
struct BreakOperator;
struct ContinueOperator;
struct WithOperator;
struct IfOperator;
struct StaticIfOperator;
struct SingleExpressionOperator;
struct AssignmentOperator;
struct AdditiveAssignmentOperator;
struct IncrementOperator;
struct DecrementOperator;
struct StaticAssert;
struct Halt;
struct HaltIf;

struct Function;
struct Typedef;
struct Enum;
struct Class;
struct ClassField;
struct ClassVisibilityLabel;
struct TypeTemplate;
struct FunctionTemplate;

struct Namespace;

using FunctionTypePtr= std::unique_ptr<FunctionType>;
using BlockPtr= std::unique_ptr<Block>;
using ClassPtr= std::unique_ptr<Class>;
using FunctionPtr= std::unique_ptr<Function>;
using NamespacePtr= std::unique_ptr<Namespace>;

using TypeName= std::variant<
	EmptyVariant,
	ArrayTypeName,
	NamedTypeName,
	FunctionTypePtr,
	TupleType,
	RawPointerType >;

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
	BinaryOperator,
	NamedOperand,
	TernaryOperator,
	ReferenceToRawPointerOperator,
	RawPointerToReferenceOperator,
	TypeNameInExpression,
	NumericConstant,
	BooleanConstant,
	StringLiteral,
	MoveOperator,
	TakeOperator,
	CastMut,
	CastImut,
	CastRef,
	CastRefUnsafe,
	TypeInfo >;

using Initializer= std::variant<
	EmptyVariant,
	ArrayInitializer,
	StructNamedInitializer,
	ConstructorInitializer,
	ExpressionInitializer,
	ZeroInitializer,
	UninitializedInitializer >;

using BlockElement= std::variant<
	Block,
	VariablesDeclaration,
	AutoVariableDeclaration,
	ReturnOperator,
	WhileOperator,
	ForOperator,
	CStyleForOperator,
	BreakOperator,
	ContinueOperator,
	WithOperator,
	IfOperator,
	StaticIfOperator,
	SingleExpressionOperator,
	AssignmentOperator,
	AdditiveAssignmentOperator,
	IncrementOperator,
	DecrementOperator,
	StaticAssert,
	Halt,
	HaltIf
>;

using ClassElement= std::variant<
	VariablesDeclaration,
	AutoVariableDeclaration,
	StaticAssert,
	Typedef,
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
	Typedef,
	Enum,
	FunctionPtr,
	ClassPtr,
	TypeTemplate,
	FunctionTemplate,
	NamespacePtr >;

using ProgramElements= std::vector<ProgramElement>;

struct SyntaxElementBase
{
public:
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

struct TypeofTypeName final : public SyntaxElementBase
{
public:
	explicit TypeofTypeName( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression;
};

struct ComplexName
{
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
		std::unique_ptr<Component> next;
	};

	std::unique_ptr<Component> tail;
};

struct ArrayTypeName final : public SyntaxElementBase
{
public:
	explicit ArrayTypeName( const SrcLoc& src_loc );

	std::unique_ptr<TypeName> element_type;
	std::unique_ptr<Expression> size;
};

struct TupleType final : public SyntaxElementBase
{
public:
	TupleType( const SrcLoc& src_loc );

public:
	std::vector<TypeName> element_types_;
};

struct RawPointerType final : public SyntaxElementBase
{
public:
	RawPointerType( const SrcLoc& src_loc );

	std::unique_ptr<TypeName> element_type;
};

struct NamedTypeName final : public SyntaxElementBase
{
public:
	explicit NamedTypeName( const SrcLoc& src_loc );

	ComplexName name;
};

using FunctionReferencesPollution= std::pair< std::string, std::string >;
using FunctionReferencesPollutionList= std::vector<FunctionReferencesPollution>;

struct FunctionArgument;
using FunctionArgumentsDeclaration= std::vector<FunctionArgument>;

struct FunctionType final : public SyntaxElementBase
{
public:
	FunctionType( const SrcLoc& src_loc );

	std::unique_ptr<TypeName> return_type_;
	std::string return_value_reference_tag_;
	FunctionReferencesPollutionList referecnces_pollution_list_;
	FunctionArgumentsDeclaration arguments_;
	std::string return_value_inner_reference_tag_;

	MutabilityModifier return_value_mutability_modifier_= MutabilityModifier::None;
	ReferenceModifier return_value_reference_modifier_= ReferenceModifier::None;
	bool unsafe_= false;
};

struct FunctionArgument final : public SyntaxElementBase
{
public:
	FunctionArgument( const SrcLoc& src_loc );

public:
	std::string name_;
	TypeName type_;
	std::string reference_tag_;
	std::string inner_arg_reference_tag_;
	MutabilityModifier mutability_modifier_= MutabilityModifier::None;
	ReferenceModifier reference_modifier_= ReferenceModifier::None;
};

struct BinaryOperator final : public SyntaxElementBase
{
public:
	explicit BinaryOperator( const SrcLoc& src_loc );

	BinaryOperatorType operator_type_;
	std::unique_ptr<Expression> left_;
	std::unique_ptr<Expression> right_;
};

struct TernaryOperator final : public SyntaxElementBase
{
public:
	explicit TernaryOperator( const SrcLoc& src_loc );

	std::unique_ptr<Expression> condition;
	std::unique_ptr<Expression> true_branch;
	std::unique_ptr<Expression> false_branch;
};

struct ReferenceToRawPointerOperator final : public SyntaxElementBase
{
public:
	explicit ReferenceToRawPointerOperator( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression;
};

struct RawPointerToReferenceOperator final : public SyntaxElementBase
{
public:
	explicit RawPointerToReferenceOperator( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression;
};

struct NamedOperand final : public SyntaxElementBase
{
public:
	NamedOperand( const SrcLoc& src_loc, ComplexName name );

	ComplexName name_;
};

struct MoveOperator final : public SyntaxElementBase
{
public:
	MoveOperator( const SrcLoc& src_loc );

	std::string var_name_;
};

struct TakeOperator final : public SyntaxElementBase
{
public:
	TakeOperator( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression_;
};

struct CastRef final : public SyntaxElementBase
{
public:
	CastRef( const SrcLoc& src_loc );

	std::unique_ptr<TypeName> type_;
	std::unique_ptr<Expression> expression_;
};

struct CastRefUnsafe : public SyntaxElementBase
{
public:
	CastRefUnsafe( const SrcLoc& src_loc );

	std::unique_ptr<TypeName> type_;
	std::unique_ptr<Expression> expression_;
};

struct CastImut final : public SyntaxElementBase
{
public:
	CastImut( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression_;
};

struct CastMut final : public SyntaxElementBase
{
public:
	CastMut( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression_;
};

struct TypeInfo final : public SyntaxElementBase
{
public:
	TypeInfo( const SrcLoc& src_loc );

	std::unique_ptr<TypeName> type_;
};

struct BooleanConstant final : public SyntaxElementBase
{
public:
	BooleanConstant( const SrcLoc& src_loc, bool value );

	bool value_;
};

using TypeSuffix= std::array<char, 7>;

struct NumericConstant final : public SyntaxElementBase, public NumberLexemData
{
public:
	NumericConstant( const SrcLoc& src_loc );
};

struct StringLiteral final : public SyntaxElementBase
{
public:
	StringLiteral( const SrcLoc& src_loc );

	std::string value_;
	TypeSuffix type_suffix_;
};

struct TypeNameInExpression : public SyntaxElementBase
{
public:
	explicit TypeNameInExpression( const SrcLoc& src_loc );

	TypeName type_name;
};

struct UnaryPlus final : public SyntaxElementBase
{
public:
	explicit UnaryPlus( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression_;
};

struct UnaryMinus final : public SyntaxElementBase
{
public:
	explicit UnaryMinus( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression_;
};

struct LogicalNot final : public SyntaxElementBase
{
public:
	explicit LogicalNot( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression_;
};

struct BitwiseNot final : public SyntaxElementBase
{
public:
	explicit BitwiseNot( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression_;
};

struct CallOperator final : public SyntaxElementBase
{
public:
	CallOperator( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression_;
	std::vector<Expression> arguments_;
};

struct IndexationOperator final : public SyntaxElementBase
{
public:
	explicit IndexationOperator( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression_;
	std::unique_ptr<Expression> index_;
};

struct MemberAccessOperator final : public SyntaxElementBase
{
public:
	MemberAccessOperator( const SrcLoc& src_loc );

	std::unique_ptr<Expression> expression_;
	std::string member_name_;
	std::vector<Expression> template_parameters;
	bool have_template_parameters= false;
};

struct ArrayInitializer final : public SyntaxElementBase
{
public:
	explicit ArrayInitializer( const SrcLoc& src_loc );

	std::vector<Initializer> initializers;
	bool has_continious_initializer= false; // ... after last initializator.
};

struct StructNamedInitializer final : public SyntaxElementBase
{
public:
	explicit StructNamedInitializer( const SrcLoc& src_loc );

	struct MemberInitializer;

	std::vector<MemberInitializer> members_initializers;
};

struct ConstructorInitializer final : public SyntaxElementBase
{
public:
	ConstructorInitializer( const SrcLoc& src_loc );

	CallOperator call_operator;
};

struct ExpressionInitializer final : public SyntaxElementBase
{
public:
	ExpressionInitializer( const SrcLoc& src_loc );

	Expression expression;
};

struct ZeroInitializer final : public SyntaxElementBase
{
public:
	explicit ZeroInitializer( const SrcLoc& src_loc );
};

struct UninitializedInitializer final : public SyntaxElementBase
{
public:
	explicit UninitializedInitializer( const SrcLoc& src_loc );
};

struct StructNamedInitializer::MemberInitializer
{
	std::string name;
	Initializer initializer;
};

struct Block final : public SyntaxElementBase
{
public:
	Block( const SrcLoc& start_src_loc );

	enum class Safety : uint8_t
	{
		None,
		Safe,
		Unsafe,
	};
public:
	SrcLoc end_src_loc_;
	std::vector<BlockElement> elements_;
	Safety safety_= Safety::None;
};

struct VariablesDeclaration final : public SyntaxElementBase
{
	VariablesDeclaration( const SrcLoc& src_loc );

	struct VariableEntry
	{
		SrcLoc src_loc;
		std::string name;
		std::unique_ptr<Initializer> initializer; // May be null for types with default constructor.
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
public:
	ReturnOperator( const SrcLoc& src_loc );

	Expression expression_;
};

struct WhileOperator final : public SyntaxElementBase
{
public:
	WhileOperator( const SrcLoc& src_loc );

	Expression condition_;
	Block block_;
};

struct ForOperator final : public SyntaxElementBase
{
public:
	ForOperator( const SrcLoc& src_loc );

	ReferenceModifier reference_modifier_= ReferenceModifier::None;
	MutabilityModifier mutability_modifier_= MutabilityModifier::None;
	std::string loop_variable_name_;
	Expression sequence_;
	Block block_;
};

struct CStyleForOperator final : public SyntaxElementBase
{
public:
	CStyleForOperator( const SrcLoc& src_loc );

	std::unique_ptr<
		std::variant<
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

	Block block_;
};

struct BreakOperator final : public SyntaxElementBase
{
public:
	explicit BreakOperator( const SrcLoc& src_loc );
};

struct ContinueOperator final : public SyntaxElementBase
{
public:
	explicit ContinueOperator( const SrcLoc& src_loc );
};

struct WithOperator final : public SyntaxElementBase
{
public:
	WithOperator( const SrcLoc& src_loc );

	ReferenceModifier reference_modifier_= ReferenceModifier::None;
	MutabilityModifier mutability_modifier_= MutabilityModifier::None;
	std::string variable_name_;
	Expression expression_;
	Block block_;
};

struct IfOperator final : public SyntaxElementBase
{
public:
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
public:
	StaticIfOperator( const SrcLoc& src_loc );

	IfOperator if_operator_;
};

struct SingleExpressionOperator final : public SyntaxElementBase
{
public:
	SingleExpressionOperator( const SrcLoc& src_loc );

	Expression expression_;
};

struct AssignmentOperator final : public SyntaxElementBase
{
public:
	AssignmentOperator( const SrcLoc& src_loc );

	Expression l_value_;
	Expression r_value_;
};

struct AdditiveAssignmentOperator final : public SyntaxElementBase
{
public:
	explicit AdditiveAssignmentOperator( const SrcLoc& src_loc );

	BinaryOperatorType additive_operation_;
	Expression l_value_;
	Expression r_value_;
};

struct IncrementOperator final : public SyntaxElementBase
{
public:
	explicit IncrementOperator( const SrcLoc& src_loc );

	Expression expression;
};

struct DecrementOperator final : public SyntaxElementBase
{
public:
	explicit DecrementOperator( const SrcLoc& src_loc );

	Expression expression;
};

struct StaticAssert final : public SyntaxElementBase
{
public:
	explicit StaticAssert( const SrcLoc& src_loc );

	Expression expression;
};

struct Halt final
	: public SyntaxElementBase

{
public:
	explicit Halt( const SrcLoc& src_loc );
};

struct HaltIf final
	: public SyntaxElementBase

{
public:
	explicit HaltIf( const SrcLoc& src_loc );

	Expression condition;
};

struct Typedef final : public SyntaxElementBase
{
public:
	explicit Typedef( const SrcLoc& src_loc );

	std::string name;
	TypeName value;
};

struct Enum final : public SyntaxElementBase
{
public:
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
public:
	Function( const SrcLoc& src_loc );

	enum class BodyKind : uint8_t
	{
		None,
		BodyGenerationRequired,
		BodyGenerationDisabled,
	};

	std::vector<std::string> name_; // A, A::B, A::B::C::D, ::A, ::A::B
	Expression condition_;
	FunctionType type_;
	std::unique_ptr<StructNamedInitializer> constructor_initialization_list_;
	BlockPtr block_;
	OverloadedOperator overloaded_operator_= OverloadedOperator::None;
	VirtualFunctionKind virtual_function_kind_= VirtualFunctionKind::None;
	BodyKind body_kind= BodyKind::None;
	bool no_mangle_= false;
	bool is_conversion_constructor_= false;
	bool constexpr_= false;
	bool is_template_= false;
};

struct ClassField final : public SyntaxElementBase
{
public:
	explicit ClassField( const SrcLoc& src_loc );

	TypeName type;
	std::string name;
	std::unique_ptr<Initializer> initializer; // May be null.
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
public:
	ClassVisibilityLabel( const SrcLoc& src_loc, ClassMemberVisibility visibility );

	const ClassMemberVisibility visibility_;
};

struct Class final : public SyntaxElementBase
{
public:
	explicit Class( const SrcLoc& src_loc );

	ClassElements elements_;
	std::string name_;
	std::vector<ComplexName> parents_;
	ClassKindAttribute kind_attribute_ = ClassKindAttribute::Struct;
	bool is_forward_declaration_= false;
	bool have_shared_state_= false;
	bool keep_fields_order_= false;
};

struct TemplateBase : public SyntaxElementBase
{
public:
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
public:
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

	std::variant<ClassPtr, std::unique_ptr<Typedef>> something_;
};

struct FunctionTemplate final : public TemplateBase
{
public:
	explicit FunctionTemplate( const SrcLoc& src_loc );

	FunctionPtr function_;
};

struct Namespace final : public SyntaxElementBase
{
public:
	explicit Namespace( const SrcLoc& src_loc );

	std::string name_;
	ProgramElements elements_;
};

struct Import final : public SyntaxElementBase
{
public:
	explicit Import( const SrcLoc& src_loc );

	std::string import_name;
};

// Utility functions for manipulations with variants.

SrcLoc GetExpressionSrcLoc( const Expression& expression );
SrcLoc GetInitializerSrcLoc( const Initializer& initializer );
SrcLoc GetBlockElementSrcLoc( const BlockElement& block_element );

} // namespace Synt

} // namespace U
