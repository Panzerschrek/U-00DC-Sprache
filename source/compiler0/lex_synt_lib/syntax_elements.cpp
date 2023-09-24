#include "../../lex_synt_lib_common/assert.hpp"
#include "syntax_analyzer.hpp"
#include "../../lex_synt_lib_common/size_assert.hpp"

namespace U
{

namespace Synt
{

// Sizes for x86-64.
// If one of types inside variant becomes too big, put it inside "unique_ptr".
SIZE_ASSERT( ComplexName, 56u )
SIZE_ASSERT( TypeName, 64u )
SIZE_ASSERT( Expression, 88u )
SIZE_ASSERT( Initializer, 96u )
SIZE_ASSERT( BlockElement, 16u ) // Variant index + unique_ptr
SIZE_ASSERT( ClassElement, 144u )
SIZE_ASSERT( ProgramElement, 144u )

TypeofTypeName::TypeofTypeName( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

RootNamespaceNameLookup::RootNamespaceNameLookup( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

RootNamespaceNameLookupCompletion::RootNamespaceNameLookupCompletion( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

NameLookup::NameLookup( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

NameLookupCompletion::NameLookupCompletion( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

NamesScopeNameFetch::NamesScopeNameFetch( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

NamesScopeNameFetchCompletion::NamesScopeNameFetchCompletion( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

TemplateParametrization::TemplateParametrization( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

ArrayTypeName::ArrayTypeName( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

TupleType::TupleType( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

RawPointerType::RawPointerType( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

GeneratorType::GeneratorType( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

UnaryPlus::UnaryPlus( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

UnaryMinus::UnaryMinus( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

LogicalNot::LogicalNot( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

BitwiseNot::BitwiseNot( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

CallOperator::CallOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

CallOperatorSignatureHelp::CallOperatorSignatureHelp( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

IndexationOperator::IndexationOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

MemberAccessOperator::MemberAccessOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

MemberAccessOperatorCompletion::MemberAccessOperatorCompletion( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

SequenceInitializer::SequenceInitializer( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

StructNamedInitializer::StructNamedInitializer( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

ConstructorInitializer::ConstructorInitializer( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

ConstructorInitializerSignatureHelp::ConstructorInitializerSignatureHelp( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

ZeroInitializer::ZeroInitializer( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

UninitializedInitializer::UninitializedInitializer( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

BinaryOperator::BinaryOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

TernaryOperator::TernaryOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

ReferenceToRawPointerOperator::ReferenceToRawPointerOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

RawPointerToReferenceOperator::RawPointerToReferenceOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

MoveOperator::MoveOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

TakeOperator::TakeOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

CastRef::CastRef( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

CastRefUnsafe::CastRefUnsafe( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

CastImut::CastImut( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

CastMut::CastMut( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

TypeInfo::TypeInfo( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

SameType::SameType( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

NonSyncExpression::NonSyncExpression( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

SafeExpression::SafeExpression( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

UnsafeExpression::UnsafeExpression( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

BooleanConstant::BooleanConstant( const SrcLoc& src_loc, bool value )
	: src_loc(src_loc)
	, value( value )
{}

NumericConstant::NumericConstant( const SrcLoc& src_loc )
	: src_loc(src_loc)
{
}

StringLiteral::StringLiteral( const SrcLoc& src_loc )
	: src_loc(src_loc)
{
	std::fill( type_suffix.begin(), type_suffix.end(), 0 );
}

Block::Block( const SrcLoc& start_src_loc )
	: src_loc(start_src_loc)
{}

ScopeBlock::ScopeBlock( Block block )
	: Block( std::move(block) )
{}

VariablesDeclaration::VariablesDeclaration( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

AutoVariableDeclaration::AutoVariableDeclaration( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

ReturnOperator::ReturnOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

YieldOperator::YieldOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

Label::Label( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

WhileOperator::WhileOperator( const SrcLoc& src_loc )
	: src_loc(src_loc), block(src_loc)
{}

LoopOperator::LoopOperator( const SrcLoc& src_loc )
	: src_loc(src_loc), block(src_loc)
{}

RangeForOperator::RangeForOperator( const SrcLoc& src_loc )
	: src_loc(src_loc), block(src_loc)
{}

CStyleForOperator::CStyleForOperator( const SrcLoc& src_loc )
	: src_loc(src_loc), block(src_loc)
{}

BreakOperator::BreakOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

ContinueOperator::ContinueOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

WithOperator::WithOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
	, block(src_loc)
{}

IfOperator::IfOperator( const SrcLoc& start_src_loc )
	: src_loc(start_src_loc)
	, block( start_src_loc )
{}

StaticIfOperator::StaticIfOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
	, block( src_loc )
{}

IfCoroAdvanceOperator::IfCoroAdvanceOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
	, block(src_loc)
{}

SwitchOperator::SwitchOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

SingleExpressionOperator::SingleExpressionOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

AssignmentOperator::AssignmentOperator(
	const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

AdditiveAssignmentOperator::AdditiveAssignmentOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

IncrementOperator::IncrementOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

DecrementOperator::DecrementOperator( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

StaticAssert::StaticAssert( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

Halt::Halt( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

HaltIf::HaltIf( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

TypeAlias::TypeAlias( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

Enum::Enum( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

FunctionParam::FunctionParam( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

FunctionType::FunctionType( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

Function::Function( const SrcLoc& src_loc )
	: src_loc(src_loc)
	, type(src_loc)
{}

ClassField::ClassField( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

ClassVisibilityLabel::ClassVisibilityLabel( const SrcLoc& src_loc, ClassMemberVisibility visibility )
	: src_loc(src_loc), visibility(visibility)
{}

Class::Class( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

TemplateBase::TemplateBase( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

TypeTemplate::TypeTemplate( const SrcLoc& src_loc )
	: TemplateBase( src_loc )
{}

FunctionTemplate::FunctionTemplate( const SrcLoc& src_loc )
	: TemplateBase( src_loc )
{}

Namespace::Namespace( const SrcLoc& src_loc )
	: src_loc(src_loc)
{}

Import::Import( const SrcLoc& src_loc )
	: src_loc(src_loc)
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

	template<typename T>
	SrcLoc operator()( const T& element ) const
	{
		return element.src_loc;
	}

	template<typename T>
	SrcLoc operator()( const std::unique_ptr<T>& element ) const
	{
		return (*this)(*element);
	}

	template<typename T>
	SrcLoc operator()( const BlockElementsListNodePtr<T>& element ) const
	{
		return element->payload.src_loc;
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
