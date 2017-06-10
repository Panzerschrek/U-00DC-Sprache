#include "assert.hpp"
#include "syntax_analyzer.hpp"

namespace Interpreter
{

static void PrintIndents( std::ostream& stream, unsigned int indents )
{
	for( unsigned int i= 0; i < indents; i++ )
		stream << "    ";
}

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

IUnaryPrefixOperatorPtr UnaryPlus::Clone() const
{
	return IUnaryPrefixOperatorPtr( new UnaryPlus( file_pos_ ) );
}

void UnaryPlus::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << "+" ;
}

UnaryMinus::UnaryMinus( const FilePos& file_pos )
	: IUnaryPrefixOperator(file_pos)
{}

UnaryMinus::~UnaryMinus()
{}

IUnaryPrefixOperatorPtr UnaryMinus::Clone() const
{
	return IUnaryPrefixOperatorPtr( new UnaryMinus( file_pos_ ) );
}

void UnaryMinus::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << "-" ;
}

CallOperator::CallOperator(
	const FilePos& file_pos,
	std::vector<BinaryOperatorsChainPtr> arguments )
	: IUnaryPostfixOperator(file_pos)
	, arguments_( std::move( arguments ) )
{}

CallOperator::~CallOperator()
{}

IUnaryPostfixOperatorPtr CallOperator::Clone() const
{
	std::vector<BinaryOperatorsChainPtr> arguments_copy;
	arguments_copy.reserve( arguments_.size() );
	for( const BinaryOperatorsChainPtr& expression : arguments_ )
		arguments_copy.emplace_back( new BinaryOperatorsChain( *expression ) );

	return IUnaryPostfixOperatorPtr( new CallOperator( file_pos_, std::move( arguments_copy ) ) );
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

IndexationOperator::IndexationOperator( const FilePos& file_pos, BinaryOperatorsChainPtr index )
	: IUnaryPostfixOperator(file_pos)
	,index_( std::move( index ) )
{
	U_ASSERT( index_ );
}

IndexationOperator::~IndexationOperator()
{}

IUnaryPostfixOperatorPtr IndexationOperator::Clone() const
{
	BinaryOperatorsChainPtr index_copy( new BinaryOperatorsChain( *index_ ) );

	return
		IUnaryPostfixOperatorPtr(
			new IndexationOperator( file_pos_, std::move( index_copy ) ) );
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

		case BinaryOperator::And: op_str= " & "; break;
		case BinaryOperator::Or: op_str= " | "; break;
		case BinaryOperator::Xor: op_str= " ^ "; break;

		case BinaryOperator::LazyLogicalAnd: op_str= " && "; break;
		case BinaryOperator::LazyLogicalOr: op_str= " || "; break;

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

MemberAccessOperator::MemberAccessOperator(
	const FilePos& file_pos,
	ProgramString member_name )
	: IUnaryPostfixOperator( file_pos )
	, member_name_( std::move(member_name) )
{}

MemberAccessOperator::~MemberAccessOperator()
{}

IUnaryPostfixOperatorPtr MemberAccessOperator::Clone() const
{
	return
		IUnaryPostfixOperatorPtr(
			new MemberAccessOperator(
				file_pos_,
				member_name_ ) );
}

void MemberAccessOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << "." << ToStdString( member_name_ );
}

IBinaryOperatorsChainComponent::IBinaryOperatorsChainComponent( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

NamedOperand::NamedOperand( const FilePos& file_pos, ProgramString name )
	: IBinaryOperatorsChainComponent(file_pos)
	, name_( std::move(name) )
{}

void NamedOperand::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << ToStdString( name_ );
}

NamedOperand::~NamedOperand()
{}

IBinaryOperatorsChainComponentPtr NamedOperand::Clone() const
{
	return IBinaryOperatorsChainComponentPtr( new NamedOperand( file_pos_, name_ ) );
}

BooleanConstant::BooleanConstant( const FilePos& file_pos, bool value )
	: IBinaryOperatorsChainComponent(file_pos)
	, value_( value )
{}

BooleanConstant::~BooleanConstant()
{}

IBinaryOperatorsChainComponentPtr BooleanConstant::Clone() const
{
	return IBinaryOperatorsChainComponentPtr( new BooleanConstant( file_pos_, value_ ) );
}

void BooleanConstant::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << ( value_ ? "true" : "false" );
}

NumericConstant::NumericConstant(
	const FilePos& file_pos,
	LongFloat value,
	ProgramString type_suffix,
	bool has_fractional_point )
	: IBinaryOperatorsChainComponent(file_pos)
	, value_( value )
	, type_suffix_( std::move(type_suffix) )
	, has_fractional_point_( has_fractional_point )
{}

void NumericConstant::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << ((long double)value_) << ToStdString( type_suffix_ );
}

NumericConstant::~NumericConstant()
{}

IBinaryOperatorsChainComponentPtr NumericConstant::Clone() const
{
	return
		IBinaryOperatorsChainComponentPtr(
			new NumericConstant(
				file_pos_,
				value_,
				type_suffix_,
				has_fractional_point_ ) );
}

BracketExpression::BracketExpression( const FilePos& file_pos, BinaryOperatorsChainPtr expression )
	: IBinaryOperatorsChainComponent(file_pos)
	, expression_( std::move( expression ) )
{}

BracketExpression::~BracketExpression()
{}

IBinaryOperatorsChainComponentPtr BracketExpression::Clone() const
{
	BinaryOperatorsChainPtr expression_copy;
	expression_copy.reset( new BinaryOperatorsChain( expression_->file_pos_ ) );
	expression_copy->components= expression_->components;

	return
		IBinaryOperatorsChainComponentPtr(
			new BracketExpression( file_pos_, std::move( expression_copy ) ) );
}

