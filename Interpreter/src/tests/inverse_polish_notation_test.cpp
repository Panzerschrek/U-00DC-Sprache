#include <iostream>

#include "../assert.hpp"
#include "../inverse_polish_notation.hpp"

#include "inverse_polish_notation_test.hpp"

namespace Interpreter
{

static void Print( const BinaryOperatorsChain& chain, const InversePolishNotation& ipl )
{
	std::cout << "\n";
	chain.Print( std::cout, 0 );
	std::cout << "  =>  ";
	PrintInversePolishNotation( std::cout, ipl );
	std::cout << "\n";
}

static void OneOperandExpressionTest()
{
	BinaryOperatorsChain chain;
	chain.components.resize(1);

	chain.components[0].component.reset( new NamedOperand( ToProgramString( "a" ) ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );

	U_ASSERT( notation.size() == 1 );
	U_ASSERT( notation[0].operand );

	Print( chain, notation );
}

static void TwoOperandExpressionTest()
{
	BinaryOperatorsChain chain;
	chain.components.resize(2);

	chain.components[0].component.reset( new NamedOperand( ToProgramString( "a" ) ) );
	chain.components[0].op= BinaryOperator::Add;
	chain.components[1].component.reset( new NamedOperand( ToProgramString( "b" ) ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_ASSERT( notation.size() == 3 );
	U_ASSERT( notation[0].operand );
	U_ASSERT( notation[1].operand );
	U_ASSERT( !notation[2].operand );
	U_ASSERT( notation[2].operator_ == chain.components[0].op );
}

static void LongFlatExpressionTest()
{
	BinaryOperatorsChain chain;
	chain.components.resize(4);

	chain.components[0].component.reset( new NamedOperand( ToProgramString( "a" ) ) );
	chain.components[0].op= BinaryOperator::Add;
	chain.components[1].component.reset( new NamedOperand( ToProgramString( "b" ) ) );
	chain.components[1].op= BinaryOperator::Add;
	chain.components[2].component.reset( new NamedOperand( ToProgramString( "c" ) ) );
	chain.components[2].op= BinaryOperator::Add;
	chain.components[3].component.reset( new NamedOperand( ToProgramString( "d" ) ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_ASSERT( notation.size() == 7 );
	U_ASSERT( notation[0].operand ); // a
	U_ASSERT( notation[1].operand ); // b

	U_ASSERT( !notation[2].operand ); // +
	U_ASSERT( notation[2].operator_ == BinaryOperator::Add );

	U_ASSERT( notation[3].operand ); // d

	U_ASSERT( !notation[4].operand ); // +
	U_ASSERT( notation[4].operator_ == BinaryOperator::Add );
}

static void TriOperandEqualExpressionTest()
{
	BinaryOperatorsChain chain;
	chain.components.resize(3);

	chain.components[0].component.reset( new NamedOperand( ToProgramString( "a" ) ) );
	chain.components[0].op= BinaryOperator::Add;
	chain.components[1].component.reset( new NamedOperand( ToProgramString( "b" ) ) );
	chain.components[1].op= BinaryOperator::Add;
	chain.components[2].component.reset( new NamedOperand( ToProgramString( "c" ) ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_ASSERT( notation.size() == 5 );
	U_ASSERT( notation[0].operand );
	U_ASSERT( notation[1].operand );
	U_ASSERT( !notation[2].operand );
	U_ASSERT( notation[2].operator_ == chain.components[0].op );
	U_ASSERT( notation[3].operand );
	U_ASSERT( !notation[4].operand );
	U_ASSERT( notation[4].operator_ == chain.components[1].op );
}

static void MultiPriorityExpressionTest0()
{
	BinaryOperatorsChain chain;
	chain.components.resize(3);

	chain.components[0].component.reset( new NamedOperand( ToProgramString( "a" ) ) );
	chain.components[0].op= BinaryOperator::Mul;
	chain.components[1].component.reset( new NamedOperand( ToProgramString( "b" ) ) );
	chain.components[1].op= BinaryOperator::Add;
	chain.components[2].component.reset( new NamedOperand( ToProgramString( "c" ) ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_ASSERT( notation.size() == 5 );
	U_ASSERT( notation[0].operand );
	U_ASSERT( notation[1].operand );
	U_ASSERT( !notation[2].operand );
	U_ASSERT( notation[2].operator_ == chain.components[0].op );
	U_ASSERT( notation[3].operand );
	U_ASSERT( !notation[4].operand );
	U_ASSERT( notation[4].operator_ == chain.components[1].op );
}

static void MultiPriorityExpressionTest1()
{
	BinaryOperatorsChain chain;
	chain.components.resize(3);

	chain.components[0].component.reset( new NamedOperand( ToProgramString( "a" ) ) );
	chain.components[0].op= BinaryOperator::Add;
	chain.components[1].component.reset( new NamedOperand( ToProgramString( "b" ) ) );
	chain.components[1].op= BinaryOperator::Mul;
	chain.components[2].component.reset( new NamedOperand( ToProgramString( "c" ) ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_ASSERT( notation.size() == 5 );

	U_ASSERT( notation[0].operand ); // a
	U_ASSERT( notation[1].operand ); // b
	U_ASSERT( notation[2].operand ); // c

	U_ASSERT( !notation[3].operand ); // *
	U_ASSERT( notation[3].operator_ == BinaryOperator::Mul );

	U_ASSERT( !notation[4].operand ); // +
	U_ASSERT( notation[4].operator_ == BinaryOperator::Add );
}

static void MultiPriorityExpressionTest2()
{
	BinaryOperatorsChain chain;
	chain.components.resize(4);

	chain.components[0].component.reset( new NamedOperand( ToProgramString( "a" ) ) );
	chain.components[0].op= BinaryOperator::Add;

	chain.components[1].component.reset( new NamedOperand( ToProgramString( "b" ) ) );
	chain.components[1].op= BinaryOperator::Mul;

	chain.components[2].component.reset( new NamedOperand( ToProgramString( "c" ) ) );
	chain.components[2].op= BinaryOperator::Mul;

	chain.components[3].component.reset( new NamedOperand( ToProgramString( "d" ) ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_ASSERT( notation.size() == 7 );

	U_ASSERT( notation[0].operand ); // a
	U_ASSERT( notation[1].operand ); // b
	U_ASSERT( notation[2].operand ); // c

	U_ASSERT( !notation[3].operand ); // *
	U_ASSERT( notation[3].operator_ == BinaryOperator::Mul );

	U_ASSERT( notation[4].operand ); // d

	U_ASSERT( !notation[5].operand ); // *
	U_ASSERT( notation[5].operator_ == BinaryOperator::Mul );

	U_ASSERT( !notation[6].operand ); // +
	U_ASSERT( notation[6].operator_ == BinaryOperator::Add );
}

static void MultiPriorityExpressionTest3()
{
	BinaryOperatorsChain chain;
	chain.components.resize(4);

	chain.components[0].component.reset( new NamedOperand( ToProgramString( "a" ) ) );
	chain.components[0].op= BinaryOperator::Mul;

	chain.components[1].component.reset( new NamedOperand( ToProgramString( "b" ) ) );
	chain.components[1].op= BinaryOperator::Add;

	chain.components[2].component.reset( new NamedOperand( ToProgramString( "c" ) ) );
	chain.components[2].op= BinaryOperator::Sub;

	chain.components[3].component.reset( new NamedOperand( ToProgramString( "d" ) ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_ASSERT( notation.size() == 7 );

	U_ASSERT( notation[0].operand ); // a
	U_ASSERT( notation[1].operand ); // b

	U_ASSERT( !notation[2].operand ); // *
	U_ASSERT( notation[2].operator_ == BinaryOperator::Mul );

	U_ASSERT( notation[3].operand ); // c

	U_ASSERT( !notation[4].operand ); // +
	U_ASSERT( notation[4].operator_ == BinaryOperator::Add );

	U_ASSERT( notation[5].operand ); // d

	U_ASSERT( !notation[6].operand ); // -
	U_ASSERT( notation[6].operator_ == BinaryOperator::Sub );
}

static void MultiPriorityExpressionTest4()
{
	BinaryOperatorsChain chain;
	chain.components.resize(4);

	chain.components[0].component.reset( new NamedOperand( ToProgramString( "a" ) ) );
	chain.components[0].op= BinaryOperator::Mul;

	chain.components[1].component.reset( new NamedOperand( ToProgramString( "b" ) ) );
	chain.components[1].op= BinaryOperator::Add;

	chain.components[2].component.reset( new NamedOperand( ToProgramString( "c" ) ) );
	chain.components[2].op= BinaryOperator::Mul;

	chain.components[3].component.reset( new NamedOperand( ToProgramString( "d" ) ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_ASSERT( notation.size() == 7 );

	U_ASSERT( notation[0].operand ); // a
	U_ASSERT( notation[1].operand ); // b

	U_ASSERT( !notation[2].operand ); // *
	U_ASSERT( notation[2].operator_ == BinaryOperator::Mul );

	U_ASSERT( notation[3].operand ); // c

	U_ASSERT( notation[4].operand ); // d

	U_ASSERT( !notation[5].operand ); // *
	U_ASSERT( notation[5].operator_ == BinaryOperator::Mul );

	U_ASSERT( !notation[6].operand ); // +
	U_ASSERT( notation[6].operator_ == BinaryOperator::Add );
}

static void MultiPriorityExpressionTest5()
{
	BinaryOperatorsChain chain;
	chain.components.resize(5);

	chain.components[0].component.reset( new NamedOperand( ToProgramString( "a" ) ) );
	chain.components[0].op= BinaryOperator::Add;

	chain.components[1].component.reset( new NamedOperand( ToProgramString( "b" ) ) );
	chain.components[1].op= BinaryOperator::Mul;

	chain.components[2].component.reset( new NamedOperand( ToProgramString( "c" ) ) );
	chain.components[2].op= BinaryOperator::Add;

	chain.components[3].component.reset( new NamedOperand( ToProgramString( "d" ) ) );
	chain.components[3].op= BinaryOperator::Sub;

	chain.components[4].component.reset( new NamedOperand( ToProgramString( "e" ) ) );

	InversePolishNotation notation= ConvertToInversePolishNotation( chain );
	Print( chain, notation );

	U_ASSERT( notation.size() == 9 );

	U_ASSERT( notation[0].operand ); // a
	U_ASSERT( notation[1].operand ); // b
	U_ASSERT( notation[2].operand ); // c

	U_ASSERT( !notation[3].operand ); // *
	U_ASSERT( notation[3].operator_ == BinaryOperator::Mul );

	U_ASSERT( !notation[4].operand ); // +
	U_ASSERT( notation[4].operator_ == BinaryOperator::Add );

	U_ASSERT( notation[5].operand ); // d

	U_ASSERT( !notation[6].operand ); // +
	U_ASSERT( notation[6].operator_ == BinaryOperator::Add );

	U_ASSERT( notation[7].operand ); // e

	U_ASSERT( !notation[8].operand ); // -
	U_ASSERT( notation[8].operator_ == BinaryOperator::Sub );

}

void RunIPNTests()
{
	OneOperandExpressionTest();
	TwoOperandExpressionTest();
	LongFlatExpressionTest();
	TriOperandEqualExpressionTest();
	MultiPriorityExpressionTest0();
	MultiPriorityExpressionTest1();
	MultiPriorityExpressionTest2();
	MultiPriorityExpressionTest3();
	MultiPriorityExpressionTest4();
	MultiPriorityExpressionTest5();
}

} // namespace Interpreter
