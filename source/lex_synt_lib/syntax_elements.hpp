#pragma once
#include <limits>
#include <memory>
#include <vector>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "lexical_analyzer.hpp"
#include "operators.hpp"

namespace U
{

namespace Synt
{

class EmptyVariant{};

class ArrayTypeName;
class TypeofTypeName;
class NamedTypeName;
class FunctionType;

class UnaryPlus;
class UnaryMinus;
class LogicalNot;
class BitwiseNot;

class CallOperator;
class IndexationOperator;
class MemberAccessOperator;

class BinaryOperator;
class NamedOperand;
class TypeNameInExpression;
class NumericConstant;
class BracketExpression;
class BooleanConstant;
class StringLiteral;
class MoveOperator;
class CastMut;
class CastImut;
class CastRef;
class CastRefUnsafe;
class TypeInfo;

class ArrayInitializer;
class StructNamedInitializer;
class ConstructorInitializer;
class ExpressionInitializer;
class ZeroInitializer;
class UninitializedInitializer;

class Block;
class VariablesDeclaration;
class AutoVariableDeclaration;
class ReturnOperator;
class WhileOperator;
class BreakOperator;
class ContinueOperator;
class IfOperator;
class StaticIfOperator;
class SingleExpressionOperator;
class AssignmentOperator;
class AdditiveAssignmentOperator;
class IncrementOperator;
class DecrementOperator;
class StaticAssert;
class Halt;
class HaltIf;

using TypeName= boost::variant< EmptyVariant, ArrayTypeName, TypeofTypeName, NamedTypeName, FunctionType >;

using UnaryPrefixOperator= boost::variant< UnaryPlus, UnaryMinus, LogicalNot, BitwiseNot >;
using UnaryPostfixOperator= boost::variant< CallOperator, IndexationOperator, MemberAccessOperator >;

using PrefixOperators= std::vector<UnaryPrefixOperator>;
using PostfixOperators= std::vector<UnaryPostfixOperator>;

using Expression= boost::variant< EmptyVariant, BinaryOperator, NamedOperand, TypeNameInExpression, NumericConstant, BracketExpression, BooleanConstant, StringLiteral, MoveOperator, CastMut, CastImut, CastRef, CastRefUnsafe, TypeInfo >;

using Initializer= boost::variant< EmptyVariant, ArrayInitializer, StructNamedInitializer, ConstructorInitializer, ExpressionInitializer, ZeroInitializer, UninitializedInitializer >;
using InitializerPtr= std::shared_ptr<Initializer>; // TODO - does this needs?

using BlockElement= boost::variant<
	Block,
	VariablesDeclaration,
	AutoVariableDeclaration,
	ReturnOperator,
	WhileOperator,
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

typedef std::vector<ProgramString> ReferencesTagsList; // If last tag is empty string - means continuous tag - like arg' a, b, c... '

class SyntaxElementBase
{
public:
	explicit SyntaxElementBase( const FilePos& file_pos );
	virtual ~SyntaxElementBase()= default;

	FilePos file_pos_;
};

enum class MutabilityModifier
{
	None,
	Mutable,
	Immutable,
	Constexpr,
};

enum class ReferenceModifier
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
		ProgramString name;
		std::vector<Expression> template_parameters;
		bool have_template_parameters= false;
	};
	std::vector<Component> components;
};

class ArrayTypeName final : public SyntaxElementBase
{
public:
	explicit ArrayTypeName( const FilePos& file_pos );

	std::unique_ptr<TypeName> element_type;
	std::unique_ptr<Expression> size;
};

class TypeofTypeName final : public SyntaxElementBase
{
public:
	explicit TypeofTypeName( const FilePos& file_pos );

	std::unique_ptr<Expression> expression;
};

class NamedTypeName final : public SyntaxElementBase
{
public:
	explicit NamedTypeName( const FilePos& file_pos );

	ComplexName name;
};

class FunctionArgument;
typedef std::unique_ptr<FunctionArgument> FunctionArgumentPtr;
typedef std::vector<FunctionArgumentPtr> FunctionArgumentsDeclaration;

struct ReferencePollutionSrc
{
	ProgramString name;
	bool is_mutable= true;
};
typedef std::pair< ProgramString, ReferencePollutionSrc > FunctionReferencesPollution;
typedef std::vector<FunctionReferencesPollution> FunctionReferencesPollutionList;

class FunctionType final : public SyntaxElementBase
{
public:
	FunctionType( const FilePos& file_pos );

	std::unique_ptr<TypeName> return_type_;
	MutabilityModifier return_value_mutability_modifier_= MutabilityModifier::None;
	ReferenceModifier return_value_reference_modifier_= ReferenceModifier::None;
	ProgramString return_value_reference_tag_;
	ReferencesTagsList return_value_inner_reference_tags_;
	FunctionReferencesPollutionList referecnces_pollution_list_;
	FunctionArgumentsDeclaration arguments_;
	bool unsafe_= false;
};

