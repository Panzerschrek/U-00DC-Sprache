#pragma once
#include <string_view>

namespace U
{

enum class BinaryOperatorType : uint8_t
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
	CompareOrder,

	And,
	Or,
	Xor,

	ShiftLeft ,
	ShiftRight,

	LazyLogicalAnd,
	LazyLogicalOr,
};

enum class OverloadedOperator : uint8_t
{
	None,

	Add, // for unary and binary +
	Sub, // for unary and binary -
	Mul,
	Div,
	Rem,

	CompareEqual,
	CompareOrder,

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
	Call,
};

std::string_view BinaryOperatorToString( BinaryOperatorType op );
std::string_view OverloadedOperatorToString( OverloadedOperator op );

OverloadedOperator GetOverloadedOperatorForBinaryOperator( const BinaryOperatorType binary_operator_type );
OverloadedOperator GetOverloadedOperatorForAdditiveAssignmentOperator( const BinaryOperatorType operator_type );

} // namespace U
