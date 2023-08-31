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

std::optional<BinaryOperatorType> StringToBinaryOperator( const std::string_view s )
{
	if( s == "+" ) return BinaryOperatorType::Add;
	if( s == "-" ) return BinaryOperatorType::Sub;
	if( s == "*" ) return BinaryOperatorType::Mul;
	if( s == "/" ) return BinaryOperatorType::Div;
	if( s == "%" ) return BinaryOperatorType::Rem;
	if( s == "=="  ) return BinaryOperatorType::Equal;
	if( s == "!="  ) return BinaryOperatorType::NotEqual;
	if( s == "<"   ) return BinaryOperatorType::Less;
	if( s == "<="  ) return BinaryOperatorType::LessEqual;
	if( s == ">"   ) return BinaryOperatorType::Greater;
	if( s == ">="  ) return BinaryOperatorType::GreaterEqual;
	if( s == "<=>" ) return BinaryOperatorType::CompareOrder;
	if( s == "&" ) return BinaryOperatorType::And;
	if( s == "|" ) return BinaryOperatorType::Or;
	if( s == "^" ) return BinaryOperatorType::Xor;
	if( s == "<<" ) return BinaryOperatorType::ShiftLeft;
	if( s == ">>" ) return BinaryOperatorType::ShiftRight;
	if( s == "&&" ) return BinaryOperatorType::LazyLogicalAnd;
	if( s == "||" ) return BinaryOperatorType::LazyLogicalOr;

	return std::nullopt;
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

std::optional<OverloadedOperator> StringToOverloadedOperator( const std::string_view s )
{
	if( s == "+" ) return OverloadedOperator::Add;
	if( s == "-" ) return OverloadedOperator::Sub;
	if( s == "*" ) return OverloadedOperator::Mul;
	if( s == "/" ) return OverloadedOperator::Div;
	if( s == "%" ) return OverloadedOperator::Rem;
	if( s == "==" ) return OverloadedOperator::CompareEqual;
	if( s == "<=>" ) return OverloadedOperator::CompareOrder;
	if( s == "&" ) return OverloadedOperator::And;
	if( s == "|" ) return OverloadedOperator::Or;
	if( s == "^" ) return OverloadedOperator::Xor;
	if( s == "<<" ) return OverloadedOperator::ShiftLeft;
	if( s == ">>" ) return OverloadedOperator::ShiftRight;
	if( s == "+=" ) return OverloadedOperator::AssignAdd;
	if( s == "-=" ) return OverloadedOperator::AssignSub;
	if( s == "*=" ) return OverloadedOperator::AssignMul;
	if( s == "/=" ) return OverloadedOperator::AssignDiv;
	if( s == "%=" ) return OverloadedOperator::AssignRem;
	if( s == "&=" ) return OverloadedOperator::AssignAnd;
	if( s == "|=" ) return OverloadedOperator::AssignOr;
	if( s == "^=" ) return OverloadedOperator::AssignXor;
	if( s == "<<=" ) return OverloadedOperator::AssignShiftLeft;
	if( s == ">>=" ) return OverloadedOperator::AssignShiftRight;
	if( s == "!" ) return OverloadedOperator::LogicalNot;
	if( s == "~" ) return OverloadedOperator::BitwiseNot;
	if( s == "=" ) return OverloadedOperator::Assign;
	if( s == "++" ) return OverloadedOperator::Increment;
	if( s == "--" ) return OverloadedOperator::Decrement;
	if( s == "[]" ) return OverloadedOperator::Indexing;
	if( s == "()" ) return OverloadedOperator::Call;

	return std::nullopt;
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