class FunctionArgument final : public SyntaxElementBase
{
public:
	FunctionArgument(
		const FilePos& file_pos,
		ProgramString name,
		TypeName type,
		MutabilityModifier mutability_modifier,
		ReferenceModifier reference_modifier,
		ProgramString reference_tag,
		ReferencesTagsList inner_arg_reference_tags );

public:
	const ProgramString name_;
	const TypeName type_;
	const MutabilityModifier mutability_modifier_;
	const ReferenceModifier reference_modifier_;
	const ProgramString reference_tag_;
	const ReferencesTagsList inner_arg_reference_tags_;
};

FilePos GetExpressionFilePos( const Expression& expression );
FilePos GetInitializerFilePos( const Initializer& initializer );
FilePos GetBlockElementFilePos( const BlockElement& block_element );

class BinaryOperator final : public SyntaxElementBase
{
public:
	explicit BinaryOperator( const FilePos& file_pos );

	BinaryOperatorType operator_type_;
	std::unique_ptr<Expression> left_;
	std::unique_ptr<Expression> right_;
};

class ExpressionComponentWithUnaryOperators : public SyntaxElementBase
{
public:
	explicit ExpressionComponentWithUnaryOperators( const FilePos& file_pos );

	std::vector<UnaryPrefixOperator > prefix_operators_ ;
	std::vector<UnaryPostfixOperator> postfix_operators_;
};

class NamedOperand final : public ExpressionComponentWithUnaryOperators
{
public:
	NamedOperand( const FilePos& file_pos, ComplexName name );

	ComplexName name_;
};

class MoveOperator final : public ExpressionComponentWithUnaryOperators
{
public:
	MoveOperator( const FilePos& file_pos );

	ProgramString var_name_;
};

class CastRef final : public ExpressionComponentWithUnaryOperators
{
public:
	CastRef( const FilePos& file_pos );

	TypeName type_;
	std::unique_ptr<Expression> expression_;
};

class CastRefUnsafe final : public ExpressionComponentWithUnaryOperators
{
public:
	CastRefUnsafe( const FilePos& file_pos );

	TypeName type_;
	std::unique_ptr<Expression> expression_;
};

class CastImut final : public ExpressionComponentWithUnaryOperators
{
public:
	CastImut( const FilePos& file_pos );

	std::unique_ptr<Expression> expression_;
};

class CastMut final : public ExpressionComponentWithUnaryOperators
{
public:
	CastMut( const FilePos& file_pos );

	std::unique_ptr<Expression> expression_;
};

class TypeInfo final : public ExpressionComponentWithUnaryOperators
{
public:
	TypeInfo( const FilePos& file_pos );

	TypeName type_;
};

class BooleanConstant final : public ExpressionComponentWithUnaryOperators
{
public:
	BooleanConstant( const FilePos& file_pos, bool value );

	bool value_;
};

class NumericConstant final : public ExpressionComponentWithUnaryOperators
{
public:
	typedef long double LongFloat;
	static_assert(
		std::numeric_limits<LongFloat>::digits >= 64,
		"Too short \"LongFloat\". LongFloat must store all uint64_t and int64_t values exactly." );

	NumericConstant(
		const FilePos& file_pos,
		LongFloat value,
		ProgramString type_suffix,
		bool has_fractional_point );

	LongFloat value_;
	ProgramString type_suffix_;
	bool has_fractional_point_;
};

class StringLiteral final : public ExpressionComponentWithUnaryOperators
{
public:
	StringLiteral( const FilePos& file_pos );

	ProgramString value_;
	ProgramString type_suffix_;
};

class BracketExpression final : public ExpressionComponentWithUnaryOperators
{
public:
	BracketExpression( const FilePos& file_pos );

	std::unique_ptr<Expression> expression_;
};

class TypeNameInExpression final : public ExpressionComponentWithUnaryOperators
{
public:
	explicit TypeNameInExpression( const FilePos& file_pos );

	TypeName type_name;
};

class UnaryPlus final : public SyntaxElementBase
{
public:
	explicit UnaryPlus( const FilePos& file_pos );
};

class UnaryMinus final : public SyntaxElementBase
{
public:
	explicit UnaryMinus( const FilePos& file_pos );
};

class LogicalNot final : public SyntaxElementBase
{
public:
	explicit LogicalNot( const FilePos& file_pos );
};

class BitwiseNot final : public SyntaxElementBase
{
public:
	explicit BitwiseNot( const FilePos& file_pos );
};

class CallOperator final : public SyntaxElementBase
{
public:
	CallOperator( const FilePos& file_pos );

