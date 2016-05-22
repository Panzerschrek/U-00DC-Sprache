#include "assert.hpp"
#include "inverse_polish_notation.hpp"

namespace Interpreter
{

static const unsigned int g_operators_priority[ size_t(BinaryOperator::Last) ]=
{
	[ size_t( BinaryOperator::None ) ]= 0,

	[ size_t( BinaryOperator::Add ) ]= 2,
	[ size_t( BinaryOperator::Sub ) ]= 2,

	[ size_t( BinaryOperator::Div ) ]= 3,
	[ size_t( BinaryOperator::Mul ) ]= 3,

	[ size_t( BinaryOperator::Equal ) ]= 1,
	[ size_t( BinaryOperator::NotEqual ) ]= 1,
	[ size_t( BinaryOperator::Less ) ]= 1,
	[ size_t( BinaryOperator::LessEqual ) ]= 1,
	[ size_t( BinaryOperator::Greater ) ]= 1,
	[ size_t( BinaryOperator::GreaterEqual ) ]= 1,
};

static inline unsigned int GetOperatorPriority( BinaryOperator op )
{
	return g_operators_priority[ size_t(op) ];
}

void InversePolishNotationComponent::Print( std::ostream& stream ) const
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
			InversePolishNotationComponent operand;
			operand.operand= component_with_operator.component->Clone();

			operand.prefix_operand_operators.reserve( component_with_operator.prefix_operators.size() );
			for( const IUnaryPrefixOperatorPtr& op : component_with_operator.prefix_operators )
				operand.prefix_operand_operators.emplace_back( op->Clone() );

			operand.postfix_operand_operators.reserve( component_with_operator.postfix_operators.size() );
			for( const IUnaryPostfixOperatorPtr& op : component_with_operator.postfix_operators )
				operand.postfix_operand_operators.emplace_back( op->Clone() );

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

				if( op_priority > stack_top_priority )
					operators_stack.push_back( component_with_operator.op );

				else
				{
					while( op_priority<= stack_top_priority )
					{
						InversePolishNotationComponent operator_;
						operator_.operator_= operators_stack.back();
						result.emplace_back( std::move(operator_) );

						operators_stack.pop_back();
						if( operators_stack.empty() ) break;

						stack_top_priority= GetOperatorPriority( operators_stack.back() );
					}
					operators_stack.push_back( component_with_operator.op );
				}
			}
		}

	} // for binary expression chain

	// Clear operators stack
	for( auto it= operators_stack.rbegin(); it != operators_stack.rend(); ++it )
	{
		InversePolishNotationComponent operator_;
		operator_.operator_= *it;
		result.emplace_back( std::move(operator_) );
	}

	return result;
}

void PrintInversePolishNotation( std::ostream& stream, const InversePolishNotation& ipn )
{
	for( const InversePolishNotationComponent& component : ipn )
	{
		component.Print( stream );
		stream << " ";
	}
}

} // namespace Interpreter
