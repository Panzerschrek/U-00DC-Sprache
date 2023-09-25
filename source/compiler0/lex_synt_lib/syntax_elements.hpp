#pragma once
#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "lexical_analyzer.hpp"
#include "operators.hpp"
#include "variant_linked_list.hpp"

namespace U
{

namespace Synt
{

/*
	A note about variant usage.
	It is used widely for some structures, like Expressions.
	But it is imprortant to use it wisely, in order to reduce total structs size and number of indirections.
	It is fine to store terminal nodes directly (like number, boolean constant), sine such nodes are small.
	Recursive nodes (like binary operators) should be stored in variant via pointer (unique_ptr), since indirection already required.
	Doing so, instead of storing such nodes by-value and storing pointers inside them allows to reduce size of variant and reduce number of allocations.
	Exception - node with single "vector" inside and (maybe) a little bit of extra data, that doesn't increase result variant size.
*/

struct EmptyVariant{};

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
struct CallOperatorSignatureHelp;
struct IndexationOperator;
struct MemberAccessOperator;
struct MemberAccessOperatorCompletion;

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
struct SameType;
struct NonSyncExpression;
struct SafeExpression;
struct UnsafeExpression;

struct SequenceInitializer;
struct StructNamedInitializer;
struct ConstructorInitializer;
struct ConstructorInitializerSignatureHelp;
struct ZeroInitializer;
struct UninitializedInitializer;

struct Block;
struct ScopeBlock;
struct VariablesDeclaration;
struct AutoVariableDeclaration;
struct ReturnOperator;
struct YieldOperator;
struct WhileOperator;
struct LoopOperator;
struct RangeForOperator;
struct CStyleForOperator;
struct BreakOperator;
struct ContinueOperator;
struct WithOperator;
struct IfOperator;
struct StaticIfOperator;
struct IfCoroAdvanceOperator;
struct SwitchOperator;
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

struct TypeofTypeName;
struct RootNamespaceNameLookup;
struct RootNamespaceNameLookupCompletion;
struct NameLookup;
struct NameLookupCompletion;
struct NamesScopeNameFetch;
struct NamesScopeNameFetchCompletion;
struct TemplateParametrization;

using ComplexName= std::variant<
	// Terminal nodes.
	RootNamespaceNameLookup,
	RootNamespaceNameLookupCompletion,
	NameLookup,
	NameLookupCompletion,
	// Non-terminal nodes (that contain ComplexName inside).
	std::unique_ptr<const TypeofTypeName>,
	std::unique_ptr<const NamesScopeNameFetch>,
	std::unique_ptr<const NamesScopeNameFetchCompletion>,
	std::unique_ptr<const TemplateParametrization>
	>;

struct Namespace;

using TypeName= std::variant<
	EmptyVariant,
	// Include all ComplexName variants.
	// Do not store ComplexName itself, since it adds extra size because of nested variants.
	RootNamespaceNameLookup,
	RootNamespaceNameLookupCompletion,
	NameLookup,
	NameLookupCompletion,
	std::unique_ptr<const TypeofTypeName>,
	std::unique_ptr<const NamesScopeNameFetch>,
	std::unique_ptr<const NamesScopeNameFetchCompletion>,
	std::unique_ptr<const TemplateParametrization>,
	// Non-terminals.
	TupleType, // Just vector of contained types
	std::unique_ptr<const RawPointerType>,
	std::unique_ptr<const ArrayTypeName>,
	std::unique_ptr<const FunctionType>,
	std::unique_ptr<const GeneratorType> >;

struct BooleanConstant
{
	BooleanConstant( const SrcLoc& src_loc, bool value );

	SrcLoc src_loc;
	bool value= false;
};

struct NumericConstant : public NumberLexemData
{
	NumericConstant( const SrcLoc& src_loc );

