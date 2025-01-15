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

//
// Forward declarations.
//

struct EmptyVariant{};

// ComplexName
struct RootNamespaceNameLookup;
struct RootNamespaceNameLookupCompletion;
struct NameLookup;
struct NameLookupCompletion;
struct TypeofTypeName;
struct NamesScopeNameFetch;
struct NamesScopeNameFetchCompletion;
struct TemplateParameterization;

// TypeName
struct TupleType;
struct RawPointerType;
struct ArrayTypeName;
struct FunctionType;
struct CoroutineType;

// Expression
struct NumericConstant;
struct BooleanConstant;
struct MoveOperator;
struct MoveOperatorCompletion;
struct StringLiteral;
struct TypeInfo;
struct SameType;
struct NonSyncExpression;
struct CallOperator;
struct CallOperatorSignatureHelp;
struct IndexationOperator;
struct MemberAccessOperator;
struct MemberAccessOperatorCompletion;
struct VariableInitialization;
struct AwaitOperator;
struct UnaryMinus;
struct LogicalNot;
struct BitwiseNot;
struct SafeExpression;
struct UnsafeExpression;
struct BinaryOperator;
struct TernaryOperator;
struct ReferenceToRawPointerOperator;
struct RawPointerToReferenceOperator;
struct TakeOperator;
struct Lambda;
struct CastMut;
struct CastImut;
struct CastRef;
struct CastRefUnsafe;
struct Embed;

// Initializers
struct ZeroInitializer;
struct UninitializedInitializer;
struct SequenceInitializer;
struct StructNamedInitializer;
struct ConstructorInitializer;
struct ConstructorInitializerSignatureHelp;

// Block elements
struct Block;
struct ScopeBlock;
struct VariablesDeclaration;
struct AutoVariableDeclaration;
struct AllocaDeclaration;
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
struct CompoundAssignmentOperator;
struct IncrementOperator;
struct DecrementOperator;
struct StaticAssert;
struct Halt;
struct HaltIf;

// Class/namespace elements.

struct Function;
struct TypeAlias;
struct Enum;
struct Class;
struct ClassField;
struct ClassVisibilityLabel;
struct TypeTemplate;
struct FunctionTemplate;
struct Namespace;

struct Mixin; // Mixins may be used in different contexts.

//
// Variants
//

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
	std::unique_ptr<const TemplateParameterization>
	>;

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
	std::unique_ptr<const TemplateParameterization>,
	// Non-terminals.
	TupleType, // Just vector of contained types.
	std::unique_ptr<const RawPointerType>,
	std::unique_ptr<const ArrayTypeName>,
	std::unique_ptr<const FunctionType>,
	std::unique_ptr<const CoroutineType>,
	std::unique_ptr<const Mixin>
	>;

using Expression= std::variant<
	EmptyVariant,
	// Terminal nodes.
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
	std::unique_ptr<const VariableInitialization>,
	std::unique_ptr<const AwaitOperator>,
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
	std::unique_ptr<const Lambda>,
	std::unique_ptr<const CastMut>,
	std::unique_ptr<const CastImut>,
	std::unique_ptr<const CastRef>,
	std::unique_ptr<const CastRefUnsafe>,
	std::unique_ptr<const Embed>,
	// Type name in expression context.
	RootNamespaceNameLookup,
	RootNamespaceNameLookupCompletion,
	NameLookup,
	NameLookupCompletion,
	std::unique_ptr<const TypeofTypeName>,
	std::unique_ptr<const NamesScopeNameFetch>,
	std::unique_ptr<const NamesScopeNameFetchCompletion>,
	std::unique_ptr<const TemplateParameterization>,
	TupleType, // Just vector of contained types.
	std::unique_ptr<const RawPointerType>,
	std::unique_ptr<const ArrayTypeName>,
	std::unique_ptr<const FunctionType>,
	std::unique_ptr<const CoroutineType>,
	std::unique_ptr<const Mixin>
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
	AllocaDeclaration,
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
	CompoundAssignmentOperator,
	IncrementOperator,
	DecrementOperator,
	StaticAssert,
	TypeAlias,
	Halt,
	HaltIf,
	Mixin>;

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
	Function,
	TypeAlias,
	Enum,
	Class,
	ClassField,
	ClassVisibilityLabel,
	TypeTemplate,
	FunctionTemplate,
	Mixin >;

