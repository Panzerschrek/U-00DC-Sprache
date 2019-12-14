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
struct TypeNameInExpression;
struct NumericConstant;
struct BracketExpression;
struct BooleanConstant;
struct StringLiteral;
struct MoveOperator;
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
struct BreakOperator;
struct ContinueOperator;
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
struct ClassTemplate;
struct TypedefTemplate;
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
	TypeofTypeName,
	NamedTypeName,
	FunctionTypePtr,
	TupleType >;

using UnaryPrefixOperator= std::variant<
	UnaryPlus,
	UnaryMinus,
	LogicalNot,
	BitwiseNot >;

using UnaryPostfixOperator= std::variant<
	CallOperator,
	IndexationOperator,
	MemberAccessOperator >;

using Expression= std::variant<
	EmptyVariant,
	BinaryOperator,
	NamedOperand,
	TernaryOperator,
	TypeNameInExpression,
	NumericConstant,
	BracketExpression,
	BooleanConstant,
	StringLiteral,
	MoveOperator,
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
	BreakOperator,
	ContinueOperator,
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
	ClassTemplate,
	TypedefTemplate,
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
	ClassTemplate,
	TypedefTemplate,
	FunctionTemplate,
	NamespacePtr >;

using ProgramElements= std::vector<ProgramElement>;

using ReferencesTagsList= std::vector<std::string>; // If last tag is empty string - means continuous tag - like arg' a, b, c... '

struct SyntaxElementBase
{
public:
	explicit SyntaxElementBase( const FilePos& file_pos );
	// WARNING! This struct have NO virtual destructor for, size optimization.
	// Do not like this:  SyntaxElementBase* x= new Derived();

	FilePos file_pos_;
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

struct ComplexName final
{
	// A
	// A::b
	// TheClass::Method
	// ::Absolute::Name
	// ::C_Function
	// std::vector</i32/>
	// std::map</f32, T/>::value_type

	// If first component name is empty, name starts with "::".
	struct Component
	{
		std::string name;
		std::vector<Expression> template_parameters;
		bool have_template_parameters= false;
	};
	std::vector<Component> components;
};

struct ArrayTypeName final : public SyntaxElementBase
{
public:
	explicit ArrayTypeName( const FilePos& file_pos );

	std::unique_ptr<TypeName> element_type;
	std::unique_ptr<Expression> size;
};

struct TupleType final : public SyntaxElementBase
{
public:
	TupleType( const FilePos& file_pos );

public:
	std::vector<TypeName> element_types_;
};

struct TypeofTypeName final : public SyntaxElementBase
{
public:
	explicit TypeofTypeName( const FilePos& file_pos );

	std::unique_ptr<Expression> expression;
};

struct NamedTypeName final : public SyntaxElementBase
{
public:
	explicit NamedTypeName( const FilePos& file_pos );

	ComplexName name;
};

struct ReferencePollutionSrc
{
	std::string name;
	bool is_mutable= true;
};
using FunctionReferencesPollution= std::pair< std::string, ReferencePollutionSrc >;
using FunctionReferencesPollutionList= std::vector<FunctionReferencesPollution>;

struct FunctionArgument;
using FunctionArgumentsDeclaration= std::vector<FunctionArgument>;

struct FunctionType final : public SyntaxElementBase
{
public:
	FunctionType( const FilePos& file_pos );

	std::unique_ptr<TypeName> return_type_;
	std::string return_value_reference_tag_;
	FunctionReferencesPollutionList referecnces_pollution_list_;
	FunctionArgumentsDeclaration arguments_;
	ReferencesTagsList return_value_inner_reference_tags_;

	MutabilityModifier return_value_mutability_modifier_= MutabilityModifier::None;
	ReferenceModifier return_value_reference_modifier_= ReferenceModifier::None;
	bool unsafe_= false;
};

struct FunctionArgument final : public SyntaxElementBase
{
public:
	FunctionArgument( const FilePos& file_pos );

public:
	std::string name_;
	TypeName type_;
	std::string reference_tag_;
	ReferencesTagsList inner_arg_reference_tags_;
	MutabilityModifier mutability_modifier_= MutabilityModifier::None;
	ReferenceModifier reference_modifier_= ReferenceModifier::None;
};

struct BinaryOperator final : public SyntaxElementBase
{
public:
	explicit BinaryOperator( const FilePos& file_pos );

