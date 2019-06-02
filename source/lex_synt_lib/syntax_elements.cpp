#include "assert.hpp"
#include "syntax_analyzer.hpp"

namespace U
{

namespace Synt
{

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
	// TODO - use visitor
	if( const auto array_initializer= boost::get<ArrayInitializer>( &initializer ) )
		return array_initializer->file_pos_;
	if( const auto struct_named_initializer= boost::get<StructNamedInitializer>( &initializer ) )
		return struct_named_initializer->file_pos_;
	if( const auto expression_initializer= boost::get<ExpressionInitializer>( &initializer ) )
		return expression_initializer->file_pos_;
	if( const auto zero_initializer= boost::get<ZeroInitializer>( &initializer ) )
		return zero_initializer->file_pos_;
	if( const auto uninitialized_initializer= boost::get<UninitializedInitializer>( &initializer ) )
		return uninitialized_initializer->file_pos_;
	return FilePos();
}

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

StringLiteral::StringLiteral( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

BracketExpression::BracketExpression( const FilePos& file_pos)
	: ExpressionComponentWithUnaryOperators(file_pos)
{}

TypeNameInExpression::TypeNameInExpression( const FilePos& file_pos )
	: ExpressionComponentWithUnaryOperators( file_pos )
{}

const FilePos& IBlockElement::GetFilePos() const
{
	// All non-abstract childs must be based on SyntaxElementBase.
	const SyntaxElementBase* const base= dynamic_cast<const SyntaxElementBase*>( this );
	U_ASSERT( base != nullptr );
	return base->file_pos_;
}

Block::Block( const FilePos& start_file_pos, const FilePos& end_file_pos, BlockElements elements )
	: SyntaxElementBase(start_file_pos)
	, end_file_pos_(end_file_pos)
	, elements_( std::move( elements ) )
{}

VariablesDeclaration::VariablesDeclaration( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

VariablesDeclaration::VariablesDeclaration( VariablesDeclaration&& other )
	: SyntaxElementBase(other.file_pos_)
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

AutoVariableDeclaration::AutoVariableDeclaration( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

ReturnOperator::ReturnOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

WhileOperator::WhileOperator( const FilePos& file_pos)
	: SyntaxElementBase(file_pos)
{}

BreakOperator::BreakOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

ContinueOperator::ContinueOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
{}

IfOperator::IfOperator( const FilePos& start_file_pos, const FilePos& end_file_pos, std::vector<Branch> branches )
	: SyntaxElementBase(start_file_pos)
	, branches_( std::move( branches ) )
	, end_file_pos_(end_file_pos)
{}

StaticIfOperator::StaticIfOperator( const FilePos& file_pos )
	: SyntaxElementBase(file_pos)
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

FunctionArgument::FunctionArgument(
	const FilePos& file_pos,
	ProgramString name,
	TypeName type,
	MutabilityModifier mutability_modifier,
	ReferenceModifier reference_modifier,
	ProgramString reference_tag,
	ReferencesTagsList inner_arg_reference_tags )
	: SyntaxElementBase( file_pos )
	, name_(std::move(name))
	, type_(std::move(type))
	, mutability_modifier_(mutability_modifier)
	, reference_modifier_(reference_modifier)
	, reference_tag_(std::move(reference_tag))
	, inner_arg_reference_tags_(std::move(inner_arg_reference_tags))
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

} // namespace Synt

} // namespace U
