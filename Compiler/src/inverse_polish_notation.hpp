#pragma once

#include "syntax_elements.hpp"

namespace U
{

struct InversePolishNotationComponent final
{
	// If operand is zero - is operator.
	IBinaryOperatorsChainComponentPtr operand;
	PrefixOperators prefix_operand_operators;
	PostfixOperators postfix_operand_operators;

	BinaryOperator operator_= BinaryOperator::None;

	static const constexpr unsigned int c_no_parent= ~0;

	// Index of parent component, or c_no_parent
	unsigned int l_index= c_no_parent;
	unsigned int r_index= c_no_parent;

	void Print( std::ostream& stream ) const;
};

typedef std::vector<InversePolishNotationComponent> InversePolishNotation;

InversePolishNotation ConvertToInversePolishNotation( const BinaryOperatorsChain& binary_operators_chain );

void PrintInversePolishNotation( std::ostream& stream, const InversePolishNotation& ipn );

} // namespace U
