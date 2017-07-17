#include <iostream>

#include "../inverse_polish_notation.hpp"

#include "tests.hpp"

namespace U
{

static IBinaryOperatorsChainComponentPtr Op( const char* name )
{
	return
		IBinaryOperatorsChainComponentPtr(
			new NamedOperand( FilePos{ 0, 0 }, ToProgramString( name) ) );
}

static void Print( const BinaryOperatorsChain& chain, const InversePolishNotation& ipl )
{
	std::cout << "\n";
	chain.Print( std::cout, 0 );
	std::cout << "  =>  ";
	PrintInversePolishNotation( std::cout, ipl );
	std::cout << "\n";
}

U_TEST(OneOperandExpressionTest)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(1);

	chain.components[0].component= Op( "a" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );

	U_TEST_ASSERT( notation.size() == 1 );
	U_TEST_ASSERT( notation[0].operand );

	Print( chain, notation );
}

U_TEST(TwoOperandExpressionTest)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(2);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Add;
	chain.components[1].component= Op( "b" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 3 );
	U_TEST_ASSERT( notation[0].operand );
	U_TEST_ASSERT( notation[1].operand );
	U_TEST_ASSERT( !notation[2].operand );
	U_TEST_ASSERT( notation[2].operator_ == chain.components[0].op );
}

U_TEST(LongFlatExpressionTest)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(4);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Add;
	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Add;
	chain.components[2].component= Op( "c" );
	chain.components[2].op= BinaryOperator::Add;
	chain.components[3].component= Op( "d" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 7 );
	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b

	U_TEST_ASSERT( !notation[2].operand ); // +
	U_TEST_ASSERT( notation[2].operator_ == BinaryOperator::Add );

	U_TEST_ASSERT( notation[3].operand ); // d

	U_TEST_ASSERT( !notation[4].operand ); // +
	U_TEST_ASSERT( notation[4].operator_ == BinaryOperator::Add );
}

U_TEST(TriOperandEqualExpressionTest)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(3);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Add;
	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Add;
	chain.components[2].component= Op( "c" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 5 );
	U_TEST_ASSERT( notation[0].operand );
	U_TEST_ASSERT( notation[1].operand );
	U_TEST_ASSERT( !notation[2].operand );
	U_TEST_ASSERT( notation[2].operator_ == chain.components[0].op );
	U_TEST_ASSERT( notation[3].operand );
	U_TEST_ASSERT( !notation[4].operand );
	U_TEST_ASSERT( notation[4].operator_ == chain.components[1].op );
}

U_TEST(MultiPriorityExpressionTest0)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(3);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Mul;
	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Add;
	chain.components[2].component= Op( "c" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 5 );
	U_TEST_ASSERT( notation[0].operand );
	U_TEST_ASSERT( notation[1].operand );
	U_TEST_ASSERT( !notation[2].operand );
	U_TEST_ASSERT( notation[2].operator_ == chain.components[0].op );
	U_TEST_ASSERT( notation[3].operand );
	U_TEST_ASSERT( !notation[4].operand );
	U_TEST_ASSERT( notation[4].operator_ == chain.components[1].op );
}

U_TEST(MultiPriorityExpressionTest1)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(3);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Add;
	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Mul;
	chain.components[2].component= Op( "c" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 5 );

	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b
	U_TEST_ASSERT( notation[2].operand ); // c

	U_TEST_ASSERT( !notation[3].operand ); // *
	U_TEST_ASSERT( notation[3].operator_ == BinaryOperator::Mul );

	U_TEST_ASSERT( !notation[4].operand ); // +
	U_TEST_ASSERT( notation[4].operator_ == BinaryOperator::Add );
}

