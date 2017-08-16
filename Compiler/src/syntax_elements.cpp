#include "assert.hpp"
#include "syntax_analyzer.hpp"

namespace U
{

SyntaxElementBase::SyntaxElementBase( const FilePos& file_pos )
	: file_pos_(file_pos)
{}

IUnaryPrefixOperator::IUnaryPrefixOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

IUnaryPostfixOperator::IUnaryPostfixOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

UnaryPlus::UnaryPlus( const FilePos& file_pos )
	: IUnaryPrefixOperator(file_pos)
{}

UnaryPlus::~UnaryPlus()
{}

UnaryMinus::UnaryMinus( const FilePos& file_pos )
	: IUnaryPrefixOperator(file_pos)
{}

UnaryMinus::~UnaryMinus()
{}

LogicalNot::LogicalNot( const FilePos& file_pos )
	: IUnaryPrefixOperator(file_pos)
{}

LogicalNot::~LogicalNot()
{}

BitwiseNot::BitwiseNot( const FilePos& file_pos )
	: IUnaryPrefixOperator(file_pos)
{}

BitwiseNot::~BitwiseNot()
{}

CallOperator::CallOperator(
	const FilePos& file_pos,
	std::vector<IExpressionComponentPtr> arguments )
	: IUnaryPostfixOperator(file_pos)
	, arguments_( std::move( arguments ) )
{}

CallOperator::~CallOperator()
{}

IndexationOperator::IndexationOperator( const FilePos& file_pos, IExpressionComponentPtr index )
	: IUnaryPostfixOperator(file_pos)
	,index_( std::move( index ) )
{
	U_ASSERT( index_ );
}

IndexationOperator::~IndexationOperator()
{}

ProgramString BinaryOperatorToString( const BinaryOperatorType op )
{
	const char* op_str= "";
	switch( op )
	{
		case BinaryOperatorType::Add: op_str= "+"; break;
		case BinaryOperatorType::Sub: op_str= "-"; break;
		case BinaryOperatorType::Mul: op_str= "*"; break;
		case BinaryOperatorType::Div: op_str= "/"; break;

		case BinaryOperatorType::Equal: op_str= "=="; break;
		case BinaryOperatorType::NotEqual: op_str= "!="; break;
		case BinaryOperatorType::Less: op_str= "<"; break;
		case BinaryOperatorType::LessEqual: op_str= "<="; break;
		case BinaryOperatorType::Greater: op_str= ">"; break;
		case BinaryOperatorType::GreaterEqual: op_str= ">="; break;

		case BinaryOperatorType::And: op_str= "&"; break;
		case BinaryOperatorType::Or: op_str= "|"; break;
		case BinaryOperatorType::Xor: op_str= "^"; break;

		case BinaryOperatorType::LazyLogicalAnd: op_str= "&&"; break;
		case BinaryOperatorType::LazyLogicalOr: op_str= "||"; break;

		case BinaryOperatorType::Last: U_ASSERT(false); break;
	};

	return ToProgramString( op_str );
}

MemberAccessOperator::MemberAccessOperator(
	const FilePos& file_pos,
	ProgramString member_name )
	: IUnaryPostfixOperator( file_pos )
	, member_name_( std::move(member_name) )
{}

MemberAccessOperator::~MemberAccessOperator()
{}

IExpressionComponent::IExpressionComponent( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

IInitializer::IInitializer( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

ArrayInitializer::ArrayInitializer( const FilePos& file_pos )
	: IInitializer( file_pos )
{}

StructNamedInitializer::StructNamedInitializer( const FilePos& file_pos )
	: IInitializer( file_pos )
{}

ConstructorInitializer::ConstructorInitializer(
	const FilePos& file_pos,
	std::vector<IExpressionComponentPtr> arguments )
	: IInitializer( file_pos )
	, call_operator( file_pos, std::move(arguments) )
{}

ExpressionInitializer::ExpressionInitializer(
	const FilePos& file_pos , IExpressionComponentPtr in_expression )
	: IInitializer( file_pos )
	, expression(std::move(in_expression))
{}

ZeroInitializer::ZeroInitializer( const FilePos& file_pos )
	: IInitializer(file_pos)
{}

BinaryOperator::BinaryOperator( const FilePos& file_pos )
	: IExpressionComponent( file_pos )
{}

ExpressionComponentWithUnaryOperators::ExpressionComponentWithUnaryOperators( const FilePos& file_pos )
	: IExpressionComponent( file_pos )
{}

NamedOperand::NamedOperand( const FilePos& file_pos, ComplexName name )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, name_( std::move(name) )
{}

NamedOperand::~NamedOperand()
{}

BooleanConstant::BooleanConstant( const FilePos& file_pos, bool value )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, value_( value )
{}

BooleanConstant::~BooleanConstant()
{}

NumericConstant::NumericConstant(
	const FilePos& file_pos,
	LongFloat value,
	ProgramString type_suffix,
	bool has_fractional_point )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, value_( value )
	, type_suffix_( std::move(type_suffix) )
	, has_fractional_point_( has_fractional_point )
{}

NumericConstant::~NumericConstant()
{}

BracketExpression::BracketExpression( const FilePos& file_pos, IExpressionComponentPtr expression )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, expression_( std::move( expression ) )
{}

BracketExpression::~BracketExpression()
{}

IProgramElement::IProgramElement( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

IBlockElement::IBlockElement( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

Block::Block( const FilePos& file_pos, BlockElements elements )
	: IBlockElement(file_pos)
	, elements_( std::move( elements ) )
{}

Block::~Block()
{}

VariablesDeclaration::~VariablesDeclaration()
{}

VariablesDeclaration::VariablesDeclaration( const FilePos& file_pos )
	: IBlockElement(file_pos)
{}

VariablesDeclaration::VariablesDeclaration( VariablesDeclaration&& other )
	: IBlockElement(other.file_pos_)
{
	*this= std::move(other);
}

VariablesDeclaration& VariablesDeclaration::operator=( VariablesDeclaration&& other )
{
	file_pos_= other.file_pos_;

	variables= std::move( other.variables );
	type= std::move( other.type );

	return *this;
}

AutoVariableDeclaration::AutoVariableDeclaration( const FilePos& file_pos )
	: IBlockElement( file_pos )
{}

ReturnOperator::ReturnOperator( const FilePos& file_pos, IExpressionComponentPtr expression )
	: IBlockElement(file_pos)
	, expression_( std::move( expression ) )
{}

ReturnOperator::~ReturnOperator()
{}

WhileOperator::WhileOperator( const FilePos& file_pos, IExpressionComponentPtr condition, BlockPtr block )
	: IBlockElement(file_pos)
	, condition_( std::move( condition ) )
	, block_( std::move( block ) )
{}

WhileOperator::~WhileOperator()
{}

BreakOperator::BreakOperator( const FilePos& file_pos )
	: IBlockElement(file_pos)
{}

BreakOperator::~BreakOperator()
{}

ContinueOperator::ContinueOperator( const FilePos& file_pos )
	: IBlockElement(file_pos)
{}

ContinueOperator::~ContinueOperator()
{}

IfOperator::IfOperator( const FilePos& file_pos, std::vector<Branch> branches )
	: IBlockElement(file_pos)
	, branches_( std::move( branches ) )
{}

IfOperator::~IfOperator()
{}

SingleExpressionOperator::SingleExpressionOperator( const FilePos& file_pos, IExpressionComponentPtr expression )
	: IBlockElement(file_pos)
	, expression_( std::move( expression ) )
{}

SingleExpressionOperator::~SingleExpressionOperator()
{}

AssignmentOperator::AssignmentOperator(
	const FilePos& file_pos,
	IExpressionComponentPtr l_value,
	IExpressionComponentPtr r_value )
	: IBlockElement(file_pos)
	, l_value_( std::move( l_value ) )
	, r_value_( std::move( r_value ) )
{}

AssignmentOperator::~AssignmentOperator()
{}

AdditiveAssignmentOperator::AdditiveAssignmentOperator( const FilePos& file_pos )
	: IBlockElement(file_pos)
{}

IncrementOperator::IncrementOperator( const FilePos& file_pos )
	: IBlockElement(file_pos)
{}

DecrementOperator::DecrementOperator( const FilePos& file_pos )
	: IBlockElement(file_pos)
{}

FunctionArgumentDeclaration::FunctionArgumentDeclaration(
	const FilePos& file_pos,
	ProgramString name,
	TypeName type,
	MutabilityModifier mutability_modifier,
	ReferenceModifier reference_modifier )
	: SyntaxElementBase( file_pos )
	, name_(std::move(name))
	, type_(std::move(type))
	, mutability_modifier_(mutability_modifier)
	, reference_modifier_(reference_modifier)
{}

FunctionArgumentDeclaration::~FunctionArgumentDeclaration()
{}

FunctionDeclaration::FunctionDeclaration(
	const FilePos& file_pos,
	ComplexName name,
	TypeName return_type,
	MutabilityModifier return_value_mutability_modifier,
	ReferenceModifier return_value_reference_modifier,
	FunctionArgumentsDeclaration arguments,
	std::unique_ptr<StructNamedInitializer> constructor_initialization_list,
	BlockPtr block )
	: IProgramElement(file_pos)
	, name_( std::move(name) )
	, return_type_( std::move(return_type) )
	, return_value_mutability_modifier_(return_value_mutability_modifier)
	, return_value_reference_modifier_(return_value_reference_modifier)
	, arguments_( std::move(arguments) )
	, constructor_initialization_list_( std::move(constructor_initialization_list) )
	, block_( std::move(block) )
{}

FunctionDeclaration::~FunctionDeclaration()
{}

ClassDeclaration::ClassDeclaration( const FilePos& file_pos )
	: IProgramElement( file_pos )
{}

ClassDeclaration::~ClassDeclaration()
{}

Namespace::Namespace( const FilePos& file_pos )
	: IProgramElement(file_pos)
{}

} // namespace U
