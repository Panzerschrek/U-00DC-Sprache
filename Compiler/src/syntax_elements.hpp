#pragma once
#include <limits>
#include <memory>
#include <vector>

#include <boost/variant.hpp>

#include "lexical_analyzer.hpp"

namespace U
{

class SyntaxElementBase
{
public:
	explicit SyntaxElementBase( const FilePos& file_pos );
	virtual ~SyntaxElementBase(){}

	FilePos file_pos_;
};

class IExpressionComponent;
typedef std::unique_ptr<IExpressionComponent> IExpressionComponentPtr;
class ExpressionComponentWithUnaryOperators;
typedef std::unique_ptr<ExpressionComponentWithUnaryOperators> ExpressionComponentWithUnaryOperatorsPtr;

class IUnaryPrefixOperator ;
class IUnaryPostfixOperator;

typedef std::unique_ptr<IUnaryPrefixOperator > IUnaryPrefixOperatorPtr ;
typedef std::unique_ptr<IUnaryPostfixOperator> IUnaryPostfixOperatorPtr;

typedef std::vector<IUnaryPrefixOperatorPtr > PrefixOperators ;
typedef std::vector<IUnaryPostfixOperatorPtr> PostfixOperators;

class IInitializer;
typedef std::unique_ptr<IInitializer> IInitializerPtr;

struct ComplexName final
{
	// A
	// A::b
	// TheClass::Method
	// ::Absolute::Name
	// ::C_Function

	// If first component is empty, name starts with "::".
	std::vector<ProgramString> components;
};

class IUnaryPrefixOperator : public SyntaxElementBase
{
public:
	explicit IUnaryPrefixOperator( const FilePos& file_pos );
	virtual ~IUnaryPrefixOperator() {}
};

class IUnaryPostfixOperator : public SyntaxElementBase
{
public:
	explicit IUnaryPostfixOperator( const FilePos& file_pos );
	virtual ~IUnaryPostfixOperator() {}
};

class UnaryPlus final : public IUnaryPrefixOperator
{
public:
	explicit UnaryPlus( const FilePos& file_pos );
	virtual ~UnaryPlus() override;
};

class UnaryMinus final : public IUnaryPrefixOperator
{
public:
	explicit UnaryMinus( const FilePos& file_pos );
	virtual ~UnaryMinus() override;
};

class LogicalNot final : public IUnaryPrefixOperator
{
public:
	explicit LogicalNot( const FilePos& file_pos );
	virtual ~LogicalNot() override;
};

class BitwiseNot final : public IUnaryPrefixOperator
{
public:
	explicit BitwiseNot( const FilePos& file_pos );
	virtual ~BitwiseNot() override;
};

class CallOperator final : public IUnaryPostfixOperator
{
public:
	CallOperator(
		const FilePos& file_pos,
		std::vector<IExpressionComponentPtr> arguments );
	virtual ~CallOperator() override;

	const std::vector<IExpressionComponentPtr> arguments_;
};

class IndexationOperator final : public IUnaryPostfixOperator
{
public:
	explicit IndexationOperator( const FilePos& file_pos, IExpressionComponentPtr index );
	virtual ~IndexationOperator() override;

	const IExpressionComponentPtr index_;
};

class MemberAccessOperator final : public IUnaryPostfixOperator
{
public:
	MemberAccessOperator( const FilePos& file_pos, ProgramString member_name );
	virtual ~MemberAccessOperator() override;

	const ProgramString member_name_;
};

enum class BinaryOperatorType
{
	Add,
	Sub,
	Mul,
	Div,
	Rem,

	Equal,
	NotEqual,
	Less,
	LessEqual,
	Greater,
	GreaterEqual,

	And,
	Or,
	Xor,

	ShiftLeft ,
	ShiftRight,

	LazyLogicalAnd,
	LazyLogicalOr,

	Last,
};

ProgramString BinaryOperatorToString( BinaryOperatorType op );

class IExpressionComponent;
typedef std::unique_ptr<IExpressionComponent> IExpressionComponentPtr;

class IExpressionComponent : public SyntaxElementBase
{
public:
	explicit IExpressionComponent( const FilePos& file_pos );
	virtual ~IExpressionComponent(){}
};

class IInitializer : public SyntaxElementBase
{
public:
	explicit IInitializer( const FilePos& file_pos );
	virtual ~IInitializer() override= default;
};

class ArrayInitializer final : public IInitializer
{
public:
	explicit ArrayInitializer( const FilePos& file_pos );
	virtual ~ArrayInitializer() override= default;

	std::vector<IInitializerPtr> initializers;
	bool has_continious_initializer= false; // ... after last initializator.
};

class StructNamedInitializer final : public IInitializer
{
public:
	explicit StructNamedInitializer( const FilePos& file_pos );
	virtual ~StructNamedInitializer() override= default;

	struct MemberInitializer
	{
		ProgramString name;
		IInitializerPtr initializer;
	};

	std::vector<MemberInitializer> members_initializers;
};

class ConstructorInitializer final : public IInitializer
{
public:
	ConstructorInitializer(
		const FilePos& file_pos,
		std::vector<IExpressionComponentPtr> arguments );
	virtual ~ConstructorInitializer() override= default;