	std::vector<Expression> arguments_;
};

class IndexationOperator final : public SyntaxElementBase
{
public:
	explicit IndexationOperator( const FilePos& file_pos );

	Expression index_;
};

class MemberAccessOperator final : public SyntaxElementBase
{
public:
	MemberAccessOperator( const FilePos& file_pos );

	ProgramString member_name_;
	std::vector<Expression> template_parameters;
	bool have_template_parameters= false;
};

class ArrayInitializer final : public SyntaxElementBase
{
public:
	explicit ArrayInitializer( const FilePos& file_pos );

	std::vector<Initializer> initializers;
	bool has_continious_initializer= false; // ... after last initializator.
};

class StructNamedInitializer final : public SyntaxElementBase
{
public:
	explicit StructNamedInitializer( const FilePos& file_pos );

	struct MemberInitializer;

	std::vector<MemberInitializer> members_initializers;
};

class ConstructorInitializer final : public SyntaxElementBase
{
public:
	ConstructorInitializer( const FilePos& file_pos );

	CallOperator call_operator;
};

class ExpressionInitializer final : public SyntaxElementBase
{
public:
	ExpressionInitializer( const FilePos& file_pos );

	Expression expression;
};

class ZeroInitializer final : public SyntaxElementBase
{
public:
	explicit ZeroInitializer( const FilePos& file_pos );
};

class UninitializedInitializer final : public SyntaxElementBase
{
public:
	explicit UninitializedInitializer( const FilePos& file_pos );
};

struct StructNamedInitializer::MemberInitializer
{
	ProgramString name;
	Initializer initializer;
};

class IProgramElement
{
public:
	virtual ~IProgramElement(){}
};

typedef std::unique_ptr<IProgramElement> IProgramElementPtr;
typedef std::vector<IProgramElementPtr> ProgramElements;

class IClassElement
{
public:
	virtual ~IClassElement(){}
};

typedef std::unique_ptr<IClassElement> IClassElementPtr;
typedef std::vector<IClassElementPtr> ClassElements;

class Block final : public SyntaxElementBase
{
public:
	Block( const FilePos& start_file_pos );

	enum class Safety
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

typedef std::unique_ptr<Block> BlockPtr;

struct VariablesDeclaration final : public SyntaxElementBase
	, public IProgramElement
	, public IClassElement
{
	VariablesDeclaration( const FilePos& file_pos );

	struct VariableEntry
	{
		FilePos file_pos;
		ProgramString name;
		InitializerPtr initializer; // May be null for types with default constructor.
		MutabilityModifier mutability_modifier= MutabilityModifier::None;
		ReferenceModifier reference_modifier= ReferenceModifier::None;
	};

	std::vector<VariableEntry> variables;
	TypeName type;
};

typedef std::unique_ptr<VariablesDeclaration> VariablesDeclarationPtr;

struct AutoVariableDeclaration final
	: public SyntaxElementBase

	, public IProgramElement
	, public IClassElement
{
	explicit AutoVariableDeclaration( const FilePos& file_pos );

	ProgramString name;
	Expression initializer_expression;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
	bool lock_temps= false;
};

class ReturnOperator final : public SyntaxElementBase
{
public:
	ReturnOperator( const FilePos& file_pos );

	Expression expression_;
};

class WhileOperator final : public SyntaxElementBase
{
public:
	WhileOperator( const FilePos& file_pos );

	Expression condition_;
	Block block_;
};

class BreakOperator final : public SyntaxElementBase
{
public:
	explicit BreakOperator( const FilePos& file_pos );
};

class ContinueOperator final : public SyntaxElementBase
{
public:
	explicit ContinueOperator( const FilePos& file_pos );
};

class IfOperator final : public SyntaxElementBase
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

class StaticIfOperator final : public SyntaxElementBase
{
public:
	StaticIfOperator( const FilePos& file_pos );

	IfOperator if_operator_;
};

class SingleExpressionOperator final : public SyntaxElementBase
{
public:
	SingleExpressionOperator( const FilePos& file_pos );

	Expression expression_;
};

class AssignmentOperator final : public SyntaxElementBase
{
public:
	AssignmentOperator( const FilePos& file_pos );

	Expression l_value_;
	Expression r_value_;
};

class AdditiveAssignmentOperator final : public SyntaxElementBase
{
public:
	explicit AdditiveAssignmentOperator( const FilePos& file_pos );

	Expression l_value_;
	Expression r_value_;
	BinaryOperatorType additive_operation_;
};

class IncrementOperator final : public SyntaxElementBase
{
public:
	explicit IncrementOperator( const FilePos& file_pos );

	Expression expression;
};

class DecrementOperator final : public SyntaxElementBase
{
public:
	explicit DecrementOperator( const FilePos& file_pos );