	BinaryOperatorType operator_type_;
	std::unique_ptr<Expression> left_;
	std::unique_ptr<Expression> right_;
};

struct ExpressionComponentWithUnaryOperators : public SyntaxElementBase
{
public:
	explicit ExpressionComponentWithUnaryOperators( const FilePos& file_pos );

	std::vector<UnaryPrefixOperator > prefix_operators_ ;
	std::vector<UnaryPostfixOperator> postfix_operators_;
};

struct TernaryOperator final : public ExpressionComponentWithUnaryOperators
{
public:
	explicit TernaryOperator( const FilePos& file_pos );

	std::unique_ptr<Expression> condition;
	std::unique_ptr<Expression> true_branch;
	std::unique_ptr<Expression> false_branch;
};

struct NamedOperand final : public ExpressionComponentWithUnaryOperators
{
public:
	NamedOperand( const FilePos& file_pos, ComplexName name );

	ComplexName name_;
};

struct MoveOperator final : public ExpressionComponentWithUnaryOperators
{
public:
	MoveOperator( const FilePos& file_pos );

	std::string var_name_;
};

struct CastRef final : public ExpressionComponentWithUnaryOperators
{
public:
	CastRef( const FilePos& file_pos );

	std::unique_ptr<TypeName> type_;
	std::unique_ptr<Expression> expression_;
};

struct CastRefUnsafe final : public ExpressionComponentWithUnaryOperators
{
public:
	CastRefUnsafe( const FilePos& file_pos );

	std::unique_ptr<TypeName> type_;
	std::unique_ptr<Expression> expression_;
};

struct CastImut final : public ExpressionComponentWithUnaryOperators
{
public:
	CastImut( const FilePos& file_pos );

	std::unique_ptr<Expression> expression_;
};

struct CastMut final : public ExpressionComponentWithUnaryOperators
{
public:
	CastMut( const FilePos& file_pos );

	std::unique_ptr<Expression> expression_;
};

struct TypeInfo final : public ExpressionComponentWithUnaryOperators
{
public:
	TypeInfo( const FilePos& file_pos );

	std::unique_ptr<TypeName> type_;
};

struct BooleanConstant final : public ExpressionComponentWithUnaryOperators
{
public:
	BooleanConstant( const FilePos& file_pos, bool value );

	bool value_;
};

using TypeSuffix= std::array<char, 7>;

struct NumericConstant final : public ExpressionComponentWithUnaryOperators, public NumberLexemData
{
public:
	NumericConstant( const FilePos& file_pos );
};

struct StringLiteral final : public ExpressionComponentWithUnaryOperators
{
public:
	StringLiteral( const FilePos& file_pos );

	std::string value_;
	TypeSuffix type_suffix_;
};

struct BracketExpression final : public ExpressionComponentWithUnaryOperators
{
public:
	BracketExpression( const FilePos& file_pos );

	std::unique_ptr<Expression> expression_;
};

struct TypeNameInExpression final : public ExpressionComponentWithUnaryOperators
{
public:
	explicit TypeNameInExpression( const FilePos& file_pos );

	TypeName type_name;
};

struct UnaryPlus final : public SyntaxElementBase
{
public:
	explicit UnaryPlus( const FilePos& file_pos );
};

struct UnaryMinus final : public SyntaxElementBase
{
public:
	explicit UnaryMinus( const FilePos& file_pos );
};

struct LogicalNot final : public SyntaxElementBase
{
public:
	explicit LogicalNot( const FilePos& file_pos );
};

struct BitwiseNot final : public SyntaxElementBase
{
public:
	explicit BitwiseNot( const FilePos& file_pos );
};

struct CallOperator final : public SyntaxElementBase
{
public:
	CallOperator( const FilePos& file_pos );

