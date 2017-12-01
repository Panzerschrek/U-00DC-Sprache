#pragma once
#include "program_string.hpp"

namespace U
{

enum class BinaryOperatorType
{
	Add,
	Sub,
	Mul,
	Div,
	Rem,

	Equal,
	NotEqual,
	Less,
	LessEqual,
	Greater,
	GreaterEqual,

	And,
	Or,
	Xor,

	ShiftLeft ,
	ShiftRight,

	LazyLogicalAnd,
	LazyLogicalOr,

	Last,
};

enum class OverloadedOperator
{
	None,

	Add, // for unary and binary +
	Sub, // for unary and binary -
	Mul,
	Div,
	Rem,

	Equal,
	NotEqual,
	Less,
	LessEqual,
	Greater,
	GreaterEqual,

	And,
	Or,
	Xor,

	ShiftLeft ,
	ShiftRight,

	AssignAdd,
	AssignSub,
	AssignMul,
	AssignDiv,
	AssignRem,

	AssignAnd,
	AssignOr ,
	AssignXor,

	AssignShiftLeft ,
	AssignShiftRight,

	LogicalNot,
	BitwiseNot,

	Assign,
	Increment,
	Decrement,

	Indexing,
};

ProgramString BinaryOperatorToString( BinaryOperatorType op );
ProgramString OverloadedOperatorToString( OverloadedOperator op );

OverloadedOperator GetOverloadedOperatorForBinaryOperator( const BinaryOperatorType binary_operator_type );
OverloadedOperator GetOverloadedOperatorForAdditiveAssignmentOperator( const BinaryOperatorType operator_type );

} // namespace U
