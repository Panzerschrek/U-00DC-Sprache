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

IUnaryPrefixOperatorPtr UnaryPlus::Clone() const
{
	return IUnaryPrefixOperatorPtr( new UnaryPlus() );
}

void UnaryPlus::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << "+" ;
}

UnaryMinus::~UnaryMinus()
{
}

IUnaryPrefixOperatorPtr UnaryMinus::Clone() const
{
	return IUnaryPrefixOperatorPtr( new UnaryMinus() );
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

IUnaryPostfixOperatorPtr CallOperator::Clone() const
{
	std::vector<BinaryOperatorsChainPtr> arguments_copy;
	arguments_copy.reserve( arguments_.size() );
	for( const BinaryOperatorsChainPtr& expression : arguments_ )
		arguments_copy.emplace_back( new BinaryOperatorsChain( *expression ) );

	return IUnaryPostfixOperatorPtr( new CallOperator( std::move( arguments_copy ) ) );
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
	U_ASSERT( index_ );
}

IndexationOperator::~IndexationOperator()
{
}

IUnaryPostfixOperatorPtr IndexationOperator::Clone() const
{
	BinaryOperatorsChainPtr index_copy( new BinaryOperatorsChain( *index_ ) );

	return
		IUnaryPostfixOperatorPtr(
			new IndexationOperator( std::move( index_copy ) ) );
}

void PrintOperator( std::ostream& stream, BinaryOperator op )
{
	const char* op_str= "";
	switch( op )
	{
		case BinaryOperator::None: op_str= ""; break;

		case BinaryOperator::Add: op_str= " + "; break;
		case BinaryOperator::Sub: op_str= " - "; break;
		case BinaryOperator::Mul: op_str= " * "; break;
		case BinaryOperator::Div: op_str= " / "; break;

		case BinaryOperator::Equal: op_str= " == "; break;
		case BinaryOperator::NotEqual: op_str= " != "; break;
		case BinaryOperator::Less: op_str= " < "; break;
		case BinaryOperator::LessEqual: op_str= " <= "; break;
		case BinaryOperator::Greater: op_str= " > "; break;
		case BinaryOperator::GreaterEqual: op_str= " >= "; break;

		case BinaryOperator::Last: U_ASSERT(false); break;
	};

	stream << op_str;
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

IBinaryOperatorsChainComponentPtr NamedOperand::Clone() const
{
	return IBinaryOperatorsChainComponentPtr( new NamedOperand( name_ ) );
}

BooleanConstant::BooleanConstant( bool value )
	: value_( value )
{
}

BooleanConstant::~BooleanConstant()
{
}

IBinaryOperatorsChainComponentPtr BooleanConstant::Clone() const
{
	return IBinaryOperatorsChainComponentPtr( new BooleanConstant( value_ ) );
}

void BooleanConstant::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << ( value_ ? "true" : "false" );
}

NumericConstant::NumericConstant(
	LongFloat value,
	ProgramString type_suffix,
	bool has_fractional_point )
	: value_( value )
	, type_suffix_( std::move(type_suffix) )
	, has_fractional_point_( has_fractional_point )
{
}

void NumericConstant::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << ((long double)value_) << ToStdString( type_suffix_ );
}

NumericConstant::~NumericConstant()
{
}

IBinaryOperatorsChainComponentPtr NumericConstant::Clone() const
{
	return
		IBinaryOperatorsChainComponentPtr(
			new NumericConstant(
				value_,
				type_suffix_,
				has_fractional_point_ ) );
}

BracketExpression::BracketExpression( BinaryOperatorsChainPtr expression )
	: expression_( std::move( expression ) )
{
}

BracketExpression::~BracketExpression()
{
}

IBinaryOperatorsChainComponentPtr BracketExpression::Clone() const
{
	BinaryOperatorsChainPtr expression_copy;
	expression_copy.reset( new BinaryOperatorsChain() );
	expression_copy->components= expression_->components;

	return
		IBinaryOperatorsChainComponentPtr(
			new BracketExpression( std::move( expression_copy ) ) );
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

		PrintOperator( stream, comp.op );
	}
}

BinaryOperatorsChain::ComponentWithOperator::ComponentWithOperator()
{
}

BinaryOperatorsChain::ComponentWithOperator::ComponentWithOperator(
	const ComponentWithOperator& other )
{
	*this= other;
}

BinaryOperatorsChain::ComponentWithOperator::ComponentWithOperator(
	ComponentWithOperator&& other )
{
	*this= std::move( other );
}

BinaryOperatorsChain::ComponentWithOperator&
	BinaryOperatorsChain::ComponentWithOperator::operator=(
		const ComponentWithOperator& other )
{
	prefix_operators.reserve( other.prefix_operators.size() );
	for( const IUnaryPrefixOperatorPtr& op : other.prefix_operators )
		prefix_operators.emplace_back( op->Clone() );

	component= other.component->Clone();

	postfix_operators.reserve( other.postfix_operators.size() );
	for( const IUnaryPostfixOperatorPtr& op : other.postfix_operators )
		postfix_operators.emplace_back( op->Clone() );

	op= other.op;

	return *this;
}

BinaryOperatorsChain::ComponentWithOperator&
	BinaryOperatorsChain::ComponentWithOperator::operator=(
		ComponentWithOperator&& other )
{
	prefix_operators= std::move( other.prefix_operators );
	component= std::move( other.component );
	postfix_operators= std::move( other.postfix_operators );
	op= std::move( other.op );

	return *this;
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

WhileOperator::WhileOperator( BinaryOperatorsChainPtr condition, BlockPtr block )
	: condition_( std::move( condition ) )
	, block_( std::move( block ) )
{
}

WhileOperator::~WhileOperator()
{
}

void WhileOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "while( ";
	condition_->Print( stream, indent );
	stream << " )" << "\n";

	PrintIndents( stream, indent );
	block_->Print( stream, indent  );
}

BreakOperator::~BreakOperator()
{
}

void BreakOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED(indent);
	stream << "break;";
}

ContinueOperator::~ContinueOperator()
{
}

void ContinueOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED(indent);
	stream << "continue;";
}

IfOperator::IfOperator( std::vector<Branch> branches )
	: branches_( std::move( branches ) )
{
}

IfOperator::~IfOperator()
{
}

void IfOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	for( const Branch& branch : branches_ )
	{
		bool first= &branch == &branches_.front();
		if( !first )
			PrintIndents( stream, indent );

		if( branch.condition )
		{
			stream << ( first ? "if( " : "else if( ");
			branch.condition->Print( stream, indent );
			stream << " )\n";
		}
		else
			stream << "else\n";


		PrintIndents( stream, indent );
		branch.block->Print( stream, indent );
	}
}

SingleExpressionOperator::SingleExpressionOperator( BinaryOperatorsChainPtr expression )
	: expression_( std::move( expression ) )
{
}

SingleExpressionOperator::~SingleExpressionOperator()
{
}

void SingleExpressionOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	expression_->Print( stream, indent );
	stream << ";";
}

AssignmentOperator::AssignmentOperator( BinaryOperatorsChainPtr l_value, BinaryOperatorsChainPtr r_value )
	: l_value_( std::move( l_value ) )
	, r_value_( std::move( r_value ) )
{
}

AssignmentOperator::~AssignmentOperator()
{
}

void AssignmentOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	l_value_->Print( stream, indent );
	stream << "= ";
	r_value_->Print( stream, indent );
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