void BracketExpression::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "( ";
	expression_->Print( stream, indent );
	stream << " )";
}

BinaryOperatorsChain::BinaryOperatorsChain( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

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
{}

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

void TypeName::Print( std::ostream& stream ) const
{
	for( const std::unique_ptr<NumericConstant>& num : array_sizes )
	{
		U_UNUSED( num );
		stream << "[ ";
	}

	stream << ToStdString( name );

	for( const std::unique_ptr<NumericConstant>& num : array_sizes )
	{
		U_UNUSED( num );
		stream << ", ";
		num->Print( stream, 0 );
		stream << " ]";
	}
}

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

void VariablesDeclaration::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "let : ";
	type.Print( stream );
	stream << " ";

	for( const VariableEntry& variable : variables )
	{
		stream << ToStdString( variable.name );
		if( variable.initial_value != nullptr )
		{
			stream << " = ";
			variable.initial_value->Print( stream, indent );
		}

		if( &variable != &variables.back() )
			stream << ", ";
		else
			stream << ";";
	}
}

ReturnOperator::ReturnOperator( const FilePos& file_pos, BinaryOperatorsChainPtr expression )
	: IBlockElement(file_pos)
	, expression_( std::move( expression ) )
{}

ReturnOperator::~ReturnOperator()
{}

void ReturnOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "return ";
	if( expression_ )
		expression_->Print( stream, indent );
	stream << ";";
}

WhileOperator::WhileOperator( const FilePos& file_pos, BinaryOperatorsChainPtr condition, BlockPtr block )
	: IBlockElement(file_pos)
	, condition_( std::move( condition ) )
	, block_( std::move( block ) )
{}

WhileOperator::~WhileOperator()
{}

void WhileOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "while( ";
	condition_->Print( stream, indent );
	stream << " )" << "\n";

	PrintIndents( stream, indent );
	block_->Print( stream, indent  );
}

BreakOperator::BreakOperator( const FilePos& file_pos )
	: IBlockElement(file_pos)
{}

BreakOperator::~BreakOperator()
{}

void BreakOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED(indent);
	stream << "break;";
}

ContinueOperator::ContinueOperator( const FilePos& file_pos )
	: IBlockElement(file_pos)
{}

ContinueOperator::~ContinueOperator()
{}

void ContinueOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED(indent);
	stream << "continue;";
}

IfOperator::IfOperator( const FilePos& file_pos, std::vector<Branch> branches )
	: IBlockElement(file_pos)
	, branches_( std::move( branches ) )
{}

IfOperator::~IfOperator()
{}

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

SingleExpressionOperator::SingleExpressionOperator( const FilePos& file_pos, BinaryOperatorsChainPtr expression )
	: IBlockElement(file_pos)
	, expression_( std::move( expression ) )
{}

SingleExpressionOperator::~SingleExpressionOperator()
{}

void SingleExpressionOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	expression_->Print( stream, indent );
	stream << ";";
}

AssignmentOperator::AssignmentOperator(
	const FilePos& file_pos,
	BinaryOperatorsChainPtr l_value,
	BinaryOperatorsChainPtr r_value )
	: IBlockElement(file_pos)
	, l_value_( std::move( l_value ) )
	, r_value_( std::move( r_value ) )
{}

AssignmentOperator::~AssignmentOperator()
{}

void AssignmentOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	l_value_->Print( stream, indent );
	stream << "= ";
	r_value_->Print( stream, indent );
	stream << ";";
}

FunctionArgumentDeclaration::FunctionArgumentDeclaration(
	const FilePos& file_pos,
	ProgramString name,
	TypeName type,
	MutabilityModifier mutability_modifier,
	ReferenceModifier reference_modifier )
	: IProgramElement( file_pos )
	, name_(std::move(name))
	, type_(std::move(type))
	, mutability_modifier_(mutability_modifier)
	, reference_modifier_(reference_modifier)
{}

FunctionArgumentDeclaration::~FunctionArgumentDeclaration()
{}

void FunctionArgumentDeclaration::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );

	type_.Print( stream );
	stream << " " << ToStdString( name_ );
}

FunctionDeclaration::FunctionDeclaration(
	const FilePos& file_pos,
	ProgramString name,
	ProgramString return_type,
	FunctionArgumentsDeclaration arguments,
	BlockPtr block )
	: IProgramElement(file_pos)
	, name_( std::move(name) )
	, return_type_( std::move(return_type) )
	, arguments_( std::move(arguments) )
	, block_( std::move(block) )
{}

FunctionDeclaration::~FunctionDeclaration()
{}

void FunctionDeclaration::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "fn " << ToStdString( name_ ) << "( ";
	for( const FunctionArgumentDeclarationPtr& decl : arguments_ )
	{
		decl->Print( stream, indent );
		if( &decl != &arguments_.back() )
			stream << ", ";
	}
	stream << " )";

	if( ! return_type_.empty() )
		stream << " : " << ToStdString( return_type_ );

	stream << "\n";

	block_->Print( stream, indent);
}

ClassDeclaration::ClassDeclaration( const FilePos& file_pos )
	: IProgramElement( file_pos )
{}

ClassDeclaration::~ClassDeclaration()
{}

void ClassDeclaration::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED(indent);

	stream << "class " << ToStdString( name_ ) << "\n";
	stream << "{\n";
	for( const Field& field : fields_ )
	{
		PrintIndents( stream, 1 );
		stream << ToStdString( field.name ) << " : ";
		field.type.Print( stream );
		stream << ";\n";
	}
	stream << "}\n";
}

} // namespace Interpreter