	const CallOperator call_operator;
};

class ExpressionInitializer final : public IInitializer
{
public:
	ExpressionInitializer( const FilePos& file_pos, IExpressionComponentPtr expression );
	virtual ~ExpressionInitializer() override= default;

	IExpressionComponentPtr expression;
};

class ZeroInitializer final : public IInitializer
{
public:
	explicit ZeroInitializer( const FilePos& file_pos );
	virtual ~ZeroInitializer() override= default;
};

class BinaryOperator final : public IExpressionComponent
{
public:
	explicit BinaryOperator( const FilePos& file_pos );
	virtual ~BinaryOperator() override= default;

	BinaryOperatorType operator_type_;
	IExpressionComponentPtr left_;
	IExpressionComponentPtr right_;
};

class ExpressionComponentWithUnaryOperators : public IExpressionComponent
{
public:
	explicit ExpressionComponentWithUnaryOperators( const FilePos& file_pos );
	virtual ~ExpressionComponentWithUnaryOperators() override= default;

	std::vector<IUnaryPrefixOperatorPtr > prefix_operators_ ;
	std::vector<IUnaryPostfixOperatorPtr> postfix_operators_;
};

class NamedOperand final : public ExpressionComponentWithUnaryOperators
{
public:
	NamedOperand( const FilePos& file_pos, ComplexName name );
	virtual ~NamedOperand() override;

	const ComplexName name_;
};

class BooleanConstant final : public ExpressionComponentWithUnaryOperators
{
public:
	BooleanConstant( const FilePos& file_pos, bool value );
	virtual ~BooleanConstant() override;

	const bool value_;
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

	virtual ~NumericConstant() override;

	const LongFloat value_;
	const ProgramString type_suffix_;
	const bool has_fractional_point_;
};

class BracketExpression final : public ExpressionComponentWithUnaryOperators
{
public:
	BracketExpression( const FilePos& file_pos, IExpressionComponentPtr expression );
	~BracketExpression() override;

	const IExpressionComponentPtr expression_;
};

class IProgramElement
{
public:
	virtual ~IProgramElement(){}
};

typedef std::unique_ptr<IProgramElement> IProgramElementPtr;
typedef std::vector<IProgramElementPtr> ProgramElements;

class IBlockElement : public SyntaxElementBase
{
public:
	explicit IBlockElement( const FilePos& file_pos );
	virtual ~IBlockElement(){}
};

typedef std::unique_ptr<IBlockElement> IBlockElementPtr;
typedef std::vector<IBlockElementPtr> BlockElements;

class Block final : public IBlockElement
{
public:
	Block( const FilePos& file_pos, BlockElements elements );
	virtual ~Block() override;

public:
	const BlockElements elements_;
};

typedef std::unique_ptr<Block> BlockPtr;

struct TypeName
{
	// [ [i32, 5] 7 ]
	ComplexName name; // Can be empty in some cases.
	std::vector< IExpressionComponentPtr > array_sizes;

	// Compiler so stupid - can not generate move constructors without noexcept. Make help for it.
	TypeName() = default;
	TypeName( TypeName&& ) noexcept = default;
	TypeName( const TypeName& )= default;

	TypeName& operator=( const TypeName& )= default;
	TypeName& operator=( TypeName&& )= default;
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
	// TODO - add "move" references here
};

struct VariablesDeclaration final : public IBlockElement
{
	virtual ~VariablesDeclaration() override;

	VariablesDeclaration( const FilePos& file_pos );
	VariablesDeclaration( const VariablesDeclaration& )= delete;
	VariablesDeclaration( VariablesDeclaration&& other );

	VariablesDeclaration operator=( const VariablesDeclaration& )= delete;
	VariablesDeclaration& operator=( VariablesDeclaration&& other );

	struct VariableEntry
	{
		ProgramString name;
		IInitializerPtr initializer; // May be null for types with default constructor.
		MutabilityModifier mutability_modifier= MutabilityModifier::None;
		ReferenceModifier reference_modifier= ReferenceModifier::None;
	};

	std::vector<VariableEntry> variables;
	TypeName type; // Type with empty name for auto-type detection.
};

typedef std::unique_ptr<VariablesDeclaration> VariablesDeclarationPtr;

struct AutoVariableDeclaration final : public IBlockElement
{
	explicit AutoVariableDeclaration( const FilePos& file_pos );
	virtual ~AutoVariableDeclaration() override= default;

	ProgramString name;
	IExpressionComponentPtr initializer_expression;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;
};

class ReturnOperator final : public IBlockElement
{
public:
	ReturnOperator( const FilePos& file_pos, IExpressionComponentPtr expression );
	~ReturnOperator() override;

	const IExpressionComponentPtr expression_;
};

class WhileOperator final : public IBlockElement
{
public:
	WhileOperator( const FilePos& file_pos, IExpressionComponentPtr condition, BlockPtr block );
	~WhileOperator() override;

