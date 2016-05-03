#include "assert.hpp"
#include "syntax_analyzer.hpp"
#include <iostream>
namespace Interpreter
{

namespace Keywords
{

const ProgramString fn= ToProgramString( "fn" );
const ProgramString let= ToProgramString( "let" );
const ProgramString return_= ToProgramString( "return" );
const ProgramString while_= ToProgramString( "while" );
const ProgramString break_= ToProgramString( "break" );
const ProgramString continue_= ToProgramString( "continue" );
const ProgramString if_= ToProgramString( "if" );
const ProgramString else_= ToProgramString( "else" );

}

// Prototypes.
static BlockPtr ParseBlock(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end );

static void PushErrorMessage(
	SyntaxErrorMessages& error_messages,
	const Lexem& lexem )
{
	error_messages.emplace_back(
		std::to_string(lexem.line) + ":" + std::to_string(lexem.pos_in_line) +
		" Syntax error - unexpected lexem: " + ToStdString(lexem.text) );
}

static bool IsBinaryOperator( const Lexem& lexem )
{
	return
		lexem.type == Lexem::Type::Plus ||
		lexem.type == Lexem::Type::Minus ||
		lexem.type == Lexem::Type::Star ||
		lexem.type == Lexem::Type::Slash;
}

static BinaryOperator LexemToBinaryOperator( const Lexem& lexem )
{
	switch( lexem.type )
	{
		case Lexem::Type::Plus: return BinaryOperator::Add;
		case Lexem::Type::Minus: return BinaryOperator::Sub;
		case Lexem::Type::Star: return BinaryOperator::Mul;
		case Lexem::Type::Slash: return BinaryOperator::Div;

		default:
		U_ASSERT(false);
		return BinaryOperator::None;
	};
}

static BinaryOperatorsChainPtr ParseExpression(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	BinaryOperatorsChainPtr result( new BinaryOperatorsChain );

	while(1)
	{
		PrefixOperators prefix_operators;

		// Prefix operators.
		while(1)
		{
			switch( it->type )
			{
			case Lexem::Type::Identifier:
			case Lexem::Type::Number:
			case Lexem::Type::BracketLeft:
				goto parse_operand;

			case Lexem::Type::Plus:
				 prefix_operators.emplace_back( new UnaryPlus() );
				++it;
				break;

			case Lexem::Type::Minus:
				prefix_operators.emplace_back( new UnaryMinus() );
				++it;
				break;

			default:
				if( prefix_operators.empty() )
					goto return_result;
				else
				{
					PushErrorMessage( error_messages, *it );
					return nullptr;
				}
			};
		}

	parse_operand:
		result->components.emplace_back();
		BinaryOperatorsChain::ComponentWithOperator& component= result->components.back();

		component.prefix_operators= std::move( prefix_operators );

		// Operand.
		if( it->type == Lexem::Type::Identifier )
		{
			component.component.reset( new NamedOperand( it->text ) );
			++it;
		}
		else if( it->type == Lexem::Type::Number )
		{
			component.component.reset( new NumericConstant( it->text ) );
			++it;
		}
		else if( it->type == Lexem::Type::BracketLeft )
		{
			++it;

			component.component.reset(
				new BracketExpression(
					ParseExpression(
						error_messages,
						it,
						it_end ) ) );

			U_ASSERT( it < it_end );
			if( it->type != Lexem::Type::BracketRight )
			{
				PushErrorMessage( error_messages, *it );
				return nullptr;
			}
			++it;
		}
		else
		{
			U_ASSERT(false);
		}

		// Postfix operators.
		while(1)
		{
			switch( it->type )
			{
			case Lexem::Type::SquareBracketLeft:
				{
					++it;
					U_ASSERT( it < it_end );

					component.postfix_operators.emplace_back(
						new IndexationOperator(
							ParseExpression(
								error_messages,
								it,
								it_end ) ) );

					U_ASSERT( it < it_end );
					if( it->type != Lexem::Type::SquareBracketRight )
					{
						PushErrorMessage( error_messages, *it );
						return nullptr;
					}
					++it;
				}
				break;

			case Lexem::Type::BracketLeft:
				{
					++it;
					U_ASSERT( it < it_end );

					std::vector<BinaryOperatorsChainPtr> arguments;
					while(1)
					{
						if( it->type == Lexem::Type::BracketRight )
						{
							++it;
							break;
						}

						arguments.emplace_back(
							ParseExpression(
								error_messages,
								it,
								it_end ) );

						if( it->type == Lexem::Type::Comma )
						{
							++it;
							if( it->type == Lexem::Type::BracketRight )
							{
								PushErrorMessage( error_messages, *it );
								return nullptr;
							}
						}
						else if( it->type == Lexem::Type::BracketRight )
						{
							++it;
							break;
						}
					}

					component.postfix_operators.emplace_back( new CallOperator( std::move( arguments ) ) );

				} break;

				default:
				if( IsBinaryOperator( *it ) )
					goto parse_binary_operator;
				else // End of postfix operators.
					goto return_result;
			};
		}

	parse_binary_operator:;
		component.op= LexemToBinaryOperator( *it );

		++it;
		U_ASSERT( it < it_end );
	}

return_result:
	if( result->components.empty() )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}
	if( result->components.back().op != BinaryOperator::None )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	return result;
}