	SrcLoc src_loc;
};

struct StringLiteral
{
	StringLiteral( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string value;
	std::string type_suffix;
};

struct MoveOperator
{
	MoveOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string var_name;
};

struct MoveOperatorCompletion
{
	MoveOperatorCompletion( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string var_name;
};

using Expression= std::variant<
	EmptyVariant,
	// Terminal nodes.
	ComplexName,
	NumericConstant,
	BooleanConstant,
	MoveOperator,
	MoveOperatorCompletion,
	std::unique_ptr<const StringLiteral>, // Terminal, but too heavy, to store by-value.
	// Non-terminal nodes (with Expression or TypeName containing inside).
	std::unique_ptr<const TypeInfo>,
	std::unique_ptr<const SameType>,
	std::unique_ptr<const NonSyncExpression>,
	std::unique_ptr<const CallOperator>,
	std::unique_ptr<const CallOperatorSignatureHelp>,
	std::unique_ptr<const IndexationOperator>,
	std::unique_ptr<const MemberAccessOperator>,
	std::unique_ptr<const MemberAccessOperatorCompletion>,
	std::unique_ptr<const UnaryPlus>,
	std::unique_ptr<const UnaryMinus>,
	std::unique_ptr<const LogicalNot>,
	std::unique_ptr<const BitwiseNot>,
	std::unique_ptr<const SafeExpression>,
	std::unique_ptr<const UnsafeExpression>,
	std::unique_ptr<const BinaryOperator>,
	std::unique_ptr<const TernaryOperator>,
	std::unique_ptr<const ReferenceToRawPointerOperator>,
	std::unique_ptr<const RawPointerToReferenceOperator>,
	std::unique_ptr<const TakeOperator>,
	std::unique_ptr<const CastMut>,
	std::unique_ptr<const CastImut>,
	std::unique_ptr<const CastRef>,
	std::unique_ptr<const CastRefUnsafe>,
	// Type name in expression context.
	TupleType,
	std::unique_ptr<const ArrayTypeName>,
	std::unique_ptr<const FunctionType>,
	std::unique_ptr<const RawPointerType>,
	std::unique_ptr<const GeneratorType>
	>;

using Initializer= std::variant<
	EmptyVariant,
	// Terminals.
	ZeroInitializer,
	UninitializedInitializer,
	// Non-terminals.
	Expression,
	SequenceInitializer,
	StructNamedInitializer,
	ConstructorInitializer,
	ConstructorInitializerSignatureHelp
	>;

//
// Block elements list structures.
// Since size of each block element is so different, allocate each element in its own unique_ptr.
// Since we are already allocating, use linked list (via BlockElementsListNode template) in order to build list, instead of using extra allocation for vector.
//

using BlockElementsList= VariantLinkedList<
	ScopeBlock,
	VariablesDeclaration,
	AutoVariableDeclaration,
	ReturnOperator,
	YieldOperator,
	WhileOperator,
	LoopOperator,
	RangeForOperator,
	CStyleForOperator,
	BreakOperator,
	ContinueOperator,
	WithOperator,
	IfOperator,
	StaticIfOperator,
	IfCoroAdvanceOperator,
	SwitchOperator,
	SingleExpressionOperator,
	AssignmentOperator,
	AdditiveAssignmentOperator,
	IncrementOperator,
	DecrementOperator,
	StaticAssert,
	TypeAlias,
	Halt,
	HaltIf >;

using IfAlternative= std::variant<
	Block,
	IfOperator,
	StaticIfOperator,
	IfCoroAdvanceOperator
	>;

using IfAlternativePtr= std::unique_ptr<const IfAlternative>;

using ClassElementsList= VariantLinkedList<
	VariablesDeclaration,
	AutoVariableDeclaration,
	StaticAssert,
	TypeAlias,
	Enum,
	Function,
	ClassField,
	ClassVisibilityLabel,
	Class,
	TypeTemplate,
	FunctionTemplate >;

using ProgramElementsList= VariantLinkedList<
	VariablesDeclaration,
	AutoVariableDeclaration,
	StaticAssert,
	TypeAlias,
	Enum,
	Function,
	Class,
	TypeTemplate,
	FunctionTemplate,
	Namespace >;

struct NonSyncTagNone{};
struct NonSyncTagTrue{};
using NonSyncTag= std::variant<NonSyncTagNone, NonSyncTagTrue, std::unique_ptr<const Expression>>;

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
};

struct RootNamespaceNameLookup
{
	explicit RootNamespaceNameLookup( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string name;
};

// Variant of name lookup, used internally by language server for completion.
// In normal compilation process it is not used.
struct RootNamespaceNameLookupCompletion
{
	explicit RootNamespaceNameLookupCompletion( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string name;
};

struct NameLookup
{
	explicit NameLookup( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string name;
};

// Variant of name lookup, used internally by language server for completion.
// In normal compilation process it is not used.
struct NameLookupCompletion
{
	explicit NameLookupCompletion( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string name;
};

struct NamesScopeNameFetch
{
	SrcLoc src_loc;
	std::string name;
	ComplexName base;
};

// Variant of name lookup, used internally by language server for completion.
// In normal compilation process it is not used.
struct NamesScopeNameFetchCompletion
{
	SrcLoc src_loc;
	std::string name;
	ComplexName base;
};

struct TemplateParametrization
{
	SrcLoc src_loc;
	std::vector<Expression> template_args;
	ComplexName base;
};

struct TupleType
{
	explicit TupleType( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::vector<TypeName> element_types;
};

struct RawPointerType
{
	RawPointerType( const SrcLoc& src_loc );

