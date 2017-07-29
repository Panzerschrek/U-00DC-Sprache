#pragma once
#include <limits>
#include <memory>
#include <ostream>
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

	virtual void Print( std::ostream& stream, unsigned int indent ) const= 0;

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

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;
};

class UnaryMinus final : public IUnaryPrefixOperator
{
public:
	explicit UnaryMinus( const FilePos& file_pos );
	virtual ~UnaryMinus() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;
};

class CallOperator final : public IUnaryPostfixOperator
{
public:
	CallOperator(
		const FilePos& file_pos,
		std::vector<IExpressionComponentPtr> arguments );
	virtual ~CallOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const std::vector<IExpressionComponentPtr> arguments_;
};

class IndexationOperator final : public IUnaryPostfixOperator
{
public:
	explicit IndexationOperator( const FilePos& file_pos, IExpressionComponentPtr index );
	virtual ~IndexationOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const IExpressionComponentPtr index_;
};

class MemberAccessOperator final : public IUnaryPostfixOperator
{
public:
	MemberAccessOperator( const FilePos& file_pos, ProgramString member_name );
	virtual ~MemberAccessOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const ProgramString member_name_;
};

enum class BinaryOperatorType
{
	Add,
	Sub,
	Div,
	Mul,

	Equal,
	NotEqual,
	Less,
	LessEqual,
	Greater,
	GreaterEqual,

	And,
	Or,
	Xor,

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

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	std::vector<IInitializerPtr> initializers;
	bool has_continious_initializer= false; // ... after last initializator.
};

class StructNamedInitializer final : public IInitializer
{
public:
	explicit StructNamedInitializer( const FilePos& file_pos );
	virtual ~StructNamedInitializer() override= default;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

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

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const CallOperator call_operator;
};

class ExpressionInitializer final : public IInitializer
{
public:
	ExpressionInitializer( const FilePos& file_pos, IExpressionComponentPtr expression );
	virtual ~ExpressionInitializer() override= default;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	IExpressionComponentPtr expression;
};

class ZeroInitializer final : public IInitializer
{
public:
	explicit ZeroInitializer( const FilePos& file_pos );
	virtual ~ZeroInitializer() override= default;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;
};

class BinaryOperator final : public IExpressionComponent
{
public:
	explicit BinaryOperator( const FilePos& file_pos );
	virtual ~BinaryOperator() override= default;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

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
	NamedOperand( const FilePos& file_pos, ProgramString name );
	virtual ~NamedOperand() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const ProgramString name_;
};

class BooleanConstant final : public ExpressionComponentWithUnaryOperators
{
public:
	BooleanConstant( const FilePos& file_pos, bool value );
	virtual ~BooleanConstant() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

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

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const LongFloat value_;
	const ProgramString type_suffix_;
	const bool has_fractional_point_;
};

class BracketExpression final : public ExpressionComponentWithUnaryOperators
{
public:
	BracketExpression( const FilePos& file_pos, IExpressionComponentPtr expression );
	~BracketExpression() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const IExpressionComponentPtr expression_;
};

class IProgramElement : public SyntaxElementBase
{
public:
	explicit IProgramElement( const FilePos& file_pos );
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

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

public:
	const BlockElements elements_;
};

typedef std::unique_ptr<Block> BlockPtr;

struct TypeName
{
	// [ [i32, 5] 7 ]
	ProgramString name;
	std::vector< std::unique_ptr<NumericConstant> > array_sizes;

	void Print( std::ostream& stream ) const;

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

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

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
	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

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

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const IExpressionComponentPtr expression_;
};

class WhileOperator final : public IBlockElement
{
public:
	WhileOperator( const FilePos& file_pos, IExpressionComponentPtr condition, BlockPtr block );
	~WhileOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const IExpressionComponentPtr condition_;
	const BlockPtr block_;
};

class BreakOperator final : public IBlockElement
{
public:
	explicit BreakOperator( const FilePos& file_pos );
	~BreakOperator() override;
	virtual void Print( std::ostream& stream, unsigned int indent ) const override;
};

class ContinueOperator final : public IBlockElement
{
public:
	explicit ContinueOperator( const FilePos& file_pos );
	~ContinueOperator() override;
	virtual void Print( std::ostream& stream, unsigned int indent ) const override;
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

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	std::vector<Branch> branches_; // else if()
};

class SingleExpressionOperator final : public IBlockElement
{
public:
	SingleExpressionOperator( const FilePos& file_pos, IExpressionComponentPtr expression );
	virtual ~SingleExpressionOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const IExpressionComponentPtr expression_;
};

class AssignmentOperator final : public IBlockElement
{
public:
	AssignmentOperator( const FilePos& file_pos, IExpressionComponentPtr l_value, IExpressionComponentPtr r_value );
	virtual ~AssignmentOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	IExpressionComponentPtr l_value_;
	IExpressionComponentPtr r_value_;
};

class FunctionArgumentDeclaration final : public IProgramElement
{
public:
	FunctionArgumentDeclaration(
		const FilePos& file_pos,
		ProgramString name,
		TypeName type,
		MutabilityModifier mutability_modifier,
		ReferenceModifier reference_modifier );

	virtual ~FunctionArgumentDeclaration() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

public:
	const ProgramString name_;
	const TypeName type_;
	const MutabilityModifier mutability_modifier_;
	const ReferenceModifier reference_modifier_;
};


typedef std::unique_ptr<FunctionArgumentDeclaration> FunctionArgumentDeclarationPtr;
typedef std::vector<FunctionArgumentDeclarationPtr> FunctionArgumentsDeclaration;

class FunctionDeclaration final : public IProgramElement
{
public:
	FunctionDeclaration(
		const FilePos& file_pos,
		ProgramString name,
		ProgramString return_type,
		MutabilityModifier return_value_mutability_modifier,
		ReferenceModifier return_value_reference_modifier,
		FunctionArgumentsDeclaration arguments,
		BlockPtr block );

	virtual ~FunctionDeclaration() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const ProgramString name_;
	const ProgramString return_type_;
	const MutabilityModifier return_value_mutability_modifier_;
	const ReferenceModifier return_value_reference_modifier_;
	const FunctionArgumentsDeclaration arguments_;
	const BlockPtr block_;
};

class ClassDeclaration final : public IProgramElement
{
public:
	explicit ClassDeclaration( const FilePos& file_pos );
	virtual ~ClassDeclaration() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	struct Field
	{
		FilePos file_pos;
		TypeName type;
		ProgramString name;
	};

	typedef boost::variant< std::unique_ptr<FunctionDeclaration>, Field > Member;

	std::vector<Member> members_;
	ProgramString name_;
};

} // namespace U
