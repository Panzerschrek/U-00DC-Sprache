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
static_assert( sizeof(ComplexName) <= 56u, "Size of variant too big" );
static_assert( sizeof(TypeName) <= 64u, "Size of variant too big" );
static_assert( sizeof(Expression) <= 88u, "Size of variant too big" );
static_assert( sizeof(Initializer) <= 96u, "Size of variant too big" );
static_assert( sizeof(BlockElement) <= 200u, "Size of variant too big" );
static_assert( sizeof(ClassElement) <= 144u, "Size of variant too big" );
static_assert( sizeof(ProgramElement) <= 144u, "Size of variant too big" );

}

SyntaxElementBase::SyntaxElementBase( const SrcLoc& src_loc )
	: src_loc_(src_loc)
{}

TypeofTypeName::TypeofTypeName( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

RootNamespaceLookup::RootNamespaceLookup( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

NameLookup::NameLookup( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

NamesScopeNameFetch::NamesScopeNameFetch( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

TemplateParametrization::TemplateParametrization( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
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

GeneratorType::GeneratorType( const SrcLoc& src_loc )
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

SequenceInitializer::SequenceInitializer( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

StructNamedInitializer::StructNamedInitializer( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

ConstructorInitializer::ConstructorInitializer( const SrcLoc& src_loc )
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

TernaryOperator::TernaryOperator( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

ReferenceToRawPointerOperator::ReferenceToRawPointerOperator( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

RawPointerToReferenceOperator::RawPointerToReferenceOperator( const SrcLoc& src_loc )
	: SyntaxElementBase( src_loc )
{}

MoveOperator::MoveOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

TakeOperator::TakeOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

CastRef::CastRef( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

CastRefUnsafe::CastRefUnsafe( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

CastImut::CastImut( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

CastMut::CastMut( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

TypeInfo::TypeInfo( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

NonSyncExpression::NonSyncExpression( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

SafeExpression::SafeExpression( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

UnsafeExpression::UnsafeExpression( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

BooleanConstant::BooleanConstant( const SrcLoc& src_loc, bool value )
	: SyntaxElementBase(src_loc)
	, value_( value )
{}

NumericConstant::NumericConstant( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{
}

StringLiteral::StringLiteral( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{
	std::fill( type_suffix_.begin(), type_suffix_.end(), 0 );
}

Block::Block( const SrcLoc& start_src_loc )
	: SyntaxElementBase(start_src_loc)
{}

ScopeBlock::ScopeBlock( Block block )
	: Block( std::move(block) )
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

YieldOperator::YieldOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

Label::Label( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

WhileOperator::WhileOperator( const SrcLoc& src_loc)
	: SyntaxElementBase(src_loc)
{}

LoopOperator::LoopOperator( const SrcLoc& src_loc)
	: SyntaxElementBase(src_loc)
{}

RangeForOperator::RangeForOperator( const SrcLoc& src_loc)
	: SyntaxElementBase(src_loc)
{}

CStyleForOperator::CStyleForOperator( const SrcLoc& src_loc)
	: SyntaxElementBase(src_loc)
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
	, block( start_src_loc )
{}

StaticIfOperator::StaticIfOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
	, block( src_loc )
{}

IfCoroAdvanceOperator::IfCoroAdvanceOperator( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
	, block(src_loc)
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

TypeAlias::TypeAlias( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

Enum::Enum( const SrcLoc& src_loc )
	: SyntaxElementBase(src_loc)
{}

FunctionParam::FunctionParam( const SrcLoc& src_loc )
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

	SrcLoc operator()( const Expression& expression ) const
	{
		return GetExpressionSrcLoc(expression);
	}

	SrcLoc operator()( const ComplexName& complex_name ) const
	{
		return GetComplexNameSrcLoc(complex_name);
	}

	SrcLoc operator()( const SyntaxElementBase& element ) const
	{
		return element.src_loc_;
	}

	template<class T>
	SrcLoc operator()( const std::unique_ptr<T>& element ) const
	{
		return (*this)(*element);
	}
};

SrcLoc GetExpressionSrcLoc( const Expression& expression )
{
	return std::visit( GetSrcLocVisitor(), expression );
}

SrcLoc GetComplexNameSrcLoc( const ComplexName& complex_name )
{
	return std::visit( GetSrcLocVisitor(), complex_name );
}

SrcLoc GetInitializerSrcLoc( const Initializer& initializer )
{
	return std::visit( GetSrcLocVisitor(), initializer );
}

SrcLoc GetBlockElementSrcLoc( const BlockElement& block_element )
{
	return std::visit( GetSrcLocVisitor(), block_element );
}

} // namespace Synt

} // namespace U
