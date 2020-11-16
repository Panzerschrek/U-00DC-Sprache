#include "../../lex_synt_lib_common/assert.hpp"
#include "syntax_analyzer.hpp"

namespace U
{

namespace Synt
{

namespace Asserts
{

// Sizes for x86-64.
// If one of types inside variant becomes too big, put it inside "unique_ptr".

static_assert( sizeof(TypeName) <= 64u, "Size of variant too big" );
static_assert( sizeof(Expression) <= 128u, "Size of variant too big" );
static_assert( sizeof(Initializer) <= 160u, "Size of variant too big" );
static_assert( sizeof(BlockElement) <= 288u, "Size of variant too big" );
static_assert( sizeof(ClassElement) <= 208u, "Size of variant too big" );
static_assert( sizeof(ProgramElement) <= 208u, "Size of variant too big" );

}

SyntaxElementBase::SyntaxElementBase( const SrcLoc& file_pos )
	: file_pos_(file_pos)
{}

ArrayTypeName::ArrayTypeName( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

TupleType::TupleType( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

TypeofTypeName::TypeofTypeName( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

NamedTypeName::NamedTypeName( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

UnaryPlus::UnaryPlus( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

UnaryMinus::UnaryMinus( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

LogicalNot::LogicalNot( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

BitwiseNot::BitwiseNot( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

CallOperator::CallOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

IndexationOperator::IndexationOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

MemberAccessOperator::MemberAccessOperator(
	const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

ArrayInitializer::ArrayInitializer( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

StructNamedInitializer::StructNamedInitializer( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

ConstructorInitializer::ConstructorInitializer( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
	, call_operator( file_pos )
{}

ExpressionInitializer::ExpressionInitializer( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

ZeroInitializer::ZeroInitializer( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

UninitializedInitializer::UninitializedInitializer( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

BinaryOperator::BinaryOperator( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

ExpressionComponentWithUnaryOperators::ExpressionComponentWithUnaryOperators( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

TernaryOperator::TernaryOperator( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators( file_pos )
{}

NamedOperand::NamedOperand( const SrcLoc& file_pos, ComplexName name )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, name_( std::move(name) )
{}

MoveOperator::MoveOperator( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

TakeOperator::TakeOperator( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

CastRef::CastRef( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

CastRefUnsafe::CastRefUnsafe( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

CastImut::CastImut( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

CastMut::CastMut( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

TypeInfo::TypeInfo( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

BooleanConstant::BooleanConstant( const SrcLoc& file_pos, bool value )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, value_( value )
{}

NumericConstant::NumericConstant( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{
}

StringLiteral::StringLiteral( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{
	std::fill( type_suffix_.begin(), type_suffix_.end(), 0 );
}

BracketExpression::BracketExpression( const SrcLoc& file_pos)
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

TypeNameInExpression::TypeNameInExpression( const SrcLoc& file_pos )
	: ExpressionComponentWithUnaryOperators( file_pos )
{}

Block::Block( const SrcLoc& start_file_pos )
	: SyntaxElementBase(start_file_pos)
{}

VariablesDeclaration::VariablesDeclaration( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

AutoVariableDeclaration::AutoVariableDeclaration( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

ReturnOperator::ReturnOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

WhileOperator::WhileOperator( const SrcLoc& file_pos)
	: SyntaxElementBase(file_pos)
	, block_(file_pos)
{}

ForOperator::ForOperator( const SrcLoc& file_pos)
	: SyntaxElementBase(file_pos)
	, block_(file_pos)
{}

CStyleForOperator::CStyleForOperator( const SrcLoc& file_pos)
	: SyntaxElementBase(file_pos)
	, block_(file_pos)
{}

BreakOperator::BreakOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

ContinueOperator::ContinueOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

WithOperator::WithOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
	, block_(file_pos)
{}

IfOperator::IfOperator( const SrcLoc& start_file_pos )
	: SyntaxElementBase(start_file_pos)
{}

StaticIfOperator::StaticIfOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
	, if_operator_(file_pos)
{}

SingleExpressionOperator::SingleExpressionOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

AssignmentOperator::AssignmentOperator(
	const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

AdditiveAssignmentOperator::AdditiveAssignmentOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

IncrementOperator::IncrementOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

DecrementOperator::DecrementOperator( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

StaticAssert::StaticAssert( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

Halt::Halt( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

HaltIf::HaltIf( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

Typedef::Typedef( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

Enum::Enum( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

FunctionArgument::FunctionArgument( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

FunctionType::FunctionType( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

Function::Function( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
	, type_(file_pos)
{}

ClassField::ClassField( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

ClassVisibilityLabel::ClassVisibilityLabel( const SrcLoc& file_pos, ClassMemberVisibility visibility )
 : SyntaxElementBase(file_pos), visibility_(visibility)
{}

Class::Class( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

TemplateBase::TemplateBase( const SrcLoc& file_pos )
	: SyntaxElementBase( file_pos )
{}

TypeTemplate::TypeTemplate( const SrcLoc& file_pos )
	: TemplateBase( file_pos )
{}

FunctionTemplate::FunctionTemplate( const SrcLoc& file_pos )
	: TemplateBase( file_pos )
{}

Namespace::Namespace( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

Import::Import( const SrcLoc& file_pos )
	: SyntaxElementBase(file_pos)
{}

struct GetFilePosVisitor final
{
	SrcLoc operator()( const EmptyVariant& ) const
	{
		return SrcLoc();
	}

	SrcLoc operator()( const SyntaxElementBase& element ) const
	{
		return element.file_pos_;
	}
};

SrcLoc GetExpressionFilePos( const Expression& expression )
{

	return std::visit( GetFilePosVisitor(), expression );
}

SrcLoc GetInitializerFilePos( const Initializer& initializer )
{
	return std::visit( GetFilePosVisitor(), initializer );
}

SrcLoc GetBlockElementFilePos( const BlockElement& block_element )
{
	return std::visit( GetFilePosVisitor(), block_element );
}

namespace
{

OverloadedOperator PrefixOperatorKind( const UnaryPlus& ) { return OverloadedOperator::Add; }
OverloadedOperator PrefixOperatorKind( const UnaryMinus& ) { return OverloadedOperator::Sub; }
OverloadedOperator PrefixOperatorKind( const LogicalNot& ) { return OverloadedOperator::LogicalNot; }
OverloadedOperator PrefixOperatorKind( const BitwiseNot& ) { return OverloadedOperator::BitwiseNot; }

}

OverloadedOperator PrefixOperatorKind( const UnaryPrefixOperator& prefix_operator )
{
	return
		std::visit(
			[]( const auto& t ) { return PrefixOperatorKind(t); },
			prefix_operator );
}

} // namespace Synt

} // namespace U
