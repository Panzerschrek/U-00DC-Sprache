#pragma once

#include "syntax_elements.hpp"

namespace Interpreter
{

struct ReversePolishNotationComponent final
{
	// If operand is zero - is operator.
	IBinaryOperatorsChainComponentPtr operand;
	BinaryOperator operator_= BinaryOperator::None;

	void Print( std::ostream& stream ) const;
};

typedef std::vector<ReversePolishNotationComponent> InversePolishNotation;

InversePolishNotation ConvertToInversePolishNotation( const BinaryOperatorsChain& binary_operators_chain );

void PrintInversePolishNotation( std::ostream& stream, const InversePolishNotation& ipn );

} // namespace Interpreter
