#pragma once
#include <memory>
#include <vector>

#include "lexical_analyzer.hpp"

namespace Interpreter
{

struct BinaryOperatorsChain;
typedef std::unique_ptr<BinaryOperatorsChain> BinaryOperatorsChainPtr;

class IUnaryPrefixOperator
{
public:
	virtual ~IUnaryPrefixOperator() {}
};

class IUnaryPostfixOperator
{
public:
	virtual ~IUnaryPostfixOperator() {}
};

typedef std::unique_ptr<IUnaryPrefixOperator > IUnaryPrefixOperatorPtr ;
typedef std::unique_ptr<IUnaryPostfixOperator> IUnaryPostfixOperatorPtr;

typedef std::vector<IUnaryPrefixOperatorPtr > PrefixOperators ;
typedef std::vector<IUnaryPostfixOperatorPtr> PostfixOperators;

class UnaryPlus final : public IUnaryPrefixOperator
{
public:
	virtual ~UnaryPlus() override;
};

class UnaryMinus final : public IUnaryPrefixOperator
{
public:
	virtual ~UnaryMinus() override;
};

class CallOperator final : public IUnaryPostfixOperator
{
public:
	CallOperator( std::vector<BinaryOperatorsChainPtr> arguments );
	virtual ~CallOperator() override;

private:
	const std::vector<BinaryOperatorsChainPtr> arguments_;
};

class IndexationOperator final : public IUnaryPostfixOperator
{
public:
	IndexationOperator( BinaryOperatorsChainPtr index );
	virtual ~IndexationOperator() override;

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
};

class IBinaryOperatorsChainComponent
{
public:
	virtual ~IBinaryOperatorsChainComponent(){}
};

typedef std::unique_ptr<IBinaryOperatorsChainComponent> IBinaryOperatorsChainComponentPtr;

class NamedOperand final : public IBinaryOperatorsChainComponent
{
public:
	NamedOperand( ProgramString name );
	virtual ~NamedOperand() override;

private:
	const ProgramString name_;
};

class NumericConstant final : public IBinaryOperatorsChainComponent
{
public:
	NumericConstant( ProgramString value );
	virtual ~NumericConstant() override;

private:
	const ProgramString value_;
};

class BracketExpression final : public IBinaryOperatorsChainComponent
{
public:
	BracketExpression( BinaryOperatorsChainPtr expression );
	~BracketExpression() override;

private:
	const BinaryOperatorsChainPtr expression_;
};

struct BinaryOperatorsChain final
{
	struct ComponentWithOperator
	{
		PrefixOperators prefix_operators;
		IBinaryOperatorsChainComponentPtr component;
		PostfixOperators postfix_operators;

		BinaryOperator op= BinaryOperator::None;
	};

	std::vector<ComponentWithOperator> components;
};

class IProgramElement
{
public:
	virtual ~IProgramElement(){}
};

typedef std::unique_ptr<IProgramElement> IProgramElementPtr;
typedef std::vector<IProgramElementPtr> ProgramElements;

class IBlockElement
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

private:
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

	ProgramString name;
	ProgramString type;
	BinaryOperatorsChainPtr initial_value;
};

typedef std::unique_ptr<VariableDeclaration> VariableDeclarationPtr;

class FunctionDeclaration final : public IProgramElement
{
public:
	FunctionDeclaration(
		ProgramString name,
		ProgramString return_type,
		std::vector<VariableDeclaration> arguments,
		BlockPtr block );

	virtual ~FunctionDeclaration() override;

private:
	const ProgramString name_;
	const ProgramString return_type_;
	const std::vector<VariableDeclaration> arguments_;
	const BlockPtr block_;
};

} // namespace Interpreter
