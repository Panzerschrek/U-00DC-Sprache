#include "assert.hpp"

#include "operators.hpp"

namespace U
{

// TODO - return const char*
ProgramString BinaryOperatorToString( const BinaryOperatorType op )
{
	const char* op_str= "";
	switch( op )
	{
	case BinaryOperatorType::Add: op_str= "+"; break;
	case BinaryOperatorType::Sub: op_str= "-"; break;
	case BinaryOperatorType::Mul: op_str= "*"; break;
	case BinaryOperatorType::Div: op_str= "/"; break;
	case BinaryOperatorType::Rem: op_str= "%"; break;

	case BinaryOperatorType::Equal: op_str= "=="; break;
	case BinaryOperatorType::NotEqual: op_str= "!="; break;
	case BinaryOperatorType::Less: op_str= "<"; break;
	case BinaryOperatorType::LessEqual: op_str= "<="; break;
	case BinaryOperatorType::Greater: op_str= ">"; break;
	case BinaryOperatorType::GreaterEqual: op_str= ">="; break;

	case BinaryOperatorType::And: op_str= "&"; break;
	case BinaryOperatorType::Or: op_str= "|"; break;
	case BinaryOperatorType::Xor: op_str= "^"; break;

	case BinaryOperatorType::ShiftLeft : op_str= "<<"; break;
	case BinaryOperatorType::ShiftRight: op_str= ">>"; break;

	case BinaryOperatorType::LazyLogicalAnd: op_str= "&&"; break;
	case BinaryOperatorType::LazyLogicalOr: op_str= "||"; break;

	case BinaryOperatorType::Last: U_ASSERT(false); break;
	};

	return op_str;
}

ProgramString OverloadedOperatorToString( const OverloadedOperator op )
{
	switch( op )
	{
	case OverloadedOperator::None: return "";

	case OverloadedOperator::Add: return "+";
	case OverloadedOperator::Sub: return "-";
	case OverloadedOperator::Mul: return "*";
	case OverloadedOperator::Div: return "/";
	case OverloadedOperator::Rem: return "%";

	case OverloadedOperator::Equal: return "==";
	case OverloadedOperator::NotEqual: return "!=";
	case OverloadedOperator::Less: return "<";
	case OverloadedOperator::LessEqual: return "<=";
	case OverloadedOperator::Greater: return ">";
	case OverloadedOperator::GreaterEqual: return ">=";

	case OverloadedOperator::And: return "&";
	case OverloadedOperator::Or : return "|";
	case OverloadedOperator::Xor: return "^";

	case OverloadedOperator::ShiftLeft : return "<<";
	case OverloadedOperator::ShiftRight: return ">>";

	case OverloadedOperator::AssignAdd: return "+=";
	case OverloadedOperator::AssignSub: return "-=";
	case OverloadedOperator::AssignMul: return "*=";
	case OverloadedOperator::AssignDiv: return "/=";
	case OverloadedOperator::AssignRem: return "%=";

	case OverloadedOperator::AssignAnd: return "&=";
	case OverloadedOperator::AssignOr : return "|=";
	case OverloadedOperator::AssignXor: return "^=";

	case OverloadedOperator::AssignShiftLeft : return "<<=";
	case OverloadedOperator::AssignShiftRight: return ">>=";

	case OverloadedOperator::LogicalNot: return "!";
	case OverloadedOperator::BitwiseNot: return "~";

	case OverloadedOperator::Assign: return "=";
	case OverloadedOperator::Increment: return "++";
	case OverloadedOperator::Decrement: return "--";

	case OverloadedOperator::Indexing: return "[]";
	case OverloadedOperator::Call: return "()";
	};

	U_ASSERT(false);
	return "";
}

OverloadedOperator GetOverloadedOperatorForBinaryOperator( const BinaryOperatorType binary_operator_type )
{
	switch( binary_operator_type )
	{
	case BinaryOperatorType::Add: return OverloadedOperator::Add;
	case BinaryOperatorType::Sub: return OverloadedOperator::Sub;
	case BinaryOperatorType::Mul: return OverloadedOperator::Mul;
	case BinaryOperatorType::Div: return OverloadedOperator::Div;
	case BinaryOperatorType::Rem: return OverloadedOperator::Rem;
	case BinaryOperatorType::And: return OverloadedOperator::And;
	case BinaryOperatorType::Or : return OverloadedOperator::Or ;
	case BinaryOperatorType::Xor: return OverloadedOperator::Xor;
	case BinaryOperatorType::Equal: return OverloadedOperator::Equal;
	case BinaryOperatorType::NotEqual: return OverloadedOperator::NotEqual;
	case BinaryOperatorType::Less: return OverloadedOperator::Less;
	case BinaryOperatorType::LessEqual: return OverloadedOperator::LessEqual;
	case BinaryOperatorType::Greater: return OverloadedOperator::Greater;
	case BinaryOperatorType::GreaterEqual: return OverloadedOperator::GreaterEqual;
	case BinaryOperatorType::ShiftLeft : return OverloadedOperator::ShiftLeft ;
	case BinaryOperatorType::ShiftRight: return OverloadedOperator::ShiftRight;
	default: U_ASSERT(false); return OverloadedOperator::None;
	};
}

OverloadedOperator GetOverloadedOperatorForAdditiveAssignmentOperator( const BinaryOperatorType operator_type )
{
	switch( operator_type )
	{
	case BinaryOperatorType::Add: return OverloadedOperator::AssignAdd;
	case BinaryOperatorType::Sub: return OverloadedOperator::AssignSub;
	case BinaryOperatorType::Mul: return OverloadedOperator::AssignMul;
	case BinaryOperatorType::Div: return OverloadedOperator::AssignDiv;
	case BinaryOperatorType::Rem: return OverloadedOperator::AssignRem;
	case BinaryOperatorType::And: return OverloadedOperator::AssignAnd;
	case BinaryOperatorType::Or : return OverloadedOperator::AssignOr ;
	case BinaryOperatorType::Xor: return OverloadedOperator::AssignXor;
	case BinaryOperatorType::ShiftLeft : return OverloadedOperator::AssignShiftLeft ;
	case BinaryOperatorType::ShiftRight: return OverloadedOperator::AssignShiftRight;
	default: U_ASSERT(false); return OverloadedOperator::None;
	};
}

} // namespace U
