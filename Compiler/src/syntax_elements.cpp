#include "assert.hpp"
#include "syntax_analyzer.hpp"

namespace U
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
	std::vector<IExpressionComponentPtr> arguments )
	: IUnaryPostfixOperator(file_pos)
	, arguments_( std::move( arguments ) )
{}

CallOperator::~CallOperator()
{}

IUnaryPostfixOperatorPtr CallOperator::Clone() const
{
	std::vector<IExpressionComponentPtr> arguments_copy;
	arguments_copy.reserve( arguments_.size() );
	for( const IExpressionComponentPtr& expression : arguments_ )
		arguments_copy.emplace_back( expression->Clone() );

	return IUnaryPostfixOperatorPtr( new CallOperator( file_pos_, std::move( arguments_copy ) ) );
}

void CallOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "( ";
	for( const IExpressionComponentPtr& arg : arguments_ )
	{
		arg->Print( stream, indent );
		if( arg != arguments_.back() )
			stream << ", ";
	}

	stream << " )";
}

IndexationOperator::IndexationOperator( const FilePos& file_pos, IExpressionComponentPtr index )
	: IUnaryPostfixOperator(file_pos)
	,index_( std::move( index ) )
{
	U_ASSERT( index_ );
}

IndexationOperator::~IndexationOperator()
{}

IUnaryPostfixOperatorPtr IndexationOperator::Clone() const
{
	IExpressionComponentPtr index_copy= index_->Clone();

	return
		IUnaryPostfixOperatorPtr(
			new IndexationOperator( file_pos_, std::move( index_copy ) ) );
}

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

IExpressionComponent::IExpressionComponent( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

IInitializer::IInitializer( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

ArrayInitializer::ArrayInitializer( const FilePos& file_pos )
	: IInitializer( file_pos )
{}

void ArrayInitializer::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "[ ";
	for( const IInitializerPtr& initializer : initializers )
	{
		initializer->Print( stream, indent );
		if( &initializer != &initializers.back() )
			stream << ", ";
	}
	if( has_continious_initializer )
		stream << "... ";
	stream << "]";
}

StructNamedInitializer::StructNamedInitializer( const FilePos& file_pos )
	: IInitializer( file_pos )
{}

void StructNamedInitializer::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "{ ";
	for( const MemberInitializer& members_initializer : members_initializers )
	{
		stream << "." << ToStdString(members_initializer.name);
		members_initializer.initializer->Print( stream, indent );
		if( &members_initializer != &members_initializers.back() )
			stream << ", ";
	}
	stream << "}";
}

ConstructorInitializer::ConstructorInitializer(
	const FilePos& file_pos,
	std::vector<IExpressionComponentPtr> arguments )
	: IInitializer( file_pos )
	, call_operator( file_pos, std::move(arguments) )
{}

void ConstructorInitializer::Print( std::ostream& stream, unsigned int indent ) const
{
	call_operator.Print( stream, indent );
}

ExpressionInitializer::ExpressionInitializer(
	const FilePos& file_pos , IExpressionComponentPtr in_expression )
	: IInitializer( file_pos )
	, expression(std::move(in_expression))
{}

void ExpressionInitializer::Print( std::ostream& stream, unsigned int indent ) const
{
	expression->Print( stream, indent );
}

ZeroInitializer::ZeroInitializer( const FilePos& file_pos )
	: IInitializer(file_pos)
{}


void ZeroInitializer::Print( std::ostream& stream, const unsigned int indent ) const
{
	U_UNUSED(indent);
	stream << "zero_init";
}

BinaryOperator::BinaryOperator( const FilePos& file_pos )
	: IExpressionComponent( file_pos )
{}

IExpressionComponentPtr BinaryOperator::Clone() const
{
	std::unique_ptr<BinaryOperator> copy( new BinaryOperator( file_pos_ ) );
	copy->operator_type_= operator_type_;
	copy->left_ = left_ ->Clone();
	copy->right_= right_->Clone();

	return std::move( copy );
}

void BinaryOperator::Print( std::ostream& stream, unsigned int indent ) const
{
	left_ ->Print( stream, indent );
	stream << ToStdString( BinaryOperatorToString( operator_type_ ) );
	right_->Print( stream, indent );
}

ExpressionComponentWithUnaryOperators::ExpressionComponentWithUnaryOperators( const FilePos& file_pos )
	: IExpressionComponent( file_pos )
{}

