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

SyntaxElementBase::SyntaxElementBase( const SrcLoc& src_loc )
	: src_loc_(src_loc)
{}

ArrayTypeName::ArrayTypeName( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

TupleType::TupleType( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

RawPointerType::RawPointerType( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

TypeofTypeName::TypeofTypeName( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

NamedTypeName::NamedTypeName( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

UnaryPlus::UnaryPlus( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

UnaryMinus::UnaryMinus( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

LogicalNot::LogicalNot( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

BitwiseNot::BitwiseNot( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

CallOperator::CallOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

IndexationOperator::IndexationOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

MemberAccessOperator::MemberAccessOperator(
	const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

ArrayInitializer::ArrayInitializer( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

StructNamedInitializer::StructNamedInitializer( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

ConstructorInitializer::ConstructorInitializer( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
	, call_operator( src_loc )
{}

ExpressionInitializer::ExpressionInitializer( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

ZeroInitializer::ZeroInitializer( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

UninitializedInitializer::UninitializedInitializer( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

BinaryOperator::BinaryOperator( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

ExpressionComponentWithUnaryOperators::ExpressionComponentWithUnaryOperators( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

TernaryOperator::TernaryOperator( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators( src_loc )
{}

ReferenceToRawPointerOperator::ReferenceToRawPointerOperator( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators( src_loc )
{}

RawPointerToReferenceOperator::RawPointerToReferenceOperator( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators( src_loc )
{}

NamedOperand::NamedOperand( const SrcLoc& src_loc, ComplexName name )
	: ExpressionComponentWithUnaryOperators(src_loc)
	, name_( std::move(name) )
{}

MoveOperator::MoveOperator( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators(src_loc)
{}

TakeOperator::TakeOperator( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators(src_loc)
{}

CastRef::CastRef( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators(src_loc)
{}

CastRefUnsafe::CastRefUnsafe( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators(src_loc)
{}

CastImut::CastImut( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators(src_loc)
{}

CastMut::CastMut( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators(src_loc)
{}

TypeInfo::TypeInfo( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators(src_loc)
{}

BooleanConstant::BooleanConstant( const SrcLoc& src_loc, bool value )
	: ExpressionComponentWithUnaryOperators(src_loc)
	, value_( value )
{}

NumericConstant::NumericConstant( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators(src_loc)
{
}

StringLiteral::StringLiteral( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators(src_loc)
{
	std::fill( type_suffix_.begin(), type_suffix_.end(), 0 );
}

BracketExpression::BracketExpression( const SrcLoc& src_loc)
	: ExpressionComponentWithUnaryOperators(src_loc)
{}

TypeNameInExpression::TypeNameInExpression( const SrcLoc& src_loc )
	: ExpressionComponentWithUnaryOperators( src_loc )
{}

Block::Block( const SrcLoc& start_src_loc )
	: SyntaxElementBase(start_src_loc)
{}

VariablesDeclaration::VariablesDeclaration( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

AutoVariableDeclaration::AutoVariableDeclaration( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

ReturnOperator::ReturnOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

WhileOperator::WhileOperator( const SrcLoc& src_loc)
	: SyntaxElementBase(src_loc)
	, block_(src_loc)
{}

ForOperator::ForOperator( const SrcLoc& src_loc)
	: SyntaxElementBase(src_loc)
	, block_(src_loc)
{}

CStyleForOperator::CStyleForOperator( const SrcLoc& src_loc)
	: SyntaxElementBase(src_loc)
	, block_(src_loc)
{}

BreakOperator::BreakOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

ContinueOperator::ContinueOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

WithOperator::WithOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
	, block_(src_loc)
{}

IfOperator::IfOperator( const SrcLoc& start_src_loc )
	: SyntaxElementBase(start_src_loc)
{}

StaticIfOperator::StaticIfOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
	, if_operator_(src_loc)
{}

SingleExpressionOperator::SingleExpressionOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

AssignmentOperator::AssignmentOperator(
	const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

AdditiveAssignmentOperator::AdditiveAssignmentOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

IncrementOperator::IncrementOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

DecrementOperator::DecrementOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

StaticAssert::StaticAssert( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

Halt::Halt( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

HaltIf::HaltIf( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

Typedef::Typedef( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

Enum::Enum( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

FunctionArgument::FunctionArgument( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

FunctionType::FunctionType( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

Function::Function( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
	, type_(src_loc)
{}

ClassField::ClassField( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

ClassVisibilityLabel::ClassVisibilityLabel( const SrcLoc& src_loc, ClassMemberVisibility visibility )
 : SyntaxElementBase(src_loc), visibility_(visibility)
{}

Class::Class( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

TemplateBase::TemplateBase( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

TypeTemplate::TypeTemplate( const SrcLoc& src_loc )
	: TemplateBase( src_loc )
{}

FunctionTemplate::FunctionTemplate( const SrcLoc& src_loc )
	: TemplateBase( src_loc )
{}

Namespace::Namespace( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

Import::Import( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

struct GetSrcLocVisitor final
{
	SrcLoc operator()( const EmptyVariant& ) const
	{
		return SrcLoc();
	}

	SrcLoc operator()( const SyntaxElementBase& element ) const
	{
		return element.src_loc_;
	}
};

SrcLoc GetExpressionSrcLoc( const Expression& expression )
{

	return std::visit( GetSrcLocVisitor(), expression );
}

SrcLoc GetInitializerSrcLoc( const Initializer& initializer )
{
	return std::visit( GetSrcLocVisitor(), initializer );
}

SrcLoc GetBlockElementSrcLoc( const BlockElement& block_element )
{
	return std::visit( GetSrcLocVisitor(), block_element );
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
