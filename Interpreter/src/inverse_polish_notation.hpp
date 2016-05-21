#pragma once

#include "syntax_elements.hpp"

namespace Interpreter
{

struct InversePolishNotationComponent final
{
	// If operand is zero - is operator.
	IBinaryOperatorsChainComponentPtr operand;
	PrefixOperators prefix_operand_operators;
	PostfixOperators postfix_operand_operators;

	BinaryOperator operator_= BinaryOperator::None;

	void Print( std::ostream& stream ) const;
};

typedef std::vector<InversePolishNotationComponent> InversePolishNotation;

InversePolishNotation ConvertToInversePolishNotation( const BinaryOperatorsChain& binary_operators_chain );

void PrintInversePolishNotation( std::ostream& stream, const InversePolishNotation& ipn );

} // namespace Interpreter