using ProgramElementsList= VariantLinkedList<
	VariablesDeclaration,
	AutoVariableDeclaration,
	StaticAssert,
	Function,
	TypeAlias,
	Enum,
	Class,
	TypeTemplate,
	FunctionTemplate,
	Namespace,
	Mixin >;

struct NonSyncTagNone{};
struct NonSyncTagTrue{};
using NonSyncTag= std::variant<NonSyncTagNone, NonSyncTagTrue, std::unique_ptr<const Expression>>;

//
// Enums definitions.
//

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

enum class VirtualFunctionKind : uint8_t
{
	None, // Regular, non-virtual
	DeclareVirtual,
	VirtualOverride,
	VirtualFinal,
	VirtualPure,
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

//
// Struct definitions.
//

struct RootNamespaceNameLookup
{
	explicit RootNamespaceNameLookup( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string name;
};

// Variant of name lookup, used internally by language server for completion.
// In normal compilation process it is not used.
struct RootNamespaceNameLookupCompletion
{
	explicit RootNamespaceNameLookupCompletion( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string name;
};

struct NameLookup
{
	explicit NameLookup( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string name;
};

// Variant of name lookup, used internally by language server for completion.
// In normal compilation process it is not used.
struct NameLookupCompletion
{
	explicit NameLookupCompletion( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string name;
};

struct NumericConstant
{
	NumericConstant( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	NumberLexemData num;
};

struct BooleanConstant
{
	BooleanConstant( const SrcLoc& src_loc, const bool value )
		: src_loc(src_loc), value(value) {}