NamedOperand::NamedOperand( const FilePos& file_pos, ProgramString name )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, name_( std::move(name) )
{}

void NamedOperand::Print( std::ostream& stream, unsigned int indent ) const
{
	U_UNUSED( indent );
	stream << ToStdString( name_ );
}

NamedOperand::~NamedOperand()
{}

IExpressionComponentPtr NamedOperand::Clone() const
{
	return IExpressionComponentPtr( new NamedOperand( file_pos_, name_ ) );
}

BooleanConstant::BooleanConstant( const FilePos& file_pos, bool value )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, value_( value )
{}

BooleanConstant::~BooleanConstant()
{}

IExpressionComponentPtr BooleanConstant::Clone() const
{
	return IExpressionComponentPtr( new BooleanConstant( file_pos_, value_ ) );
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
	: ExpressionComponentWithUnaryOperators(file_pos)
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

IExpressionComponentPtr NumericConstant::Clone() const
{
	return
		IExpressionComponentPtr(
			new NumericConstant(
				file_pos_,
				value_,
				type_suffix_,
				has_fractional_point_ ) );
}

BracketExpression::BracketExpression( const FilePos& file_pos, IExpressionComponentPtr expression )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, expression_( std::move( expression ) )
{}

BracketExpression::~BracketExpression()
{}

IExpressionComponentPtr BracketExpression::Clone() const
{
	return
		IExpressionComponentPtr(
			new BracketExpression( file_pos_,  expression_->Clone() ) );
}

void BracketExpression::Print( std::ostream& stream, unsigned int indent ) const
{
	stream << "( ";
	expression_->Print( stream, indent );
	stream << " )";
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
	stream << "var ";
	type.Print( stream );
	stream << " ";

	for( const VariableEntry& variable : variables )
	{
		if( variable.reference_modifier == ReferenceModifier::Reference )
			stream << "&";
		if( variable.mutability_modifier == MutabilityModifier::Mutable )
			stream << "mut ";
		else if( variable.mutability_modifier == MutabilityModifier::Immutable )
			stream << "imut ";

		stream << ToStdString( variable.name );
		if( variable.initializer != nullptr )
		{
			stream << " = ";
			variable.initializer->Print( stream, indent );
		}

		if( &variable != &variables.back() )
			stream << ", ";
		else
			stream << ";";
	}
}

AutoVariableDeclaration::AutoVariableDeclaration( const FilePos& file_pos )
	: IBlockElement( file_pos )
{}

void AutoVariableDeclaration::Print( std::ostream& stream, const unsigned int indent ) const
{
	stream << "auto ";

	if( reference_modifier == ReferenceModifier::Reference )
		stream << "&";
	if( mutability_modifier == MutabilityModifier::Mutable )
		stream << "mut ";
	else if( mutability_modifier == MutabilityModifier::Immutable )
		stream << "imut ";

	stream << ToStdString( name );
	stream << "= ";
	initializer_expression->Print( stream, indent );
	stream << ";";
}

ReturnOperator::ReturnOperator( const FilePos& file_pos, IExpressionComponentPtr expression )
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

WhileOperator::WhileOperator( const FilePos& file_pos, IExpressionComponentPtr condition, BlockPtr block )
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

SingleExpressionOperator::SingleExpressionOperator( const FilePos& file_pos, IExpressionComponentPtr expression )
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
	IExpressionComponentPtr l_value,
	IExpressionComponentPtr r_value )
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
	MutabilityModifier return_value_mutability_modifier,
	ReferenceModifier return_value_reference_modifier,
	FunctionArgumentsDeclaration arguments,
	BlockPtr block )
	: IProgramElement(file_pos)
	, name_( std::move(name) )
	, return_type_( std::move(return_type) )
	, return_value_mutability_modifier_(return_value_mutability_modifier)
	, return_value_reference_modifier_(return_value_reference_modifier)
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
	for( const Member& member : members_ )
	{
		if( const Field* const field=
			boost::get< Field >( &member ) )
		{
			PrintIndents( stream, 1 );
			field->type.Print( stream );
			stream << " " << ToStdString( field->name ) << ";\n";
		}
		else if( const std::unique_ptr<FunctionDeclaration>* const function_declaration=
			boost::get< std::unique_ptr<FunctionDeclaration> >( &member ) )
		{
			(*function_declaration)->Print( stream, indent );
		}
		else
		{
			U_ASSERT( false );
		}

	}
	stream << "}\n";
}

} // namespace U