	SrcLoc src_loc;
	TypeName element_type;
};

struct GeneratorType
{
public:
	GeneratorType( const SrcLoc& src_loc );

	struct InnerReferenceTag
	{
		std::string name;
		MutabilityModifier mutability_modifier= MutabilityModifier::None;
	};

public:
	SrcLoc src_loc;
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

struct FunctionType
{
	FunctionType( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::optional<std::string> calling_convention;
	std::unique_ptr<const TypeName> return_type;
	std::string return_value_reference_tag;
	FunctionReferencesPollutionList references_pollution_list;
	FunctionParams params;
	std::string return_value_inner_reference_tag;

	MutabilityModifier return_value_mutability_modifier= MutabilityModifier::None;
	ReferenceModifier return_value_reference_modifier= ReferenceModifier::None;
	bool unsafe= false;
};

struct FunctionParam
{
	FunctionParam( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string name;
	TypeName type;
	std::string reference_tag;
	std::string inner_arg_reference_tag;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
};

struct TypeInfo
{
	TypeInfo( const SrcLoc& src_loc );

	SrcLoc src_loc;
	TypeName type;
};

struct SameType
{
	SameType( const SrcLoc& src_loc );

	SrcLoc src_loc;
	TypeName l;
	TypeName r;
};

struct NonSyncExpression
{
	NonSyncExpression( const SrcLoc& src_loc );