	SrcLoc src_loc;
	bool value= false;
};

struct MoveOperator
{
	MoveOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string var_name;
};

struct MoveOperatorCompletion
{
	MoveOperatorCompletion( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string var_name;
};

struct StringLiteral
{
	StringLiteral( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string value;
	std::string type_suffix;
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

struct TemplateParameterization
{
	SrcLoc src_loc;
	std::vector<Expression> template_args;
	ComplexName base;
};

struct TupleType
{
	explicit TupleType( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::vector<TypeName> element_types;
};

struct RawPointerType
{
	RawPointerType( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	TypeName element_type;
};

struct ArrayTypeName
{
	explicit ArrayTypeName( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	TypeName element_type;
	Expression size;
};

struct FunctionParam
{
	FunctionParam( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string name;
	TypeName type;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
};

struct FunctionType
{
	FunctionType( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	bool IsAutoReturn() const;

public:
	SrcLoc src_loc;
	std::vector<FunctionParam> params;
	std::optional<std::string> calling_convention;
	std::unique_ptr<const TypeName> return_type;
	std::unique_ptr<const Expression> references_pollution_expression; // May be nullptr.
	std::unique_ptr<const Expression> return_value_reference_expression; // May be nullptr.
	std::unique_ptr<const Expression> return_value_inner_references_expression; // May be nullptr.

	MutabilityModifier return_value_mutability_modifier= MutabilityModifier::None;
	ReferenceModifier return_value_reference_modifier= ReferenceModifier::None;
	bool unsafe= false;
};

struct CoroutineType
{
public:
	enum class Kind : uint8_t
	{
		Generator,
		AsyncFunc,
	};

public:
	CoroutineType( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

public:
	SrcLoc src_loc;
	std::optional<MutabilityModifier> inner_reference_mutability_modifier;
	NonSyncTag non_sync_tag;
	TypeName return_type;
	std::vector<MutabilityModifier> inner_references;
	std::unique_ptr<const Expression> return_value_reference_expression; // May be nullptr.
	std::unique_ptr<const Expression> return_value_inner_references_expression; // May be nullptr.
	Kind kind= Kind::Generator;
	MutabilityModifier return_value_mutability_modifier= MutabilityModifier::None;
	ReferenceModifier return_value_reference_modifier= ReferenceModifier::None;
};

struct TypeofTypeName
{
	explicit TypeofTypeName( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct TypeInfo
{
	explicit TypeInfo( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	TypeName type;
};

struct SameType
{
	explicit SameType( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	TypeName l;
	TypeName r;
};

struct NonSyncExpression
{
	explicit NonSyncExpression( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	TypeName type;
};

struct CallOperator
{
	explicit CallOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
	std::vector<Expression> arguments;
};

// Special kind of call operator, created only by language server to perform signature help.
struct CallOperatorSignatureHelp
{
	explicit CallOperatorSignatureHelp( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
	// For now no need to parse arguments.
};

struct IndexationOperator
{
	explicit IndexationOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
	Expression index;
};

struct MemberAccessOperator
{
	explicit MemberAccessOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
	std::string member_name;

	// Hack! Use combination of vector and bool instead of optional, since it causes nasty compilation bugs on old versions of GCC.
	std::vector<Expression> template_args;
	bool has_template_args= false;
};

struct AwaitOperator
{
	explicit AwaitOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

// Variant of member access, used internally by language server for completion.
// In normal compilation process it is not used.
struct MemberAccessOperatorCompletion
{
	explicit MemberAccessOperatorCompletion( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
	std::string member_name;
};

struct UnaryMinus
{
	explicit UnaryMinus( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct LogicalNot
{
	explicit LogicalNot( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct BitwiseNot
{
	explicit BitwiseNot( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct SafeExpression
{
	explicit SafeExpression( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct UnsafeExpression
{
	explicit UnsafeExpression( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct BinaryOperator
{
	explicit BinaryOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	BinaryOperatorType operator_type;
	Expression left;
	Expression right;
};

struct TernaryOperator
{
	explicit TernaryOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression condition;
	Expression branches[2];
};

struct ReferenceToRawPointerOperator
{
	explicit ReferenceToRawPointerOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct RawPointerToReferenceOperator
{
	explicit RawPointerToReferenceOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct TakeOperator
{
	explicit TakeOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct CastMut
{
	explicit CastMut( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct CastImut
{
	explicit CastImut( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct CastRef
{
	explicit CastRef( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	TypeName type;
	Expression expression;
};

struct CastRefUnsafe
{
	explicit CastRefUnsafe( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	TypeName type;
	Expression expression;
};

struct Embed
{
	explicit Embed( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::optional<ComplexName> element_type;
	Expression expression;
};

struct ZeroInitializer
{
	explicit ZeroInitializer( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
};

struct UninitializedInitializer
{
	explicit UninitializedInitializer( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
};

struct SequenceInitializer
{
	explicit SequenceInitializer( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::vector<Initializer> initializers;
};

struct StructNamedInitializer
{
	explicit StructNamedInitializer( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	struct MemberInitializer;

	SrcLoc src_loc;
	std::vector<MemberInitializer> members_initializers;
};

struct ConstructorInitializer
{
	explicit ConstructorInitializer( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::vector<Expression> arguments;
};

struct ConstructorInitializerSignatureHelp
{
	explicit ConstructorInitializerSignatureHelp( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::vector<Expression> arguments;
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
	explicit Label( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string name;
};

// Just block - as it used as part of other syntax elements.
struct Block
{
	explicit Block( const SrcLoc& start_src_loc )
		: src_loc(start_src_loc) {}

	SrcLoc src_loc;
	SrcLoc end_src_loc;
	BlockElementsList elements;
};

// Block inside scope - with additional properties.
struct ScopeBlock
{
	explicit ScopeBlock( Block block )
		: src_loc(block.src_loc), block(std::move(block)) {}

	enum class Safety : uint8_t
	{
		None,
		Safe,
		Unsafe,
	};

	SrcLoc src_loc;
	Block block;
	std::optional<Label> label;
	Safety safety= Safety::None;
};

struct VariablesDeclaration
{
	explicit VariablesDeclaration( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

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
	explicit AutoVariableDeclaration( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string name;
	Expression initializer_expression;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
};

struct AllocaDeclaration
{
	explicit AllocaDeclaration( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	TypeName type;
	std::string name;
	Expression size;
};

struct ReturnOperator
{
	explicit ReturnOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct YieldOperator
{
	explicit YieldOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct WhileOperator
{
	explicit WhileOperator( const SrcLoc& src_loc )
		: src_loc(src_loc), block(src_loc) {}

	SrcLoc src_loc;
	Expression condition;
	std::optional<Label> label;
	Block block;
};

struct LoopOperator
{
	explicit LoopOperator( const SrcLoc& src_loc )
		: src_loc(src_loc), block(src_loc) {}

	SrcLoc src_loc;
	std::optional<Label> label;
	Block block;
};

struct RangeForOperator
{
	explicit RangeForOperator( const SrcLoc& src_loc )
		: src_loc(src_loc), block(src_loc) {}

	SrcLoc src_loc;
	std::string loop_variable_name;
	Expression sequence;
	std::optional<Label> label;
	Block block;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
};

struct CStyleForOperator
{
	using IterationPartElementsList= VariantLinkedList<
		SingleExpressionOperator,
		AssignmentOperator,
		CompoundAssignmentOperator,
		IncrementOperator,
		DecrementOperator>;

	explicit CStyleForOperator( const SrcLoc& src_loc )
		: src_loc(src_loc), block(src_loc) {}

	SrcLoc src_loc;

	std::unique_ptr< const std::variant< VariablesDeclaration, AutoVariableDeclaration > > variable_declaration_part;
	Expression loop_condition;
	IterationPartElementsList iteration_part_elements;
	std::optional<Label> label;
	Block block;
};

struct BreakOperator
{
	explicit BreakOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::optional<Label> label;
};

struct ContinueOperator
{
	explicit ContinueOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::optional<Label> label;
};

struct WithOperator
{
	explicit WithOperator( const SrcLoc& src_loc )
		: src_loc(src_loc), block(src_loc) {}

	SrcLoc src_loc;
	std::string variable_name;
	Expression expression;
	Block block;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
};

struct IfOperator
{
	explicit IfOperator( const SrcLoc& start_src_loc )
		: src_loc(start_src_loc), block(start_src_loc) {}

	SrcLoc src_loc;
	SrcLoc end_src_loc;
	Expression condition;
	Block block;
	IfAlternativePtr alternative; // non-null if "else" branch exists.
};

struct StaticIfOperator
{
	explicit StaticIfOperator( const SrcLoc& src_loc )
		: src_loc(src_loc), block(src_loc) {}

	SrcLoc src_loc;
	Expression condition;
	Block block;
	IfAlternativePtr alternative; // non-null if "else" branch exists.
};

struct IfCoroAdvanceOperator
{
	explicit IfCoroAdvanceOperator( const SrcLoc& src_loc )
		: src_loc(src_loc), block(src_loc) {}

	SrcLoc src_loc;
	SrcLoc end_src_loc;
	std::string variable_name;
	Expression expression;
	Block block;
	IfAlternativePtr alternative;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
};

struct SwitchOperator
{
	explicit SwitchOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

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
	SrcLoc end_src_loc;
	Expression value;
	std::vector<Case> cases;
};

struct SingleExpressionOperator
{
	explicit SingleExpressionOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct AssignmentOperator
{
	explicit AssignmentOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression l_value;
	Expression r_value;
};

struct CompoundAssignmentOperator
{
	explicit CompoundAssignmentOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	BinaryOperatorType compound_operation;
	Expression l_value;
	Expression r_value;
};

struct IncrementOperator
{
	explicit IncrementOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct DecrementOperator
{
	explicit DecrementOperator( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct StaticAssert
{
	explicit StaticAssert( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
	std::optional<std::string> message;
};

struct Halt
{
	explicit Halt( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
};

struct HaltIf
{
	explicit HaltIf( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression condition;
};

struct Function
{
	explicit Function( const SrcLoc& src_loc )
		: src_loc(src_loc), type(src_loc) {}

public:
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
		Async,
	};

	struct NameComponent
	{
		std::string name;
		SrcLoc src_loc;
		bool completion_requested= false;
	};

public:
	SrcLoc src_loc;
	std::vector<NameComponent> name; // A, A::B, A::B::C::D, ::A, ::A::B
	Expression condition; // Empty variant if has no condition.
	FunctionType type;
	std::unique_ptr<const StructNamedInitializer> constructor_initialization_list;
	std::unique_ptr<const Block> block;
	OverloadedOperator overloaded_operator= OverloadedOperator::None;
	VirtualFunctionKind virtual_function_kind= VirtualFunctionKind::None;
	BodyKind body_kind= BodyKind::None;
	Kind kind= Kind::Regular;
	bool no_mangle= false;
	bool is_conversion_constructor= false;
	bool constexpr_= false;
};

struct Lambda
{
	struct CaptureNothing{};
	struct CaptureAllByValue{};
	struct CaptureAllByReference{};

	struct CaptureListElement
	{
		SrcLoc src_loc;
		std::string name;
		Expression expression; // If not an EmptyVariant - contains initializer expression.
		bool by_reference= false;
		bool completion_requested= false;
	};

	using CaptureList= std::vector<CaptureListElement>;

	using Capture= std::variant<CaptureNothing, CaptureAllByValue, CaptureAllByReference, CaptureList >;

public:
	explicit Lambda( const SrcLoc& src_loc )
		: src_loc(src_loc), function(src_loc) {}

public:
	SrcLoc src_loc;
	Capture capture;
	Function function;
};

struct VariableInitialization
{
	explicit VariableInitialization( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression type;
	Initializer initializer;
};

struct TypeAlias
{
	explicit TypeAlias( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string name;
	TypeName value;
};

struct Enum
{
	explicit Enum( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	struct Member
	{
		SrcLoc src_loc;
		std::string name;
	};

	SrcLoc src_loc;
	std::string name;
	std::optional<ComplexName> underlying_type_name;
	std::vector<Member> members;
};

struct Class
{
	explicit Class( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	ClassElementsList elements;
	std::string name;
	std::vector<ComplexName> parents;
	NonSyncTag non_sync_tag;
	ClassKindAttribute kind_attribute = ClassKindAttribute::Struct;
	bool keep_fields_order= false;
	bool no_discard= false;
};

struct ClassField
{
	explicit ClassField( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	TypeName type;
	std::string name;
	std::unique_ptr<const Initializer> initializer; // May be null.
	Expression reference_tag_expression; // EmptyVariant if none.
	Expression inner_reference_tags_expression; // EmptyVariant if none.
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
};

struct ClassVisibilityLabel
{
	ClassVisibilityLabel( const SrcLoc& src_loc, ClassMemberVisibility visibility )
		: src_loc(src_loc), visibility(visibility) {}

	SrcLoc src_loc;
	const ClassMemberVisibility visibility;
};


struct TemplateParam
{
	struct TypeParamData{};

	struct TypeTemplateParamData{};

	struct VariableParamData
	{
		// Type of variable param.
		TypeName type;
	};

public:
	SrcLoc src_loc;
	// Type param or type template param or variable param with given type.
	std::variant<TypeParamData, TypeTemplateParamData, VariableParamData> kind_data;
	std::string name;
};

struct TypeTemplate
{
	// Param in template signature.
	struct SignatureParam
	{
		Expression name;
		Expression default_value;
	};

	explicit TypeTemplate( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string name;

	std::vector<TemplateParam> params;
	std::vector<SignatureParam> signature_params;

	std::variant<std::unique_ptr<const Class>, std::unique_ptr<const TypeAlias>> something;

	// Short form means that template params are also signature params.
	bool is_short_form= false;
};

struct FunctionTemplate
{
	explicit FunctionTemplate( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::vector<TemplateParam> params;

	std::unique_ptr<const Function> function;
};

struct Namespace
{
	explicit Namespace( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string name;
	ProgramElementsList elements;
};

struct Mixin
{
	explicit Mixin( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	Expression expression;
};

struct Import
{
	explicit Import( const SrcLoc& src_loc )
		: src_loc(src_loc) {}

	SrcLoc src_loc;
	std::string import_name;
};

// Utility functions for manipulations with variants.

SrcLoc GetSrcLoc( const ComplexName& complex_name );
SrcLoc GetSrcLoc( const Expression& expression );
SrcLoc GetSrcLoc( const Initializer& initializer );

inline TypeName ComplexNameToTypeName( ComplexName n )
{
	return std::visit( []( auto&& el ) { return TypeName(std::move(el)); }, std::move(n) );
}

inline Expression ComplexNameToExpression( ComplexName n )
{
	return std::visit( []( auto&& el ) { return Expression(std::move(el)); }, std::move(n) );
}

inline Expression TypeNameToExpression( TypeName t )
{
	return std::visit( []( auto&& el ) { return Expression(std::move(el)); }, std::move(t) );
}

} // namespace Synt

} // namespace U