U_TEST(MultiPriorityExpressionTest2)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(4);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Add;

	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Mul;

	chain.components[2].component= Op( "c" );
	chain.components[2].op= BinaryOperator::Mul;

	chain.components[3].component= Op( "d" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 7 );

	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b
	U_TEST_ASSERT( notation[2].operand ); // c

	U_TEST_ASSERT( !notation[3].operand ); // *
	U_TEST_ASSERT( notation[3].operator_ == BinaryOperator::Mul );

	U_TEST_ASSERT( notation[4].operand ); // d

	U_TEST_ASSERT( !notation[5].operand ); // *
	U_TEST_ASSERT( notation[5].operator_ == BinaryOperator::Mul );

	U_TEST_ASSERT( !notation[6].operand ); // +
	U_TEST_ASSERT( notation[6].operator_ == BinaryOperator::Add );
}

U_TEST(MultiPriorityExpressionTest3)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(4);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Mul;

	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Add;

	chain.components[2].component= Op( "c" );
	chain.components[2].op= BinaryOperator::Sub;

	chain.components[3].component= Op( "d" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 7 );

	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b

	U_TEST_ASSERT( !notation[2].operand ); // *
	U_TEST_ASSERT( notation[2].operator_ == BinaryOperator::Mul );

	U_TEST_ASSERT( notation[3].operand ); // c

	U_TEST_ASSERT( !notation[4].operand ); // +
	U_TEST_ASSERT( notation[4].operator_ == BinaryOperator::Add );

	U_TEST_ASSERT( notation[5].operand ); // d

	U_TEST_ASSERT( !notation[6].operand ); // -
	U_TEST_ASSERT( notation[6].operator_ == BinaryOperator::Sub );
}

U_TEST(MultiPriorityExpressionTest4)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(4);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Mul;

	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Add;

	chain.components[2].component= Op( "c" );
	chain.components[2].op= BinaryOperator::Mul;

	chain.components[3].component= Op( "d" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 7 );

	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b

	U_TEST_ASSERT( !notation[2].operand ); // *
	U_TEST_ASSERT( notation[2].operator_ == BinaryOperator::Mul );

	U_TEST_ASSERT( notation[3].operand ); // c

	U_TEST_ASSERT( notation[4].operand ); // d

	U_TEST_ASSERT( !notation[5].operand ); // *
	U_TEST_ASSERT( notation[5].operator_ == BinaryOperator::Mul );

	U_TEST_ASSERT( !notation[6].operand ); // +
	U_TEST_ASSERT( notation[6].operator_ == BinaryOperator::Add );
}

U_TEST(MultiPriorityExpressionTest5)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(5);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Add;

	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Mul;

	chain.components[2].component= Op( "c" );
	chain.components[2].op= BinaryOperator::Add;

	chain.components[3].component= Op( "d" );
	chain.components[3].op= BinaryOperator::Sub;

	chain.components[4].component= Op( "e" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 9 );

	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b
	U_TEST_ASSERT( notation[2].operand ); // c

	U_TEST_ASSERT( !notation[3].operand ); // *
	U_TEST_ASSERT( notation[3].operator_ == BinaryOperator::Mul );

	U_TEST_ASSERT( !notation[4].operand ); // +
	U_TEST_ASSERT( notation[4].operator_ == BinaryOperator::Add );

	U_TEST_ASSERT( notation[5].operand ); // d

	U_TEST_ASSERT( !notation[6].operand ); // +
	U_TEST_ASSERT( notation[6].operator_ == BinaryOperator::Add );

	U_TEST_ASSERT( notation[7].operand ); // e

	U_TEST_ASSERT( !notation[8].operand ); // -
	U_TEST_ASSERT( notation[8].operator_ == BinaryOperator::Sub );
}

U_TEST(ExpressionWithComparisionOperatorsTest0)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(3);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Equal;

	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Add;

	chain.components[2].component= Op( "c" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 5 );

	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b
	U_TEST_ASSERT( notation[2].operand ); // c

	U_TEST_ASSERT( !notation[3].operand ); // +
	U_TEST_ASSERT( notation[3].operator_ == BinaryOperator::Add );

	U_TEST_ASSERT( !notation[4].operand ); // ==
	U_TEST_ASSERT( notation[4].operator_ == BinaryOperator::Equal );
}

