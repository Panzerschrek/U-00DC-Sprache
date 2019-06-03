#include "assert.hpp"
#include "syntax_analyzer.hpp"

namespace U
{

namespace Synt
{

namespace Asserts
{

// Sizes for x86-64.
// If one of types inside variant becomes too big, put it inside "unique_ptr".

static_assert( sizeof(TypeName) <= 40u, "Size of variant too big" );
static_assert( sizeof(Expression) <= 128u, "Size of variant too big" );
static_assert( sizeof(Initializer) <= 160u, "Size of variant too big" );
static_assert( sizeof(BlockElement) <= 304u, "Size of variant too big" );
static_assert( sizeof(ClassElement) <= 208u, "Size of variant too big" );
static_assert( sizeof(ProgramElement) <= 208u, "Size of variant too big" );

}

SyntaxElementBase::SyntaxElementBase( const FilePos& file_pos )
	: file_pos_(file_pos)
{}

ArrayTypeName::ArrayTypeName( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

TypeofTypeName::TypeofTypeName( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

NamedTypeName::NamedTypeName( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

UnaryPlus::UnaryPlus( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

UnaryMinus::UnaryMinus( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

LogicalNot::LogicalNot( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

BitwiseNot::BitwiseNot( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

CallOperator::CallOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

IndexationOperator::IndexationOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

MemberAccessOperator::MemberAccessOperator(
	const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

ArrayInitializer::ArrayInitializer( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

StructNamedInitializer::StructNamedInitializer( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

ConstructorInitializer::ConstructorInitializer( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
	, call_operator( file_pos )
{}

ExpressionInitializer::ExpressionInitializer( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

ZeroInitializer::ZeroInitializer( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

UninitializedInitializer::UninitializedInitializer( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

BinaryOperator::BinaryOperator( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

ExpressionComponentWithUnaryOperators::ExpressionComponentWithUnaryOperators( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

NamedOperand::NamedOperand( const FilePos& file_pos, ComplexName name )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, name_( std::move(name) )
{}

MoveOperator::MoveOperator( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

CastRef::CastRef( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

CastRefUnsafe::CastRefUnsafe( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

CastImut::CastImut( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

CastMut::CastMut( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

TypeInfo::TypeInfo( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

BooleanConstant::BooleanConstant( const FilePos& file_pos, bool value )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, value_( value )
{}

NumericConstant::NumericConstant( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{
	std::fill( type_suffix_.begin(), type_suffix_.end(), 0 );
}

StringLiteral::StringLiteral( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{
	std::fill( type_suffix_.begin(), type_suffix_.end(), 0 );
}

BracketExpression::BracketExpression( const FilePos& file_pos)
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

TypeNameInExpression::TypeNameInExpression( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators( file_pos )
{}

Block::Block( const FilePos& start_file_pos )
	: SyntaxElementBase(start_file_pos)
{}

VariablesDeclaration::VariablesDeclaration( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

AutoVariableDeclaration::AutoVariableDeclaration( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

ReturnOperator::ReturnOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

WhileOperator::WhileOperator( const FilePos& file_pos)
	: SyntaxElementBase(file_pos)
	, block_(file_pos)
{}

BreakOperator::BreakOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

ContinueOperator::ContinueOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

IfOperator::IfOperator( const FilePos& start_file_pos )
	: SyntaxElementBase(start_file_pos)
{}

StaticIfOperator::StaticIfOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
	, if_operator_(file_pos)
{}

SingleExpressionOperator::SingleExpressionOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

AssignmentOperator::AssignmentOperator(
	const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

AdditiveAssignmentOperator::AdditiveAssignmentOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

IncrementOperator::IncrementOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

DecrementOperator::DecrementOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

StaticAssert::StaticAssert( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

Halt::Halt( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

HaltIf::HaltIf( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

Typedef::Typedef( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

Enum::Enum( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

FunctionArgument::FunctionArgument( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

FunctionType::FunctionType( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

Function::Function( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
	, type_(file_pos)
{}

ClassField::ClassField( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

ClassVisibilityLabel::ClassVisibilityLabel( const FilePos& file_pos, ClassMemberVisibility visibility )
 : SyntaxElementBase(file_pos), visibility_(visibility)
{}

Class::Class( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

TemplateBase::TemplateBase( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

TypeTemplateBase::TypeTemplateBase( const FilePos& file_pos )
	: TemplateBase( file_pos )
{}

ClassTemplate::ClassTemplate( const FilePos& file_pos )
	: TypeTemplateBase( file_pos )
{}

TypedefTemplate::TypedefTemplate( const FilePos& file_pos )
	: TypeTemplateBase( file_pos )
{}

FunctionTemplate::FunctionTemplate( const FilePos& file_pos )
	: TemplateBase( file_pos )
{}

Namespace::Namespace( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

Import::Import( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

FilePos GetExpressionFilePos( const Expression& expression )
{
	struct Visitor final : public boost::static_visitor<FilePos>
	{
		FilePos operator()( const EmptyVariant& ) const
		{
			return FilePos();
		}

		FilePos operator()( const ExpressionComponentWithUnaryOperators& expression_with_unary_operators ) const
		{
			return expression_with_unary_operators.file_pos_;
		}

		FilePos operator()( const BinaryOperator& binary_operator ) const
		{
			return binary_operator.file_pos_;
		}
	};

	return boost::apply_visitor( Visitor(), expression );
}

FilePos GetInitializerFilePos( const Initializer& initializer )
{
	struct Visitor final : public boost::static_visitor<FilePos>
	{
		FilePos operator()( const EmptyVariant& ) const
		{
			return FilePos();
		}

		FilePos operator()( const SyntaxElementBase& element ) const
		{
			return element.file_pos_;
		}
	};

	return boost::apply_visitor( Visitor(), initializer );
}

FilePos GetBlockElementFilePos( const BlockElement& block_element )
{
	struct Visitor final : public boost::static_visitor<FilePos>
	{
		FilePos operator()( const SyntaxElementBase& syntax_element ) const
		{
			return syntax_element.file_pos_;
		}
	};

	return boost::apply_visitor( Visitor(), block_element );
}

} // namespace Synt

} // namespace U