	const IExpressionComponentPtr condition_;
	const BlockPtr block_;
};

class BreakOperator final : public IBlockElement
{
public:
	explicit BreakOperator( const FilePos& file_pos );
	~BreakOperator() override;
};

class ContinueOperator final : public IBlockElement
{
public:
	explicit ContinueOperator( const FilePos& file_pos );
	~ContinueOperator() override;
};

class IfOperator final : public IBlockElement
{
public:
	struct Branch
	{
		// Condition - nullptr for last if.
		IExpressionComponentPtr condition;
		BlockPtr block;
	};

	IfOperator( const FilePos& file_pos, std::vector<Branch> branches );

	~IfOperator() override;

	std::vector<Branch> branches_; // else if()
};

class SingleExpressionOperator final : public IBlockElement
{
public:
	SingleExpressionOperator( const FilePos& file_pos, IExpressionComponentPtr expression );
	virtual ~SingleExpressionOperator() override;

	const IExpressionComponentPtr expression_;
};

class AssignmentOperator final : public IBlockElement
{
public:
	AssignmentOperator( const FilePos& file_pos, IExpressionComponentPtr l_value, IExpressionComponentPtr r_value );
	virtual ~AssignmentOperator() override;

	IExpressionComponentPtr l_value_;
	IExpressionComponentPtr r_value_;
};

class AdditiveAssignmentOperator final : public IBlockElement
{
public:
	explicit AdditiveAssignmentOperator( const FilePos& file_pos );

	IExpressionComponentPtr l_value_;
	IExpressionComponentPtr r_value_;
	BinaryOperatorType additive_operation_;
};

class IncrementOperator final : public IBlockElement
{
public:
	explicit IncrementOperator( const FilePos& file_pos );

	IExpressionComponentPtr expression;
};

class DecrementOperator final : public IBlockElement
{
public:
	explicit DecrementOperator( const FilePos& file_pos );

	IExpressionComponentPtr expression;
};

class StaticAssert final : public IBlockElement
{
public:
	explicit StaticAssert( const FilePos& file_pos );

	IExpressionComponentPtr expression;
};

class FunctionArgumentDeclaration final : public SyntaxElementBase
{
public:
	FunctionArgumentDeclaration(
		const FilePos& file_pos,
		ProgramString name,
		TypeName type,
		MutabilityModifier mutability_modifier,
		ReferenceModifier reference_modifier );

	virtual ~FunctionArgumentDeclaration() override;

public:
	const ProgramString name_;
	const TypeName type_;
	const MutabilityModifier mutability_modifier_;
	const ReferenceModifier reference_modifier_;
};


typedef std::unique_ptr<FunctionArgumentDeclaration> FunctionArgumentDeclarationPtr;
typedef std::vector<FunctionArgumentDeclarationPtr> FunctionArgumentsDeclaration;

class FunctionDeclaration final
	: public SyntaxElementBase
	, public IProgramElement
{
public:
	FunctionDeclaration(
		const FilePos& file_pos,
		ComplexName name,
		TypeName return_type,
		MutabilityModifier return_value_mutability_modifier,
		ReferenceModifier return_value_reference_modifier,
		FunctionArgumentsDeclaration arguments,
		std::unique_ptr<StructNamedInitializer> constructor_initialization_list,
		BlockPtr block );

	virtual ~FunctionDeclaration() override;

	const ComplexName name_;
	const TypeName return_type_;
	const MutabilityModifier return_value_mutability_modifier_;
	const ReferenceModifier return_value_reference_modifier_;
	const FunctionArgumentsDeclaration arguments_;
	const std::unique_ptr<StructNamedInitializer> constructor_initialization_list_;
	const BlockPtr block_;
};

class ClassDeclaration final
	: public SyntaxElementBase
	, public IProgramElement
{
public:
	explicit ClassDeclaration( const FilePos& file_pos );
	virtual ~ClassDeclaration() override;

	struct Field
	{
		FilePos file_pos;
		TypeName type;
		ProgramString name;
	};

	typedef
		boost::variant<
			std::unique_ptr<FunctionDeclaration>,
			std::unique_ptr<ClassDeclaration>,
			Field >
		Member;

	std::vector<Member> members_;
	ComplexName name_;
	bool is_forward_declaration_= false;
};

class ClassTemplateDeclaration final
	: public SyntaxElementBase
	, public IProgramElement
{
public:
	explicit ClassTemplateDeclaration( const FilePos& file_pos );

	// For type arguments, like template</ type A, type B />, arg_type is empty.
	// For value arguments, like template</ type A, A x, i32 y />, arg_type is comples name of argument.
	struct Arg
	{
		ComplexName arg_type;
		ProgramString name;
	};

	// Argument in template signature.
	struct SignatureArg
	{
		// TODO - support more comples names, like std::vector</T/>.
		// TODO - support default arguments.
		ProgramString name;
	};

	std::vector<Arg> args_;
	std::vector<SignatureArg> signature_args_;
	std::unique_ptr<ClassDeclaration> class_;
};

class Namespace final
	: public SyntaxElementBase
	, public IProgramElement
{
public:
	explicit Namespace( const FilePos& file_pos );
	virtual ~Namespace() override= default;

	ProgramString name_;
	ProgramElements elements_;
};

} // namespace U
