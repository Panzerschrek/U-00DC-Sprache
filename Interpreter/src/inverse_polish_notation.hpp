#pragma once

#include "syntax_elements.hpp"

namespace Interpreter
{

struct ReversePolishNotationComponent final
{
	// If operand is zero - is operator.
	IBinaryOperatorsChainComponentPtr operand;
	BinaryOperator operator_;

	void Print( std::ostream& stream );
};

typedef std::vector<ReversePolishNotationComponent> InversePolishNotation;

InversePolishNotation ConvertToInversePolishNotation( const BinaryOperatorsChain& binary_operators_chain );

} // namespace Interpreter