	Expression expression;
};

class StaticAssert final
	: public SyntaxElementBase

	, public IProgramElement
	, public IClassElement
{
public:
	explicit StaticAssert( const FilePos& file_pos );

	Expression expression;
};

class Halt final
	: public SyntaxElementBase

{
public:
	explicit Halt( const FilePos& file_pos );
};

class HaltIf final
	: public SyntaxElementBase

{
public:
	explicit HaltIf( const FilePos& file_pos );

	Expression condition;
};

class Typedef final
	: public SyntaxElementBase
	, public IProgramElement
	, public IClassElement
{
public:
	explicit Typedef( const FilePos& file_pos );

	ProgramString name;
	TypeName value;
};

class Enum final
	: public SyntaxElementBase
	, public IProgramElement
	, public IClassElement
{
public:
	explicit Enum( const FilePos& file_pos );

	struct Member
	{
		FilePos file_pos;
		ProgramString name;
	};

	ProgramString name;
	ComplexName underlaying_type_name;
	std::vector<Member> members;
};

enum class VirtualFunctionKind
{
	None, // Regular, non-virtual
	DeclareVirtual,
	VirtualOverride,
	VirtualFinal,
	VirtualPure,
};

class Function final
	: public SyntaxElementBase
	, public IProgramElement
	, public IClassElement
{
public:
	Function( const FilePos& file_pos );

	ComplexName name_;
	Expression condition_;
	FunctionType type_;
	std::unique_ptr<StructNamedInitializer> constructor_initialization_list_;
	BlockPtr block_;
	OverloadedOperator overloaded_operator_= OverloadedOperator::None;
	VirtualFunctionKind virtual_function_kind_= VirtualFunctionKind::None;
	bool no_mangle_= false;
	bool is_conversion_constructor_= false;

	enum class BodyKind
	{
		None,
		BodyGenerationRequired,
		BodyGenerationDisabled,
	};
	BodyKind body_kind= BodyKind::None;

	bool constexpr_= false;
};

class ClassField final
	: public SyntaxElementBase
	, public IClassElement
{
public:
	explicit ClassField( const FilePos& file_pos );

	TypeName type;
	ProgramString name;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
	InitializerPtr initializer; // May be null.
};

enum class ClassKindAttribute
{
	Struct,
	Class,
	Final,
	Polymorph,
	Interface,
	Abstract,
};

enum class ClassMemberVisibility
{
	// Must be ordered from less access to more access.
	Public,
	Protected,
	Private,
};

class ClassVisibilityLabel final
	: public SyntaxElementBase
	, public IClassElement
{
public:
	ClassVisibilityLabel( const FilePos& file_pos, ClassMemberVisibility visibility );

	const ClassMemberVisibility visibility_;
};

class Class final
	: public SyntaxElementBase
	, public IProgramElement
	, public IClassElement
{
public:
	explicit Class( const FilePos& file_pos );

	ClassElements elements_;
	ProgramString name_;
	bool is_forward_declaration_= false;
	ClassKindAttribute kind_attribute_ = ClassKindAttribute::Struct;
	std::vector<ComplexName> parents_;
};

class TemplateBase : public SyntaxElementBase
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

typedef std::unique_ptr<TemplateBase> TemplateBasePtr;

class TypeTemplateBase : public TemplateBase
{
public:
	explicit TypeTemplateBase( const FilePos& file_pos );

	// Argument in template signature.
	struct SignatureArg
	{
		Expression name;
		Expression default_value;
	};

	std::vector<SignatureArg> signature_args_;
	ProgramString name_;

	// Short form means that template argumenst are also signature arguments.
	bool is_short_form_= false;
};

typedef std::unique_ptr<TypeTemplateBase> TypeTemplateBasePtr;

class ClassTemplate final
	: public TypeTemplateBase
	, public IProgramElement
	, public IClassElement
{
public:
	explicit ClassTemplate( const FilePos& file_pos );

	std::unique_ptr<Class> class_;
};

class TypedefTemplate final
	: public TypeTemplateBase
	, public IProgramElement
	, public IClassElement
{
public:
	explicit TypedefTemplate( const FilePos& file_pos );

	std::unique_ptr<Typedef> typedef_;
};

class FunctionTemplate final
	: public TemplateBase
	, public IProgramElement
	, public IClassElement
{
public:
	explicit FunctionTemplate( const FilePos& file_pos );

	std::unique_ptr<Function> function_;
};

class Namespace final
	: public SyntaxElementBase
	, public IProgramElement
{
public:
	explicit Namespace( const FilePos& file_pos );

	ProgramString name_;
	ProgramElements elements_;
};

class Import final : public SyntaxElementBase
{
public:
	explicit Import( const FilePos& file_pos );

	ProgramString import_name;
};

} // namespace Synt

} // namespace U
