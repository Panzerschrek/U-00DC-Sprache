#include "assert.hpp"
#include "inverse_polish_notation.hpp"

namespace Interpreter
{

static const unsigned int g_operators_priority[ size_t(BinaryOperator::Last) ]=
{
	[ size_t( BinaryOperator::None ) ]= 0,

	[ size_t( BinaryOperator::Add ) ]= 1,
	[ size_t( BinaryOperator::Sub ) ]= 1,

	[ size_t( BinaryOperator::Div ) ]= 2,
	[ size_t( BinaryOperator::Mul ) ]= 2,
};

static inline unsigned int GetOperatorPriority( BinaryOperator op )
{
	return g_operators_priority[ size_t(op) ];
}

void ReversePolishNotationComponent::Print( std::ostream& stream )
{
	if( operand )
		operand->Print( stream, 0 );
	else
		PrintOperator( stream, operator_ );
}

InversePolishNotation ConvertToInversePolishNotation(
	const BinaryOperatorsChain& binary_operators_chain )
{
	InversePolishNotation result;

	std::vector<BinaryOperator> operators_stack;

	for( const BinaryOperatorsChain::ComponentWithOperator& component_with_operator :
		binary_operators_chain.components )
	{
		{
			ReversePolishNotationComponent operand;
			operand.operand= component_with_operator.component->Clone();

			result.emplace_back( std::move(operand) );
		}

		if( component_with_operator.op == BinaryOperator::None )
		{
			U_ASSERT( &component_with_operator == &binary_operators_chain.components.back() );
		}
		else
		{
			if( operators_stack.empty() )
				operators_stack.push_back( component_with_operator.op );
			else
			{
				unsigned int op_priority= GetOperatorPriority( component_with_operator.op );
				unsigned int stack_top_priority= GetOperatorPriority( operators_stack.back() );

				// Pop from stack to notation.
				if( stack_top_priority > op_priority )
				{
					ReversePolishNotationComponent operator_;
					operator_.operator_= operators_stack.back();
					result.emplace_back( std::move(operator_) );

					operators_stack.pop_back();
				}
				else
					operators_stack.push_back( component_with_operator.op );
			}
		}

	} // for binary expression chain

	// Clear operators stack
	for( auto it= operators_stack.rbegin(); it != operators_stack.rend(); ++it )
	{
		ReversePolishNotationComponent operator_;
		operator_.operator_= *it;
		result.emplace_back( std::move(operator_) );
	}

	return result;
}

} // namespace Interpreter
