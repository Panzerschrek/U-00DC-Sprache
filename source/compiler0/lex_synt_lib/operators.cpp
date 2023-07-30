#include "../../lex_synt_lib_common/assert.hpp"

#include "operators.hpp"

namespace U
{

std::string_view BinaryOperatorToString( const BinaryOperatorType op )
{
	switch( op )
	{
	case BinaryOperatorType::Add: return "+";
	case BinaryOperatorType::Sub: return "-";
	case BinaryOperatorType::Mul: return "*";
	case BinaryOperatorType::Div: return "/";
	case BinaryOperatorType::Rem: return "%";

	case BinaryOperatorType::Equal: return "==";
	case BinaryOperatorType::NotEqual: return "!=";
	case BinaryOperatorType::Less: return "<";
	case BinaryOperatorType::LessEqual: return "<=";
	case BinaryOperatorType::Greater: return ">";
	case BinaryOperatorType::GreaterEqual: return ">=";
	case BinaryOperatorType::CompareOrder: return "<=>";

	case BinaryOperatorType::And: return "&";
	case BinaryOperatorType::Or: return "|";
	case BinaryOperatorType::Xor: return "^";

	case BinaryOperatorType::ShiftLeft : return "<<";
	case BinaryOperatorType::ShiftRight: return ">>";

	case BinaryOperatorType::LazyLogicalAnd: return "&&";
	case BinaryOperatorType::LazyLogicalOr: return "||";
	};

	U_ASSERT(false);
	return "";
}

std::string_view OverloadedOperatorToString( const OverloadedOperator op )
{
	switch( op )
	{
	case OverloadedOperator::None: return "";

	case OverloadedOperator::Add: return "+";
	case OverloadedOperator::Sub: return "-";
	case OverloadedOperator::Mul: return "*";
	case OverloadedOperator::Div: return "/";
	case OverloadedOperator::Rem: return "%";

	case OverloadedOperator::CompareEqual: return "==";
	case OverloadedOperator::CompareOrder: return "<=>";

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
	case BinaryOperatorType::Equal:
	case BinaryOperatorType::NotEqual:
		return OverloadedOperator::CompareEqual;
	case BinaryOperatorType::Less:
	case BinaryOperatorType::LessEqual:
	case BinaryOperatorType::Greater:
	case BinaryOperatorType::GreaterEqual:
	case BinaryOperatorType::CompareOrder:
		return OverloadedOperator::CompareOrder;
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
