#include <cctype>
#include <cmath>

#include "assert.hpp"
#include "keywords.hpp"
#include "syntax_analyzer.hpp"

namespace Interpreter
{

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
		lexem.type == Lexem::Type::Slash ||

		lexem.type == Lexem::Type::CompareEqual ||
		lexem.type == Lexem::Type::CompareNotEqual ||
		lexem.type == Lexem::Type::CompareLess ||
		lexem.type == Lexem::Type::CompareLessOrEqual ||
		lexem.type == Lexem::Type::CompareGreater ||
		lexem.type == Lexem::Type::CompareGreaterOrEqual;
}

static BinaryOperator LexemToBinaryOperator( const Lexem& lexem )
{
	switch( lexem.type )
	{
		case Lexem::Type::Plus: return BinaryOperator::Add;
		case Lexem::Type::Minus: return BinaryOperator::Sub;
		case Lexem::Type::Star: return BinaryOperator::Mul;
		case Lexem::Type::Slash: return BinaryOperator::Div;

		case Lexem::Type::CompareEqual: return BinaryOperator::Equal;
		case Lexem::Type::CompareNotEqual: return BinaryOperator::NotEqual;
		case Lexem::Type::CompareLess: return BinaryOperator::Less;
		case Lexem::Type::CompareLessOrEqual: return BinaryOperator::LessEqual;
		case Lexem::Type::CompareGreater: return BinaryOperator::Greater;
		case Lexem::Type::CompareGreaterOrEqual: return BinaryOperator::GreaterEqual;

		default:
		U_ASSERT(false);
		return BinaryOperator::None;
	};
}

static IBinaryOperatorsChainComponentPtr ParseNumericConstant(
	const ProgramString& text )
{
	NumericConstant::LongFloat base= 10;
	unsigned int (*number_func)( sprache_char) =
		[]( sprache_char c ) -> unsigned int
		{
			U_ASSERT( c >= '0' && c <= '9' );
			return c - '0';
		};

	bool (*is_number_func)( sprache_char) =
		[]( sprache_char c ) -> bool { return std::isdigit(c); };

	ProgramString::const_iterator it= text.begin();
	const ProgramString::const_iterator it_end= text.end();

	if( text.size() >= 2 && text[0] == '0' )
	{
		sprache_char d= text[1];
		switch(d)
		{
		case 'b':
			it+= 2;
			base= 2;
			number_func=
				[]( sprache_char c ) -> unsigned int
				{
					U_ASSERT( c >= '0' && c <= '1' );
					return c - '0';
				};
			is_number_func=
				[]( sprache_char c ) -> bool
				{
					return c == '0' || c == '1';
				};
			break;

		case 'o':
			it+= 2;
			base= 8;
			number_func=
				[]( sprache_char c ) -> unsigned int
				{
					U_ASSERT( c >= '0' && c <= '7' );
					return c - '0';
				};
			is_number_func=
				[]( sprache_char c ) -> bool
				{
					return c >= '0' && c <= '9';
				};
			break;

		case 'x':
			it+= 2;
			base= 16;
			number_func=
				[]( sprache_char c ) -> unsigned int
				{
					if( c >= '0' && c <= '9' )
						return c - '0';
					else if( c >= 'a' && c <= 'f' )
						return c - 'a';
					else
					{
						U_ASSERT( c >= 'A' && c <= 'F' );
						return c - 'A';
					}
				};
			is_number_func= []( sprache_char c ) -> bool { return std::isxdigit(c); };
			break;

		default: break;
		};
	}

	NumericConstant::LongFloat number{ 0 };
	bool has_fraction_point= false;

	ProgramString::const_iterator integer_part_end= it;
	while( integer_part_end < it_end &&
		is_number_func( *integer_part_end ) )
		++integer_part_end;

	{
		NumericConstant::LongFloat pow{ 1 };
		for( auto n= integer_part_end - 1; n >= it; --n, pow*= base )
			number+= number_func(*n) * pow;
	}

	it= integer_part_end;
	if( it < it_end && *it == '.' )
	{
		++it;
		has_fraction_point= true;

		NumericConstant::LongFloat fractional_part{ 0 };
		NumericConstant::LongFloat pow{ 1 / base };

		while( it < it_end && is_number_func(*it) )
		{
			fractional_part+= number_func( *it ) * pow;
			pow/= base;
			++it;
		}

		number+= fractional_part;
	}

	// Exponent
	if( it < it_end && *it == 'e' )
	{
		++it;

		U_ASSERT( base == 10 );
		int power= 0;
		bool is_negative= false;

		if( it < it_end && *it == '-' )
		{
			is_negative= true;
			++it;
		}

		while( it < it_end && is_number_func(*it) )
		{
			power*= base;
			power+= number_func(*it);
			++it;
		}
		if( is_negative ) power= -power;

		number*= std::pow<NumericConstant::LongFloat>( base, power );
	}

	ProgramString type_suffix( it, it_end );

	return
		IBinaryOperatorsChainComponentPtr(
			new NumericConstant(
				number,
				std::move( type_suffix ),
				has_fraction_point ) );
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
			component.component= ParseNumericConstant( it->text );
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
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::let_ );
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
	U_UNUSED( it_end );

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
	U_UNUSED( it_end );
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

		else if( it->type == Lexem::Type::Identifier && it->text == Keywords::let_ )
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
			BinaryOperatorsChainPtr l_expression= ParseExpression( error_messages, it, it_end );

			U_ASSERT( it < it_end );
			if( it->type == Lexem::Type::Assignment )
			{
				++it;
				U_ASSERT( it < it_end );

				BinaryOperatorsChainPtr r_expression= ParseExpression( error_messages, it, it_end );

				if( it->type != Lexem::Type::Semicolon )
				{
					PushErrorMessage( error_messages, *it );
					return nullptr;
				}
				++it;
				U_ASSERT( it < it_end );

				elements.emplace_back(
					new AssignmentOperator(
						std::move( l_expression ),
						std::move( r_expression ) ) );
			}
			else if( it->type == Lexem::Type::Semicolon )
			{
				++it;
				U_ASSERT( it < it_end );

				elements.emplace_back(
					new SingleExpressionOperator(
						std::move( l_expression ) ) );
			}
			else
			{
				PushErrorMessage( error_messages, *it );
				return nullptr;
			}
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
	U_ASSERT( it->text == Keywords::fn_ );
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

		if( lexem.type == Lexem::Type::Identifier && lexem.text == Keywords::fn_ )
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