U_TEST(ExpressionWithComparisionOperatorsTest1)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(3);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Mul;

	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Less;

	chain.components[2].component= Op( "c" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 5 );

	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b

	U_TEST_ASSERT( !notation[2].operand ); // *
	U_TEST_ASSERT( notation[2].operator_ == BinaryOperator::Mul );

	U_TEST_ASSERT( notation[3].operand ); // c

	U_TEST_ASSERT( !notation[4].operand ); // <
	U_TEST_ASSERT( notation[4].operator_ == BinaryOperator::Less );
}

U_TEST(ExpressionWithComparisionOperatorsTest2)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(4);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::NotEqual;

	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Add;

	chain.components[2].component= Op( "c" );
	chain.components[2].op= BinaryOperator::Mul;

	chain.components[3].component= Op( "d" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 7 );

	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b
	U_TEST_ASSERT( notation[2].operand ); // c
	U_TEST_ASSERT( notation[3].operand ); // d

	U_TEST_ASSERT( !notation[4].operand ); // *
	U_TEST_ASSERT( notation[4].operator_ == BinaryOperator::Mul );

	U_TEST_ASSERT( !notation[5].operand ); // +
	U_TEST_ASSERT( notation[5].operator_ == BinaryOperator::Add );

	U_TEST_ASSERT( !notation[6].operand ); // !=
	U_TEST_ASSERT( notation[6].operator_ == BinaryOperator::NotEqual );
}

U_TEST(LogicalExpressionTest0)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(5);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::Add;

	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Equal;

	chain.components[2].component= Op( "c" );
	chain.components[2].op= BinaryOperator::Mul;

	chain.components[3].component= Op( "d" );
	chain.components[3].op= BinaryOperator::LazyLogicalOr;

	chain.components[4].component.reset( new BooleanConstant( FilePos{ 0, 0 }, false ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 9 );

	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b

	U_TEST_ASSERT( !notation[2].operand ); // +
	U_TEST_ASSERT( notation[2].operator_ == BinaryOperator::Add );

	U_TEST_ASSERT( notation[3].operand ); // c
	U_TEST_ASSERT( notation[4].operand ); // d

	U_TEST_ASSERT( !notation[5].operand ); // *
	U_TEST_ASSERT( notation[5].operator_ == BinaryOperator::Mul );

	U_TEST_ASSERT( !notation[6].operand ); // ==
	U_TEST_ASSERT( notation[6].operator_ == BinaryOperator::Equal );

	U_TEST_ASSERT( notation[7].operand ); // false

	U_TEST_ASSERT( !notation[8].operand ); // ||
	U_TEST_ASSERT( notation[8].operator_ == BinaryOperator::LazyLogicalOr );
}

U_TEST(LogicalExpressionTest1)
{
	BinaryOperatorsChain chain( FilePos{ 0, 0 } );
	chain.components.resize(4);

	chain.components[0].component= Op( "a" );
	chain.components[0].op= BinaryOperator::And;

	chain.components[1].component= Op( "b" );
	chain.components[1].op= BinaryOperator::Or;

	chain.components[2].component= Op( "c" );
	chain.components[2].op= BinaryOperator::LazyLogicalAnd;

	chain.components[3].component= Op( "d" );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_TEST_ASSERT( notation.size() == 7 );

	U_TEST_ASSERT( notation[0].operand ); // a
	U_TEST_ASSERT( notation[1].operand ); // b

	U_TEST_ASSERT( !notation[2].operand ); // +
	U_TEST_ASSERT( notation[2].operator_ == BinaryOperator::And );

	U_TEST_ASSERT( notation[3].operand ); // c

	U_TEST_ASSERT( !notation[4].operand ); // |
	U_TEST_ASSERT( notation[4].operator_ == BinaryOperator::Or );

	U_TEST_ASSERT( notation[5].operand ); // d

	U_TEST_ASSERT( !notation[6].operand ); // &&
	U_TEST_ASSERT( notation[6].operator_ == BinaryOperator::LazyLogicalAnd );
}

} // namespace U
