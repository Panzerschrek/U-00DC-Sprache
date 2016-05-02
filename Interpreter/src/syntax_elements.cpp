#include "assert.hpp"
#include "syntax_analyzer.hpp"

namespace Interpreter
{

static void PrintIndents( std::ostream& stream, unsigned int indents )
{
	for( unsigned int i= 0; i < indents; i++ )
		stream << "    ";
}

UnaryPlus::~UnaryPlus()
{
}

void UnaryPlus::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << "+" ;
}

UnaryMinus::~UnaryMinus()
{
}

void UnaryMinus::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << "-" ;
}

CallOperator::CallOperator( std::vector<BinaryOperatorsChainPtr> arguments )
	: arguments_( std::move( arguments ) )
{
}

CallOperator::~CallOperator()
{
}

void CallOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "( ";
	for( const BinaryOperatorsChainPtr& arg : arguments_ )
	{
		arg->Print( stream, indent );
		if( arg != arguments_.back() )
			stream << ", ";
	}

	stream << " )";
}

IndexationOperator::IndexationOperator( BinaryOperatorsChainPtr index )
	: index_( std::move( index ) )
{
}

IndexationOperator::~IndexationOperator()
{
}

void IndexationOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "[ ";
	index_->Print( stream, indent );
	stream << " ]";
}

NamedOperand::NamedOperand( ProgramString name )
	: name_( std::move(name) )
{
}

void NamedOperand::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << ToStdString( name_ );
}

NamedOperand::~NamedOperand()
{
}

NumericConstant::NumericConstant( ProgramString value )
	: value_( std::move(value) )
{
}

void NumericConstant::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << ToStdString( value_ );
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

void BracketExpression::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "( ";
	expression_->Print( stream, indent );
	stream << " )";
}

void BinaryOperatorsChain::Print( std::ostream& stream, unsigned int indent ) const
{
	for( const ComponentWithOperator& comp : components )
	{
		for( const IUnaryPrefixOperatorPtr& prefix_operator : comp.prefix_operators )
			prefix_operator->Print( stream, indent );

		comp.component->Print( stream, indent );

		for( const IUnaryPostfixOperatorPtr& postfix_operator : comp.postfix_operators )
			postfix_operator->Print( stream, indent );

		const char* op= "";
		switch( comp.op )
		{
			case BinaryOperator::Add: op= "+"; break;
			case BinaryOperator::Sub: op= "-"; break;
			case BinaryOperator::Mul: op= "*"; break;
			case BinaryOperator::Div: op= "/"; break;
			case BinaryOperator::None: op= ""; break;
		};

		stream << op;
	}
}

Block::Block( BlockElements elements )
	: elements_( std::move( elements ) )
{
}

Block::~Block()
{
}

void Block::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "{\n";

	for( const IBlockElementPtr& element : elements_ )
	{
		PrintIndents( stream, indent + 1 );
		element->Print( stream, indent + 1 );
		stream << "\n";
	}

	PrintIndents( stream, indent );
	stream << "}\n";
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

void VariableDeclaration::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "let " << ToStdString( name ) << " : " << ToStdString( type );
	if( initial_value )
	{
		stream << " = ";
		initial_value->Print( stream, indent );
	}

	stream << ";";
}

ReturnOperator::ReturnOperator( BinaryOperatorsChainPtr expression )
	: expression_( std::move( expression ) )
{
}

ReturnOperator::~ReturnOperator()
{
}

void ReturnOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "return ";
	if( expression_ )
		expression_->Print( stream, indent );
	stream << ";";
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

void FunctionDeclaration::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "fn " << ToStdString( name_ ) << "( ";
	for( const VariableDeclaration& decl : arguments_ )
	{
		decl.Print( stream, indent );
		if( &decl != &arguments_.back() )
			stream << ", ";
	}
	stream << " )";

	if( ! return_type_.empty() )
		stream << " : " << ToStdString( return_type_ );

	stream << "\n";

	block_->Print( stream, indent);
}

} // namespace Interpreter
