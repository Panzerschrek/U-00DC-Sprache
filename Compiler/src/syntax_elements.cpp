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

CallOperator::CallOperator(
	const FilePos& file_pos,
	std::vector<IExpressionComponentPtr> arguments )
	: SyntaxElementBase(file_pos)
	, arguments_( std::move( arguments ) )
{}

IndexationOperator::IndexationOperator( const FilePos& file_pos, IExpressionComponentPtr index )
	: SyntaxElementBase(file_pos)
	,index_( std::move( index ) )
{
	U_ASSERT( index_ );
}

MemberAccessOperator::MemberAccessOperator(
	const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

const FilePos& IExpressionComponent::GetFilePos() const
{
	// All non-abstract childs must be based on SyntaxElementBase.
	const SyntaxElementBase* const base= dynamic_cast<const SyntaxElementBase*>( this );
	U_ASSERT( base != nullptr );
	return base->file_pos_;
}

const FilePos& IInitializer::GetFilePos() const
{
	// All non-abstract childs must be based on SyntaxElementBase.
	const SyntaxElementBase* const base= dynamic_cast<const SyntaxElementBase*>( this );
	U_ASSERT( base != nullptr );
	return base->file_pos_;
}

ArrayInitializer::ArrayInitializer( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

StructNamedInitializer::StructNamedInitializer( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
{}

ConstructorInitializer::ConstructorInitializer(
	const FilePos& file_pos,
	std::vector<IExpressionComponentPtr> arguments )
	: SyntaxElementBase( file_pos )
	, call_operator( file_pos, std::move(arguments) )
{}

ExpressionInitializer::ExpressionInitializer(
	const FilePos& file_pos , IExpressionComponentPtr in_expression )
	: SyntaxElementBase( file_pos )
	, expression(std::move(in_expression))
{}

ZeroInitializer::ZeroInitializer( const FilePos& file_pos )
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

MoveOperator::MoveOperator( const FilePos& file_pos  )
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

BracketExpression::BracketExpression( const FilePos& file_pos, IExpressionComponentPtr expression )
	: ExpressionComponentWithUnaryOperators(file_pos)
	, expression_( std::move( expression ) )
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

ReturnOperator::ReturnOperator( const FilePos& file_pos, IExpressionComponentPtr expression )
	: SyntaxElementBase(file_pos)
	, expression_( std::move( expression ) )
{}

WhileOperator::WhileOperator( const FilePos& file_pos, IExpressionComponentPtr condition, BlockPtr block )
	: SyntaxElementBase(file_pos)
	, condition_( std::move( condition ) )
	, block_( std::move( block ) )
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

SingleExpressionOperator::SingleExpressionOperator( const FilePos& file_pos, IExpressionComponentPtr expression )
	: SyntaxElementBase(file_pos)
	, expression_( std::move( expression ) )
{}

AssignmentOperator::AssignmentOperator(
	const FilePos& file_pos,
	IExpressionComponentPtr l_value,
	IExpressionComponentPtr r_value )
	: SyntaxElementBase(file_pos)
	, l_value_( std::move( l_value ) )
	, r_value_( std::move( r_value ) )
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
	ITypeNamePtr type,
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

Function::Function(
	const FilePos& file_pos,
	ComplexName name,
	ITypeNamePtr return_type,
	MutabilityModifier return_value_mutability_modifier,
	ReferenceModifier return_value_reference_modifier,
	ProgramString return_value_reference_tag,
	ReferencesTagsList return_value_inner_reference_tags,
	FunctionReferencesPollutionList referecnces_pollution_list,
	FunctionArgumentsDeclaration arguments,
	std::unique_ptr<StructNamedInitializer> constructor_initialization_list,
	BlockPtr block,
	OverloadedOperator overloaded_operator,
	VirtualFunctionKind virtual_function_kind)
	: SyntaxElementBase(file_pos)
	, name_( std::move(name) )
	, return_type_( std::move(return_type) )
	, return_value_mutability_modifier_(return_value_mutability_modifier)
	, return_value_reference_modifier_(return_value_reference_modifier)
	, return_value_reference_tag_(std::move(return_value_reference_tag))
	, return_value_inner_reference_tags_(std::move(return_value_inner_reference_tags))
	, referecnces_pollution_list_( std::move(referecnces_pollution_list) )
	, arguments_( std::move(arguments) )
	, constructor_initialization_list_( std::move(constructor_initialization_list) )
	, block_( std::move(block) )
	, overloaded_operator_( std::move(overloaded_operator) )
	, virtual_function_kind_( std::move(virtual_function_kind) )
{}

ClassField::ClassField( const FilePos& file_pos )
	: SyntaxElementBase( file_pos )
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