static VariableDeclarationPtr ParseVariableDeclaration(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::let );
	U_ASSERT( it < it_end );

	++it;
	U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::Identifier )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	VariableDeclarationPtr decl( new VariableDeclaration() );
	decl->name= it->text;

	++it;
	U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::Colon )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;
	U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::Identifier )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	decl->type= it->text;

	++it;
	U_ASSERT( it < it_end );


	if( it->type == Lexem::Type::Assignment )
	{
		++it;
		decl->initial_value= ParseExpression( error_messages, it, it_end );
	}

	if( it->type == Lexem::Type::Semicolon )
		++it;
	else
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	return decl;
}

static IBlockElementPtr ParseReturnOperator(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::return_ );
	U_ASSERT( it < it_end );

	++it;
	U_ASSERT( it < it_end );

	if( it->type == Lexem::Type::Semicolon )
	{
		++it;
		return IBlockElementPtr( new ReturnOperator( nullptr ) );
	}

	BinaryOperatorsChainPtr expression=
		ParseExpression(
			error_messages,
			it,
			it_end );

	U_ASSERT( it < it_end );
	if( it->type != Lexem::Type::Semicolon  )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;

	return IBlockElementPtr( new ReturnOperator( std::move( expression ) ) );
}

static IBlockElementPtr ParseWhileOperator(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::while_ );
	U_ASSERT( it < it_end );

	++it;
	U_ASSERT( it < it_end );
	if( it->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;
	U_ASSERT( it < it_end );

	BinaryOperatorsChainPtr condition= ParseExpression( error_messages, it, it_end );

	if( it->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;
	U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	BlockPtr block= ParseBlock( error_messages, it, it_end );

	return
		IBlockElementPtr(
			new WhileOperator(
				std::move( condition ),
				std::move( block ) ) );
}

static IBlockElementPtr ParseBreakOperator(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::break_ );
	U_ASSERT( it < it_end );

	++it;
	U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;

	return IBlockElementPtr( new BreakOperator() );
}

static IBlockElementPtr ParseContinueOperator(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::continue_ );
	U_ASSERT( it < it_end );

	++it;
	U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;

	return IBlockElementPtr( new ContinueOperator() );
}

static IBlockElementPtr ParseIfOperaotr(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::if_ );
	U_ASSERT( it < it_end );

	++it;
	U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;
	U_ASSERT( it < it_end );

	std::vector<IfOperator::Branch> branches;
	branches.emplace_back();

	branches.back().condition= ParseExpression( error_messages, it, it_end );

	if( it->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;
	U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}
	branches.back().block= ParseBlock( error_messages, it, it_end );

	while(1)
	{
		if( it->type == Lexem::Type::Identifier && it->text == Keywords::else_ )
		{
			branches.emplace_back();

			++it;
			U_ASSERT( it < it_end );

			// Optional if.
			if( it->type == Lexem::Type::Identifier && it->text == Keywords::if_ )
			{
				++it;
				U_ASSERT( it < it_end );

				if( it->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage( error_messages, *it );
					return nullptr;
				}

				++it;
				U_ASSERT( it < it_end );

				branches.back().condition= ParseExpression( error_messages, it, it_end );

				if( it->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage( error_messages, *it );
					return nullptr;
				}

				++it;
				U_ASSERT( it < it_end );
			}
			// Block - common for "else" and "else if".
			if( it->type != Lexem::Type::BraceLeft )
			{
				PushErrorMessage( error_messages, *it );
				return nullptr;
			}

			branches.back().block= ParseBlock( error_messages, it, it_end );

			if( branches.back().condition == nullptr ) break;

		}
		else
			break;
	}

	return
		IBlockElementPtr(
			new IfOperator(
				std::move( branches ) ) );
}