	std::vector<Expression> arguments_;
};

struct IndexationOperator final : public SyntaxElementBase
{
public:
	explicit IndexationOperator( const FilePos& file_pos );

	Expression index_;
};

struct MemberAccessOperator final : public SyntaxElementBase
{
public:
	MemberAccessOperator( const FilePos& file_pos );

	std::string member_name_;
	std::vector<Expression> template_parameters;
	bool have_template_parameters= false;
};

struct ArrayInitializer final : public SyntaxElementBase
{
public:
	explicit ArrayInitializer( const FilePos& file_pos );

	std::vector<Initializer> initializers;
	bool has_continious_initializer= false; // ... after last initializator.
};

struct StructNamedInitializer final : public SyntaxElementBase
{
public:
	explicit StructNamedInitializer( const FilePos& file_pos );

	struct MemberInitializer;

	std::vector<MemberInitializer> members_initializers;
};

struct ConstructorInitializer final : public SyntaxElementBase
{
public:
	ConstructorInitializer( const FilePos& file_pos );

	CallOperator call_operator;
};

struct ExpressionInitializer final : public SyntaxElementBase
{
public:
	ExpressionInitializer( const FilePos& file_pos );

	Expression expression;
};

struct ZeroInitializer final : public SyntaxElementBase
{
public:
	explicit ZeroInitializer( const FilePos& file_pos );
};

struct UninitializedInitializer final : public SyntaxElementBase
{
public:
	explicit UninitializedInitializer( const FilePos& file_pos );
};

struct StructNamedInitializer::MemberInitializer
{
	std::string name;
	Initializer initializer;
};

struct Block final : public SyntaxElementBase
{
public:
	Block( const FilePos& start_file_pos );

	enum class Safety : uint8_t
	{
		None,
		Safe,
		Unsafe,
	};
public:
	FilePos end_file_pos_;
	std::vector<BlockElement> elements_;
	Safety safety_= Safety::None;
};

struct VariablesDeclaration final : public SyntaxElementBase
{
	VariablesDeclaration( const FilePos& file_pos );

	struct VariableEntry
	{
		FilePos file_pos;
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
	explicit AutoVariableDeclaration( const FilePos& file_pos );

	std::string name;
	Expression initializer_expression;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
	bool lock_temps= false;
};

struct ReturnOperator final : public SyntaxElementBase
{
public:
	ReturnOperator( const FilePos& file_pos );

	Expression expression_;
};

struct WhileOperator final : public SyntaxElementBase
{
public:
	WhileOperator( const FilePos& file_pos );

	Expression condition_;
	Block block_;
};

struct ForOperator final : public SyntaxElementBase
{
public:
	ForOperator( const FilePos& file_pos );

	ReferenceModifier reference_modifier_= ReferenceModifier::None;
	MutabilityModifier mutability_modifier_= MutabilityModifier::None;
	std::string loop_variable_name_;
	Expression sequence_;
	Block block_;
};

struct BreakOperator final : public SyntaxElementBase
{
public:
	explicit BreakOperator( const FilePos& file_pos );
};

struct ContinueOperator final : public SyntaxElementBase
{
public:
	explicit ContinueOperator( const FilePos& file_pos );
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

	IfOperator( const FilePos& start_file_pos );

	std::vector<Branch> branches_; // else if()
	FilePos end_file_pos_;
};

struct StaticIfOperator final : public SyntaxElementBase
{
public:
	StaticIfOperator( const FilePos& file_pos );

	IfOperator if_operator_;
};

struct SingleExpressionOperator final : public SyntaxElementBase
{
public:
	SingleExpressionOperator( const FilePos& file_pos );

	Expression expression_;
};

struct AssignmentOperator final : public SyntaxElementBase
{
public:
	AssignmentOperator( const FilePos& file_pos );

	Expression l_value_;
	Expression r_value_;
};

struct AdditiveAssignmentOperator final : public SyntaxElementBase
{
public:
	explicit AdditiveAssignmentOperator( const FilePos& file_pos );