	SrcLoc src_loc;
	TypeName type;
};

struct SafeExpression
{
	SafeExpression( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct UnsafeExpression
{
	UnsafeExpression( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct UnaryPlus
{
	explicit UnaryPlus( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct UnaryMinus
{
	explicit UnaryMinus( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct LogicalNot
{
	explicit LogicalNot( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct BitwiseNot
{
	explicit BitwiseNot( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct CallOperator
{
	CallOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
	std::vector<Expression> arguments;
};

// Special kind of call operator, created only by language server to perform signature help.
struct CallOperatorSignatureHelp
{
	CallOperatorSignatureHelp( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
	// For now no need to parse arguments.
};

struct IndexationOperator
{
	explicit IndexationOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
	Expression index;
};

struct MemberAccessOperator
{
	MemberAccessOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
	std::string member_name;
	std::optional<std::vector<Expression>> template_parameters;
};

// Variant of member access, used internally by language server for completion.
// In normal compilation process it is not used.
struct MemberAccessOperatorCompletion
{
	MemberAccessOperatorCompletion( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
	std::string member_name;
};

struct BinaryOperator
{
	explicit BinaryOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	BinaryOperatorType operator_type;
	Expression left;
	Expression right;
};

struct TernaryOperator
{
	explicit TernaryOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression condition;
	Expression true_branch;
	Expression false_branch;
};

struct ReferenceToRawPointerOperator
{
	explicit ReferenceToRawPointerOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct RawPointerToReferenceOperator
{
	explicit RawPointerToReferenceOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct TakeOperator
{
	TakeOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct CastRef
{
	CastRef( const SrcLoc& src_loc );

	SrcLoc src_loc;
	TypeName type;
	Expression expression;
};

struct CastRefUnsafe
{
	CastRefUnsafe( const SrcLoc& src_loc );

	SrcLoc src_loc;
	TypeName type;
	Expression expression;
};

struct CastImut
{
	CastImut( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct CastMut
{
	CastMut( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct ArrayTypeName
{
	explicit ArrayTypeName( const SrcLoc& src_loc );

	SrcLoc src_loc;
	TypeName element_type;
	Expression size;
};

struct TypeofTypeName
{
	explicit TypeofTypeName( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct SequenceInitializer
{
	explicit SequenceInitializer( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::vector<Initializer> initializers;
};

struct StructNamedInitializer
{
	explicit StructNamedInitializer( const SrcLoc& src_loc );

	struct MemberInitializer;

	SrcLoc src_loc;
	std::vector<MemberInitializer> members_initializers;
};

struct ConstructorInitializer
{
	ConstructorInitializer( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::vector<Expression> arguments;
};

struct ConstructorInitializerSignatureHelp
{
	ConstructorInitializerSignatureHelp( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::vector<Expression> arguments;
};

struct ZeroInitializer
{
	explicit ZeroInitializer( const SrcLoc& src_loc );

	SrcLoc src_loc;
};

struct UninitializedInitializer
{
	explicit UninitializedInitializer( const SrcLoc& src_loc );

	SrcLoc src_loc;
};

struct StructNamedInitializer::MemberInitializer
{
	SrcLoc src_loc;
	std::string name;
	Initializer initializer;
	bool completion_requested= false;
};

struct Label
{
	Label( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string name;
};

// Just block - as it used as part of other syntax elements.
struct Block
{
	Block( const SrcLoc& start_src_loc );

	SrcLoc src_loc;
	SrcLoc end_src_loc;
	BlockElementsList elements;
};

// Block inside scope - with additional properties.
struct ScopeBlock final : public Block
{
	ScopeBlock( Block block );

	enum class Safety : uint8_t
	{
		None,
		Safe,
		Unsafe,
	};

	SrcLoc src_loc;
	Safety safety= Safety::None;
	std::optional<Label> label;
};

struct VariablesDeclaration
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

	SrcLoc src_loc;
	TypeName type;
	std::vector<VariableEntry> variables;
};

struct AutoVariableDeclaration
{
	explicit AutoVariableDeclaration( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string name;
	Expression initializer_expression;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
};

struct ReturnOperator
{
	ReturnOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct YieldOperator
{
	YieldOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct WhileOperator
{
	WhileOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression condition;
	std::optional<Label> label;
	Block block;
};

struct LoopOperator
{
	LoopOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::optional<Label> label;
	Block block;
};

struct RangeForOperator
{
	RangeForOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	std::string loop_variable_name;
	Expression sequence;
	std::optional<Label> label;
	Block block;
};

struct CStyleForOperator
{
	CStyleForOperator( const SrcLoc& src_loc );

	std::unique_ptr<
		const std::variant<
			VariablesDeclaration,
			AutoVariableDeclaration > >
	variable_declaration_part;

	Expression loop_condition;

	std::vector<
		std::variant<
			SingleExpressionOperator,
			AssignmentOperator,
			AdditiveAssignmentOperator,
			IncrementOperator,
			DecrementOperator > >
	iteration_part_elements;

	SrcLoc src_loc;
	std::optional<Label> label;
	Block block;
};

struct BreakOperator
{
	explicit BreakOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::optional<Label> label;
};

struct ContinueOperator
{
	explicit ContinueOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::optional<Label> label;
};

struct WithOperator
{
	WithOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	std::string variable_name;
	Expression expression;
	Block block;
};

struct IfOperator
{
	IfOperator( const SrcLoc& start_src_loc );

	SrcLoc src_loc;
	Expression condition;
	Block block;
	IfAlternativePtr alternative; // non-null if "else" branch exists.
	SrcLoc end_src_loc;
};

struct StaticIfOperator
{
	StaticIfOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression condition;
	Block block;
	IfAlternativePtr alternative; // non-null if "else" branch exists.
};

struct IfCoroAdvanceOperator
{
	IfCoroAdvanceOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	std::string variable_name;
	Expression expression;
	Block block;
	IfAlternativePtr alternative;
	SrcLoc end_src_loc;
};

struct SwitchOperator
{
	SwitchOperator( const SrcLoc& src_loc );

	struct DefaultPlaceholder{};
	struct CaseRange
	{
		// EmptyVariant if has no value.
		Expression low;
		Expression high;
	};

	using CaseValue= std::variant<Expression, CaseRange>; // Range or single value.

	// List of values/ranges or single "default".
	// There is no reason to combine "default" branch with other values/ranges, since such values are unnecessary (default will handle them all).
	using CaseValues= std::variant<std::vector<CaseValue>, DefaultPlaceholder>;

	struct Case
	{
		CaseValues values;
		Block block;
	};

	SrcLoc src_loc;
	Expression value;
	std::vector<Case> cases;
	SrcLoc end_src_loc;
};

struct SingleExpressionOperator
{
	SingleExpressionOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct AssignmentOperator
{
	AssignmentOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression l_value;
	Expression r_value;
};

struct AdditiveAssignmentOperator
{
	explicit AdditiveAssignmentOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	BinaryOperatorType additive_operation;
	Expression l_value;
	Expression r_value;
};

struct IncrementOperator
{
	explicit IncrementOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct DecrementOperator
{
	explicit DecrementOperator( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
};

struct StaticAssert
{
	explicit StaticAssert( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression expression;
	std::optional<std::string> message;
};

struct Halt
{
	explicit Halt( const SrcLoc& src_loc );

	SrcLoc src_loc;
};

struct HaltIf
{
	explicit HaltIf( const SrcLoc& src_loc );

	SrcLoc src_loc;
	Expression condition;
};

struct TypeAlias
{
	explicit TypeAlias( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string name;
	TypeName value;
};

struct Enum
{
	explicit Enum( const SrcLoc& src_loc );

	struct Member
	{
		SrcLoc src_loc;
		std::string name;
	};

	SrcLoc src_loc;
	std::string name;
	std::optional<ComplexName> underlaying_type_name;
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

struct Function
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

	struct NameComponent
	{
		std::string name;
		SrcLoc src_loc;
		bool completion_requested= false;
	};

	SrcLoc src_loc;
	std::vector<NameComponent> name; // A, A::B, A::B::C::D, ::A, ::A::B
	Expression condition;
	FunctionType type;
	std::unique_ptr<const StructNamedInitializer> constructor_initialization_list;
	std::unique_ptr<const Block> block;
	OverloadedOperator overloaded_operator= OverloadedOperator::None;
	VirtualFunctionKind virtual_function_kind= VirtualFunctionKind::None;
	BodyKind body_kind= BodyKind::None;
	NonSyncTag coroutine_non_sync_tag; // Non-empty for generators
	Kind kind= Kind::Regular;
	bool no_mangle= false;
	bool is_conversion_constructor= false;
	bool constexpr_= false;
};

struct ClassField
{
	explicit ClassField( const SrcLoc& src_loc );

	SrcLoc src_loc;
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

struct ClassVisibilityLabel
{
	ClassVisibilityLabel( const SrcLoc& src_loc, ClassMemberVisibility visibility );

	SrcLoc src_loc;
	const ClassMemberVisibility visibility;
};

struct Class
{
	explicit Class( const SrcLoc& src_loc );

	SrcLoc src_loc;
	ClassElementsList elements;
	std::string name;
	std::vector<ComplexName> parents;
	ClassKindAttribute kind_attribute_ = ClassKindAttribute::Struct;
	NonSyncTag non_sync_tag;
	bool keep_fields_order= false;
};

struct TemplateBase
{
	explicit TemplateBase( const SrcLoc& src_loc );

	struct Param
	{
		SrcLoc src_loc;
		std::optional<ComplexName> param_type; // For variable params.
		std::string name;
	};

	SrcLoc src_loc;
	std::vector<Param> params;
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

	std::vector<SignatureParam> signature_params;
	std::string name;

	// Short form means that template argumenst are also signature arguments.
	bool is_short_form= false;

	std::variant<std::unique_ptr<const Class>, std::unique_ptr<const TypeAlias>> something;
};

struct FunctionTemplate final : public TemplateBase
{
	explicit FunctionTemplate( const SrcLoc& src_loc );

	std::unique_ptr<const Function> function;
};

struct Namespace
{
	explicit Namespace( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string name;
	ProgramElementsList elements;
};

struct Import
{
	explicit Import( const SrcLoc& src_loc );

	SrcLoc src_loc;
	std::string import_name;
};

// Utility functions for manipulations with variants.

SrcLoc GetExpressionSrcLoc( const Expression& expression );
SrcLoc GetComplexNameSrcLoc( const ComplexName& complex_name );
SrcLoc GetInitializerSrcLoc( const Initializer& initializer );

inline TypeName ComplexNameToTypeName( ComplexName n )
{
	return std::visit( []( auto&& el ) { return TypeName(std::move(el)); }, std::move(n) );
}

inline Expression TypeNameToExpression( TypeName t )
{
	return std::visit( []( auto&& el ) { return Expression(std::move(el)); }, std::move(t) );
}

} // namespace Synt

} // namespace U