static BlockPtr ParseBlock(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::BraceLeft );
	U_ASSERT( it < it_end );

	++it;

	BlockElements elements;

	while( it->type != Lexem::Type::EndOfFile )
	{
		if( it->type == Lexem::Type::BraceLeft )
			elements.emplace_back( ParseBlock( error_messages, it, it_end ) );

		else if( it->type == Lexem::Type::BraceRight )
			break;

		else if( it->type == Lexem::Type::Identifier && it->text == Keywords::let )
			elements.emplace_back( ParseVariableDeclaration( error_messages, it, it_end ) );

		else if( it->type == Lexem::Type::Identifier && it->text == Keywords::return_ )
			elements.emplace_back( ParseReturnOperator( error_messages, it, it_end ) );

		else if( it->type == Lexem::Type::Identifier && it->text == Keywords::while_ )
			elements.emplace_back( ParseWhileOperator( error_messages, it, it_end ) );

		else if( it->type == Lexem::Type::Identifier && it->text == Keywords::break_ )
			elements.emplace_back( ParseBreakOperator( error_messages, it, it_end ) );

		else if( it->type == Lexem::Type::Identifier && it->text == Keywords::continue_ )
			elements.emplace_back( ParseContinueOperator( error_messages, it, it_end ) );

		else if( it->type == Lexem::Type::Identifier && it->text == Keywords::if_ )
			elements.emplace_back( ParseIfOperaotr( error_messages, it, it_end ) );


		else
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}
	}

	if( it->type == Lexem::Type::BraceRight )
		++it;
	else
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	return BlockPtr(
		new Block(
			std::move( elements ) ) );
}

static IProgramElementPtr ParseFunction(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->text == Keywords::fn );
	U_ASSERT( it < it_end );

	++it;
	U_ASSERT( it < it_end );
	if( it->type != Lexem::Type::Identifier )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	const ProgramString& fn_name= it->text;

	++it;
	U_ASSERT( it < it_end );
	if( it->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;
	U_ASSERT( it < it_end );

	std::vector<VariableDeclaration> arguments;

	while(1)
	{
		if( it->type == Lexem::Type::BracketRight )
		{
			++it;
			break;
		}

		if( it->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}
		VariableDeclaration decl;
		decl.name= it->text;

		++it;
		U_ASSERT( it < it_end );
		if( it->type != Lexem::Type::Colon )
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}

		++it;
		U_ASSERT( it < it_end );
		if( it->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}

		decl.type= it->text;

		arguments.emplace_back( std::move( decl ) );

		++it;
		U_ASSERT( it < it_end );

		if( it->type == Lexem::Type::Comma )
		{
			++it;
			// Disallov constructions, like "fn f( a : int, ){}"
			if( it->type == Lexem::Type::BracketRight )
			{
				PushErrorMessage( error_messages, *it );
				return nullptr;
			}
		}
		else if( it->type == Lexem::Type::BracketRight )
		{}
		else
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}
	}

	ProgramString return_type;

	if( it->type == Lexem::Type::Colon )
	{
		++it;
		U_ASSERT( it < it_end );

		if( it->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}

		return_type= it->text;
		++it;
	}

	BlockPtr block;

	if( it < it_end && it->type == Lexem::Type::BraceLeft )
		block= ParseBlock( error_messages, it, it_end );
	else
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	return IProgramElementPtr(
		new FunctionDeclaration(
			std::move( fn_name ),
			return_type,
			std::move( arguments ),
			std::move( block ) ) );
}

SyntaxAnalysisResult SyntaxAnalysis( const Lexems& lexems )
{
	SyntaxAnalysisResult result;

	Lexems::const_iterator it= lexems.begin();
	const Lexems::const_iterator it_end= lexems.end();

	Lexems::const_iterator unexpected_lexem= it_end;

	while( it < it_end )
	{
		const Lexem& lexem= *it;

		const Lexems::const_iterator prev_it= it;

		if( lexem.type == Lexem::Type::Identifier && lexem.text == Keywords::fn )
		{
			if( IProgramElementPtr program_element= ParseFunction( result.error_messages, it, it_end ) )
				result.program_elements.emplace_back( std::move( program_element ) );

			continue;
		}
		else if( lexem.type == Lexem::Type::EndOfFile )
			break;

		if( unexpected_lexem == it_end )
			unexpected_lexem= it;
		else
		{
			// Iterator changed => end of unexpected lexems block;
			if( prev_it < it || it + 1 == it_end )
			{
				PushErrorMessage( result.error_messages, *unexpected_lexem );
				unexpected_lexem= it_end;
			}
		}
		++it;
	}

	return result;
}

} // namespace Interpreter
