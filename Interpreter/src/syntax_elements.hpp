#pragma once
#include <limits>
#include <memory>
#include <ostream>
#include <vector>

#include "lexical_analyzer.hpp"

namespace Interpreter
{

class SyntaxElementBase
{
public:
	explicit SyntaxElementBase( const FilePos& file_pos );
	virtual ~SyntaxElementBase(){}

	virtual void Print( std::ostream& stream, unsigned int indent ) const= 0;

	FilePos file_pos_;
};

struct BinaryOperatorsChain;
typedef std::unique_ptr<BinaryOperatorsChain> BinaryOperatorsChainPtr;

class IUnaryPrefixOperator ;
class IUnaryPostfixOperator;

typedef std::unique_ptr<IUnaryPrefixOperator > IUnaryPrefixOperatorPtr ;
typedef std::unique_ptr<IUnaryPostfixOperator> IUnaryPostfixOperatorPtr;

typedef std::vector<IUnaryPrefixOperatorPtr > PrefixOperators ;
typedef std::vector<IUnaryPostfixOperatorPtr> PostfixOperators;

class IUnaryPrefixOperator : public SyntaxElementBase
{
public:
	explicit IUnaryPrefixOperator( const FilePos& file_pos );
	virtual ~IUnaryPrefixOperator() {}

	virtual IUnaryPrefixOperatorPtr Clone() const= 0;
};

class IUnaryPostfixOperator : public SyntaxElementBase
{
public:
	explicit IUnaryPostfixOperator( const FilePos& file_pos );
	virtual ~IUnaryPostfixOperator() {}

	virtual IUnaryPostfixOperatorPtr Clone() const= 0;
};

class UnaryPlus final : public IUnaryPrefixOperator
{
public:
	explicit UnaryPlus( const FilePos& file_pos );
	virtual ~UnaryPlus() override;

	virtual IUnaryPrefixOperatorPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;
};

class UnaryMinus final : public IUnaryPrefixOperator
{
public:
	explicit UnaryMinus( const FilePos& file_pos );
	virtual ~UnaryMinus() override;

	virtual IUnaryPrefixOperatorPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;
};

class CallOperator final : public IUnaryPostfixOperator
{
public:
	CallOperator(
		const FilePos& file_pos,
		std::vector<BinaryOperatorsChainPtr> arguments );
	virtual ~CallOperator() override;

	virtual IUnaryPostfixOperatorPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const std::vector<BinaryOperatorsChainPtr> arguments_;
};

class IndexationOperator final : public IUnaryPostfixOperator
{
public:
	explicit IndexationOperator( const FilePos& file_pos, BinaryOperatorsChainPtr index );
	virtual ~IndexationOperator() override;

	virtual IUnaryPostfixOperatorPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const BinaryOperatorsChainPtr index_;
};

enum class BinaryOperator
{
	None, // Special value - for end of binary operators chain.

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

void PrintOperator( std::ostream& stream, BinaryOperator op );

class IBinaryOperatorsChainComponent;
typedef std::unique_ptr<IBinaryOperatorsChainComponent> IBinaryOperatorsChainComponentPtr;

class IBinaryOperatorsChainComponent : public SyntaxElementBase
{
public:
	explicit IBinaryOperatorsChainComponent( const FilePos& file_pos );
	virtual ~IBinaryOperatorsChainComponent(){}

	virtual IBinaryOperatorsChainComponentPtr Clone() const= 0;
};

class NamedOperand final : public IBinaryOperatorsChainComponent
{
public:
	NamedOperand( const FilePos& file_pos, ProgramString name );
	virtual ~NamedOperand() override;

	virtual IBinaryOperatorsChainComponentPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const ProgramString name_;
};

class BooleanConstant final : public IBinaryOperatorsChainComponent
{
public:
	BooleanConstant( const FilePos& file_pos, bool value );
	virtual ~BooleanConstant() override;

	virtual IBinaryOperatorsChainComponentPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const bool value_;
};

class NumericConstant final : public IBinaryOperatorsChainComponent
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

	virtual IBinaryOperatorsChainComponentPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const LongFloat value_;
	const ProgramString type_suffix_;
	const bool has_fractional_point_;
};

class BracketExpression final : public IBinaryOperatorsChainComponent
{
public:
	BracketExpression( const FilePos& file_pos, BinaryOperatorsChainPtr expression );
	~BracketExpression() override;

	virtual IBinaryOperatorsChainComponentPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const BinaryOperatorsChainPtr expression_;
};

struct BinaryOperatorsChain final : public SyntaxElementBase
{
	explicit BinaryOperatorsChain( const FilePos& file_pos );

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	struct ComponentWithOperator
	{
		PrefixOperators prefix_operators;
		IBinaryOperatorsChainComponentPtr component;
		PostfixOperators postfix_operators;

		BinaryOperator op= BinaryOperator::None;

		ComponentWithOperator();
		ComponentWithOperator( const ComponentWithOperator& other );
		ComponentWithOperator( ComponentWithOperator&& other );

		ComponentWithOperator& operator=( const ComponentWithOperator& other );
		ComponentWithOperator& operator=( ComponentWithOperator&& other );
	};

	std::vector<ComponentWithOperator> components;
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
};

struct VariableDeclaration final : public IBlockElement
{
	virtual ~VariableDeclaration() override;

	VariableDeclaration( const FilePos& file_pos );
	VariableDeclaration( const VariableDeclaration& )= delete;
	VariableDeclaration( VariableDeclaration&& other );

	VariableDeclaration operator=( const VariableDeclaration& )= delete;
	VariableDeclaration& operator=( VariableDeclaration&& other );

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	ProgramString name;
	TypeName type;
	BinaryOperatorsChainPtr initial_value;
};

typedef std::unique_ptr<VariableDeclaration> VariableDeclarationPtr;

class ReturnOperator final : public IBlockElement
{
public:
	ReturnOperator( const FilePos& file_pos, BinaryOperatorsChainPtr expression );
	~ReturnOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const BinaryOperatorsChainPtr expression_;
};

class WhileOperator final : public IBlockElement
{
public:
	WhileOperator( const FilePos& file_pos, BinaryOperatorsChainPtr condition, BlockPtr block );
	~WhileOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const BinaryOperatorsChainPtr condition_;
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
		BinaryOperatorsChainPtr condition;
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
	SingleExpressionOperator( const FilePos& file_pos, BinaryOperatorsChainPtr expression );
	virtual ~SingleExpressionOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const BinaryOperatorsChainPtr expression_;
};

class AssignmentOperator final : public IBlockElement
{
public:
	AssignmentOperator( const FilePos& file_pos, BinaryOperatorsChainPtr l_value, BinaryOperatorsChainPtr r_value );
	virtual ~AssignmentOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	BinaryOperatorsChainPtr l_value_;
	BinaryOperatorsChainPtr r_value_;
};

class FunctionDeclaration final : public IProgramElement
{
public:
	FunctionDeclaration(
		const FilePos& file_pos,
		ProgramString name,
		ProgramString return_type,
		std::vector<VariableDeclaration> arguments,
		BlockPtr block );

	virtual ~FunctionDeclaration() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const ProgramString name_;
	const ProgramString return_type_;
	const std::vector<VariableDeclaration> arguments_;
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
		TypeName type;
		ProgramString name;
	};

	std::vector<Field> fields_;
	ProgramString name_;
};

} // namespace Interpreter