	BinaryOperatorType additive_operation_;
	Expression l_value_;
	Expression r_value_;
};

struct IncrementOperator final : public SyntaxElementBase
{
public:
	explicit IncrementOperator( const FilePos& file_pos );

	Expression expression;
};

struct DecrementOperator final : public SyntaxElementBase
{
public:
	explicit DecrementOperator( const FilePos& file_pos );

	Expression expression;
};

struct StaticAssert final : public SyntaxElementBase
{
public:
	explicit StaticAssert( const FilePos& file_pos );

	Expression expression;
};

struct Halt final
	: public SyntaxElementBase

{
public:
	explicit Halt( const FilePos& file_pos );
};

struct HaltIf final
	: public SyntaxElementBase

{
public:
	explicit HaltIf( const FilePos& file_pos );

	Expression condition;
};

struct Typedef final : public SyntaxElementBase
{
public:
	explicit Typedef( const FilePos& file_pos );

	std::string name;
	TypeName value;
};

struct Enum final : public SyntaxElementBase
{
public:
	explicit Enum( const FilePos& file_pos );

	struct Member
	{
		FilePos file_pos;
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
	Function( const FilePos& file_pos );

	enum class BodyKind : uint8_t
	{
		None,
		BodyGenerationRequired,
		BodyGenerationDisabled,
	};

	ComplexName name_;
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
	explicit ClassField( const FilePos& file_pos );

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
	ClassVisibilityLabel( const FilePos& file_pos, ClassMemberVisibility visibility );

	const ClassMemberVisibility visibility_;
};

struct Class final : public SyntaxElementBase
{
public:
	explicit Class( const FilePos& file_pos );

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
	explicit TemplateBase( const FilePos& file_pos );

	// For type arguments, like template</ type A, type B />, arg_type is empty.
	// For value arguments, like template</ type A, A x, i32 y />, arg_type is comples name of argument.
	struct Arg
	{
		const ComplexName* arg_type= nullptr; // pointer to arg_type_expr
		std::unique_ptr<Expression> arg_type_expr; // Actyally, only NamedOperand

		const ComplexName* name= nullptr; // Actually, only name with one component
		std::unique_ptr<Expression> name_expr;
	};

	std::vector<Arg> args_;
};

struct TypeTemplateBase : public TemplateBase
{
public:
	enum class Kind{ Class, Typedef, }; // HACK! Replacement for RTTI.

	explicit TypeTemplateBase( const FilePos& file_pos, Kind kind );

	// Argument in template signature.
	struct SignatureArg
	{
		Expression name;
		Expression default_value;
	};

	const Kind kind_;
	std::vector<SignatureArg> signature_args_;
	std::string name_;

	// Short form means that template argumenst are also signature arguments.
	bool is_short_form_= false;
};

struct ClassTemplate final : public TypeTemplateBase
{
public:
	explicit ClassTemplate( const FilePos& file_pos );

	ClassPtr class_;
};

struct TypedefTemplate final : public TypeTemplateBase
{
public:
	explicit TypedefTemplate( const FilePos& file_pos );

	std::unique_ptr<Typedef> typedef_;
};

struct FunctionTemplate final : public TemplateBase
{
public:
	explicit FunctionTemplate( const FilePos& file_pos );

	FunctionPtr function_;
};

struct Namespace final : public SyntaxElementBase
{
public:
	explicit Namespace( const FilePos& file_pos );

	std::string name_;
	ProgramElements elements_;
};

struct Import final : public SyntaxElementBase
{
public:
	explicit Import( const FilePos& file_pos );

	std::string import_name;
};

// Utility functions for manipulations with variants.

FilePos GetExpressionFilePos( const Expression& expression );
FilePos GetInitializerFilePos( const Initializer& initializer );
FilePos GetBlockElementFilePos( const BlockElement& block_element );

OverloadedOperator PrefixOperatorKind( const UnaryPrefixOperator& prefix_operator );

} // namespace Synt

} // namespace U
