#include "syntax_analyzer.hpp"

namespace Interpreter
{

UnaryPlus::~UnaryPlus()
{
}

UnaryMinus::~UnaryMinus()
{
}

CallOperator::CallOperator( std::vector<BinaryOperatorsChainPtr> arguments )
	: arguments_( std::move( arguments ) )
{
}

CallOperator::~CallOperator()
{
}

IndexationOperator::IndexationOperator( BinaryOperatorsChainPtr index )
	: index_( std::move( index ) )
{
}

IndexationOperator::~IndexationOperator()
{
}

NamedOperand::NamedOperand( ProgramString name )
	: name_( std::move(name) )
{
}

NamedOperand::~NamedOperand()
{
}

NumericConstant::NumericConstant( ProgramString value )
	: value_( std::move(value) )
{
}

NumericConstant::~NumericConstant()
{
}

BracketExpression::BracketExpression( BinaryOperatorsChainPtr expression )
	: expression_( std::move( expression ) )
{
}

BracketExpression::~BracketExpression()
{
}

Block::Block( BlockElements elements )
	: elements_( std::move( elements ) )
{
}

Block::~Block()
{
}

VariableDeclaration::~VariableDeclaration()
{
}

VariableDeclaration::VariableDeclaration()
{
}

VariableDeclaration::VariableDeclaration( VariableDeclaration&& other )
{
	*this= std::move(other);
}

VariableDeclaration& VariableDeclaration::operator=( VariableDeclaration&& other )
{
	name= std::move( other.name );
	type= std::move( other.type );
	initial_value= std::move( other.initial_value );

	return *this;
}

FunctionDeclaration::FunctionDeclaration(
	ProgramString name,
	ProgramString return_type,
	std::vector<VariableDeclaration> arguments,
	BlockPtr block )
	: name_( std::move(name) )
	, return_type_( std::move(return_type) )
	, arguments_( std::move(arguments) )
	, block_( std::move(block) )
{
}

FunctionDeclaration::~FunctionDeclaration()
{
}

} // namespace Interpreter
