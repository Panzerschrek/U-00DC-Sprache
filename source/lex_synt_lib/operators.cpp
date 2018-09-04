#include "assert.hpp"

#include "operators.hpp"

namespace U
{

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

	return ToProgramString( op_str );
}

ProgramString OverloadedOperatorToString( const OverloadedOperator op )
{
	switch( op )
	{
	case OverloadedOperator::None: return ""_SpC;

	case OverloadedOperator::Add: return "+"_SpC;
	case OverloadedOperator::Sub: return "-"_SpC;
	case OverloadedOperator::Mul: return "*"_SpC;
	case OverloadedOperator::Div: return "/"_SpC;
	case OverloadedOperator::Rem: return "%"_SpC;

	case OverloadedOperator::Equal: return "=="_SpC;
	case OverloadedOperator::NotEqual: return "!="_SpC;
	case OverloadedOperator::Less: return "<"_SpC;
	case OverloadedOperator::LessEqual: return "<="_SpC;
	case OverloadedOperator::Greater: return ">"_SpC;
	case OverloadedOperator::GreaterEqual: return ">="_SpC;

	case OverloadedOperator::And: return "&"_SpC;
	case OverloadedOperator::Or : return "|"_SpC;
	case OverloadedOperator::Xor: return "^"_SpC;

	case OverloadedOperator::ShiftLeft : return "<<"_SpC;
	case OverloadedOperator::ShiftRight: return ">>"_SpC;

	case OverloadedOperator::AssignAdd: return "+="_SpC;
	case OverloadedOperator::AssignSub: return "-="_SpC;
	case OverloadedOperator::AssignMul: return "*="_SpC;
	case OverloadedOperator::AssignDiv: return "/="_SpC;
	case OverloadedOperator::AssignRem: return "%="_SpC;

	case OverloadedOperator::AssignAnd: return "&="_SpC;
	case OverloadedOperator::AssignOr : return "|="_SpC;
	case OverloadedOperator::AssignXor: return "^="_SpC;

	case OverloadedOperator::AssignShiftLeft : return "<<="_SpC;
	case OverloadedOperator::AssignShiftRight: return ">>="_SpC;

	case OverloadedOperator::LogicalNot: return "!"_SpC;
	case OverloadedOperator::BitwiseNot: return "~"_SpC;

	case OverloadedOperator::Assign: return "="_SpC;
	case OverloadedOperator::Increment: return "++"_SpC;
	case OverloadedOperator::Decrement: return "--"_SpC;

	case OverloadedOperator::Indexing: return "[]"_SpC;
	case OverloadedOperator::Call: return "()"_SpC;
	};

	U_ASSERT(false);
	return ""_SpC;
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
