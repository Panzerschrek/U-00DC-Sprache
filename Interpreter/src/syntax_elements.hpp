#pragma once
#include <memory>
#include <ostream>
#include <vector>

#include "lexical_analyzer.hpp"

namespace Interpreter
{

class IPrintable
{
public:
	virtual ~IPrintable(){}

	virtual void Print( std::ostream& stream, unsigned int indent ) const= 0;
};

struct BinaryOperatorsChain;
typedef std::unique_ptr<BinaryOperatorsChain> BinaryOperatorsChainPtr;

class IUnaryPrefixOperator ;
class IUnaryPostfixOperator;

typedef std::unique_ptr<IUnaryPrefixOperator > IUnaryPrefixOperatorPtr ;
typedef std::unique_ptr<IUnaryPostfixOperator> IUnaryPostfixOperatorPtr;

typedef std::vector<IUnaryPrefixOperatorPtr > PrefixOperators ;
typedef std::vector<IUnaryPostfixOperatorPtr> PostfixOperators;

class IUnaryPrefixOperator : public IPrintable
{
public:
	virtual ~IUnaryPrefixOperator() {}

	virtual IUnaryPrefixOperatorPtr Clone() const= 0;
};

class IUnaryPostfixOperator : public IPrintable
{
public:
	virtual ~IUnaryPostfixOperator() {}

	virtual IUnaryPostfixOperatorPtr Clone() const= 0;
};

class UnaryPlus final : public IUnaryPrefixOperator
{
public:
	virtual ~UnaryPlus() override;

	virtual IUnaryPrefixOperatorPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;
};

class UnaryMinus final : public IUnaryPrefixOperator
{
public:
	virtual ~UnaryMinus() override;

	virtual IUnaryPrefixOperatorPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;
};

class CallOperator final : public IUnaryPostfixOperator
{
public:
	explicit CallOperator( std::vector<BinaryOperatorsChainPtr> arguments );
	virtual ~CallOperator() override;

	virtual IUnaryPostfixOperatorPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

private:
	const std::vector<BinaryOperatorsChainPtr> arguments_;
};

class IndexationOperator final : public IUnaryPostfixOperator
{
public:
	explicit IndexationOperator( BinaryOperatorsChainPtr index );
	virtual ~IndexationOperator() override;

	virtual IUnaryPostfixOperatorPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

private:
	const BinaryOperatorsChainPtr index_;
};

enum class BinaryOperator
{
	None, // Special value - for end of binary operators chain.
	Add,
	Sub,
	Div,
	Mul,
	Last,
};

void PrintOperator( std::ostream& stream, BinaryOperator op );

class IBinaryOperatorsChainComponent;
typedef std::unique_ptr<IBinaryOperatorsChainComponent> IBinaryOperatorsChainComponentPtr;

class IBinaryOperatorsChainComponent : public IPrintable
{
public:
	virtual ~IBinaryOperatorsChainComponent(){}

	virtual IBinaryOperatorsChainComponentPtr Clone() const= 0;
};

class NamedOperand final : public IBinaryOperatorsChainComponent
{
public:
	explicit NamedOperand( ProgramString name );
	virtual ~NamedOperand() override;

	virtual IBinaryOperatorsChainComponentPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const ProgramString name_;
};

class NumericConstant final : public IBinaryOperatorsChainComponent
{
public:
	explicit NumericConstant( ProgramString value );
	virtual ~NumericConstant() override;

	virtual IBinaryOperatorsChainComponentPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

private:
	const ProgramString value_;
};

class BracketExpression final : public IBinaryOperatorsChainComponent
{
public:
	explicit BracketExpression( BinaryOperatorsChainPtr expression );
	~BracketExpression() override;

	virtual IBinaryOperatorsChainComponentPtr Clone() const override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

private:
	const BinaryOperatorsChainPtr expression_;
};

struct BinaryOperatorsChain final : public IPrintable
{
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

class IProgramElement : public IPrintable
{
public:
	virtual ~IProgramElement(){}
};

typedef std::unique_ptr<IProgramElement> IProgramElementPtr;
typedef std::vector<IProgramElementPtr> ProgramElements;

class IBlockElement : public IPrintable
{
public:
	virtual ~IBlockElement(){}
};

typedef std::unique_ptr<IBlockElement> IBlockElementPtr;
typedef std::vector<IBlockElementPtr> BlockElements;

class Block final : public IBlockElement
{
public:
	Block( BlockElements elements );
	virtual ~Block() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

public:
	const BlockElements elements_;
};

typedef std::unique_ptr<Block> BlockPtr;

struct VariableDeclaration final : public IBlockElement
{
	virtual ~VariableDeclaration() override;

	VariableDeclaration();
	VariableDeclaration( const VariableDeclaration& )= delete;
	VariableDeclaration( VariableDeclaration&& other );

	VariableDeclaration operator=( const VariableDeclaration& )= delete;
	VariableDeclaration& operator=( VariableDeclaration&& other );

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	ProgramString name;
	ProgramString type;
	BinaryOperatorsChainPtr initial_value;
};

typedef std::unique_ptr<VariableDeclaration> VariableDeclarationPtr;

class ReturnOperator final : public IBlockElement
{
public:
	ReturnOperator( BinaryOperatorsChainPtr expression );
	~ReturnOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

	const BinaryOperatorsChainPtr expression_;
};

class WhileOperator final : public IBlockElement
{
public:
	WhileOperator( BinaryOperatorsChainPtr condition, BlockPtr block );
	~WhileOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

private:
	const BinaryOperatorsChainPtr condition_;
	const BlockPtr block_;
};

class BreakOperator final : public IBlockElement
{
public:
	~BreakOperator() override;
	virtual void Print( std::ostream& stream, unsigned int indent ) const override;
};

class ContinueOperator final : public IBlockElement
{
public:
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

	IfOperator( std::vector<Branch> branches );

	~IfOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

private:
	std::vector<Branch> branches_; // else if()
};

class SingleExpressionOperator final : public IBlockElement
{
public:
	SingleExpressionOperator( BinaryOperatorsChainPtr expression );
	virtual ~SingleExpressionOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

private:
	const BinaryOperatorsChainPtr expression_;
};

class AssignmentOperator final : public IBlockElement
{
public:
	AssignmentOperator( BinaryOperatorsChainPtr l_value, BinaryOperatorsChainPtr r_value );
	virtual ~AssignmentOperator() override;

	virtual void Print( std::ostream& stream, unsigned int indent ) const override;

private:
	BinaryOperatorsChainPtr l_value_;
	BinaryOperatorsChainPtr r_value_;
};

class FunctionDeclaration final : public IProgramElement
{
public:
	FunctionDeclaration(
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

} // namespace Interpreter
