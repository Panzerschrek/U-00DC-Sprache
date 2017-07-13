#include <cctype>
#include <cmath>

#include "assert.hpp"
#include "keywords.hpp"
#include "syntax_analyzer.hpp"

namespace U
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
		std::to_string(lexem.file_pos.line) + ":" + std::to_string(lexem.file_pos.pos_in_line) +
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
		lexem.type == Lexem::Type::CompareGreaterOrEqual ||

		lexem.type == Lexem::Type::And ||
		lexem.type == Lexem::Type::Or ||
		lexem.type == Lexem::Type::Xor ||

		lexem.type == Lexem::Type::Conjunction ||
		lexem.type == Lexem::Type::Disjunction;
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

		case Lexem::Type::And: return BinaryOperator::And;
		case Lexem::Type::Or: return BinaryOperator::Or;
		case Lexem::Type::Xor: return BinaryOperator::Xor;

		case Lexem::Type::Conjunction: return BinaryOperator::LazyLogicalAnd;
		case Lexem::Type::Disjunction: return BinaryOperator::LazyLogicalOr;

		default:
		U_ASSERT(false);
		return BinaryOperator::None;
	};
}

static std::unique_ptr<NumericConstant> ParseNumericConstant(
	const Lexems::const_iterator num_it )
{
	const ProgramString& text= num_it->text;

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
		std::unique_ptr<NumericConstant>(
			new NumericConstant(
				num_it->file_pos,
				number,
				std::move( type_suffix ),
				has_fraction_point ) );
}

static BinaryOperatorsChainPtr ParseExpression(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	BinaryOperatorsChainPtr result( new BinaryOperatorsChain( it->file_pos ) );

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
				 prefix_operators.emplace_back( new UnaryPlus( it->file_pos ) );
				++it;
				break;

			case Lexem::Type::Minus:
				prefix_operators.emplace_back( new UnaryMinus( it->file_pos ) );
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
			if( it->text == Keywords::true_ )
				component.component.reset( new BooleanConstant( it->file_pos, true ) );
			else if( it->text == Keywords::false_ )
				component.component.reset( new BooleanConstant( it->file_pos, false ) );
			else
				component.component.reset( new NamedOperand( it->file_pos, it->text ) );

			++it;
		}
		else if( it->type == Lexem::Type::Number )
		{
			component.component= ParseNumericConstant( it );
			++it;
		}
		else if( it->type == Lexem::Type::BracketLeft )
		{
			++it;

			component.component.reset(
				new BracketExpression(
					(it-1)->file_pos,
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
							(it-1)->file_pos,
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
					const FilePos& call_operator_pos= it->file_pos;

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

					component.postfix_operators.emplace_back(
						new CallOperator(
							call_operator_pos,
							std::move( arguments ) ) );

				} break;

			case Lexem::Type::Dot:
				{
					++it; U_ASSERT( it < it_end );

					if( it->type != Lexem::Type::Identifier )
					{
						PushErrorMessage( error_messages, *it );
						return nullptr;
					}

					component.postfix_operators.emplace_back(
						new MemberAccessOperator(
								(it-1)->file_pos,
								it->text ) );

					++it; U_ASSERT( it < it_end );
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

static void ParseTypeName_r(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end,
	TypeName& result )
{
	if( it->type == Lexem::Type::SquareBracketLeft )
	{
		++it;
		ParseTypeName_r( error_messages, it, it_end, result );

		if( it->type != Lexem::Type::Comma )
		{
			PushErrorMessage( error_messages, *it );
			return;
		}
		++it;

		result.array_sizes.emplace_back( ParseNumericConstant( it ) );
		++it;

		if( it->type != Lexem::Type::SquareBracketRight )
		{
			PushErrorMessage( error_messages, *it );
			return;
		}

		++it;
	}
	else
	{
		if( it->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( error_messages, *it );
			return;
		}

		result.name= it->text;
		++it;
	}
}

static TypeName ParseTypeName(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	TypeName result;
	ParseTypeName_r( error_messages, it, it_end, result );
	return result;
}


static IInitializerPtr ParseInitializer(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end,
	bool parse_expression_initializer );

static std::unique_ptr<ArrayInitializer> ParseArrayInitializer(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it < it_end );
	U_ASSERT( it->type == Lexem::Type::SquareBracketLeft );

	std::unique_ptr<ArrayInitializer> result( new ArrayInitializer( it->file_pos ) );
	++it;
	U_ASSERT( it < it_end );

	while( it < it_end && it->type != Lexem::Type::SquareBracketRight )
	{
		result->initializers.push_back( ParseInitializer( error_messages, it, it_end, true ) );
		U_ASSERT( it < it_end );
		if( it->type == Lexem::Type::Comma )
			++it;
		else
			break;
		// TODO - parse continious flag here
	}
	if( it == it_end || it->type != Lexem::Type::SquareBracketRight )
	{
		PushErrorMessage( error_messages, *it );
		return result;
	}

	++it;

	return result;
}

static std::unique_ptr<StructNamedInitializer> ParseStructNamedInitializer(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it < it_end );
	U_ASSERT( it->type == Lexem::Type::BraceLeft );

	std::unique_ptr<StructNamedInitializer> result( new StructNamedInitializer( it->file_pos ) );
	++it;
	U_ASSERT( it < it_end );

	while( it < it_end && it->type != Lexem::Type::BraceRight )
	{
		U_ASSERT( it < it_end );
		if( it->type == Lexem::Type::Dot )
			++it;
		else
		{
			PushErrorMessage( error_messages, *it );
			return result;
		}

		U_ASSERT( it < it_end );
		if( it->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( error_messages, *it );
			return result;
		}
		ProgramString name= it->text;
		++it;

		IInitializerPtr initializer;
		if( it->type == Lexem::Type::Assignment )
		{
			++it;
			BinaryOperatorsChainPtr expression= ParseExpression( error_messages, it, it_end );
			initializer.reset( new ExpressionInitializer( it->file_pos, std::move(expression) ) );
		}
		else if(
			it->type == Lexem::Type::BracketLeft ||
			it->type == Lexem::Type::SquareBracketLeft ||
			it->type == Lexem::Type::BraceLeft )
		{
			initializer= ParseInitializer( error_messages, it, it_end, false );
		}

		result->members_initializers.emplace_back();
		result->members_initializers.back().name= std::move(name);
		result->members_initializers.back().initializer= std::move(initializer);

		if( it->type == Lexem::Type::Comma )
			++it;
	}
	if( it == it_end || it->type != Lexem::Type::SquareBracketRight )
	{
		PushErrorMessage( error_messages, *it );
		return result;
	}

	++it;

	return result;
}

static std::unique_ptr<ConstructorInitializer> ParseConstructorInitializer(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it < it_end );
	U_ASSERT( it->type == Lexem::Type::BracketLeft );

	++it;
	U_ASSERT( it < it_end );

	std::vector<BinaryOperatorsChainPtr> args;
	while( it < it_end && it->type != Lexem::Type::BracketRight )
	{
		args.push_back( ParseExpression( error_messages, it, it_end ) );
		U_ASSERT( it < it_end );
		if( it->type == Lexem::Type::Comma )
		{
			++it;
			U_ASSERT( it < it_end );
			// Disallow comma after closing bracket
			if( it->type == Lexem::Type::BracketRight )
			{
				PushErrorMessage( error_messages, *it );
				return nullptr;
			}
		}
		else
			break;
	}
	if( it == it_end || it->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}
	++it;

	std::unique_ptr<ConstructorInitializer> result(
		new ConstructorInitializer( it->file_pos, std::move(args) ) );

	return result;
}

static std::unique_ptr<ExpressionInitializer> ParseExpressionInitializer(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	std::unique_ptr<ExpressionInitializer> result(
		new ExpressionInitializer(
			it->file_pos,
			ParseExpression( error_messages, it, it_end ) ) );

	return result;
}

static IInitializerPtr ParseInitializer(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end,
	const bool parse_expression_initializer )
{
	U_ASSERT( it < it_end );

	if( it->type == Lexem::Type::SquareBracketLeft )
	{
		return ParseArrayInitializer( error_messages, it, it_end );
	}
	else if( it->type == Lexem::Type::BracketLeft )
	{
		return ParseConstructorInitializer( error_messages, it, it_end );
	}
	else if( it->type == Lexem::Type::BraceLeft )
	{
		return ParseStructNamedInitializer( error_messages, it, it_end );
	}
	else if( parse_expression_initializer )
	{
		// In some cases usage of expression in initializer is forbidden.
		return ParseExpressionInitializer( error_messages, it, it_end );
	}
	else
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}
}

static VariablesDeclarationPtr ParseVariablesDeclaration(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::let_ );
	U_ASSERT( it < it_end );

	++it;
	U_ASSERT( it < it_end );

	VariablesDeclarationPtr decl( new VariablesDeclaration( (it-1)->file_pos ) );

	if( it->type == Lexem::Type::Colon ) // Implicit type
	{
		++it;
		U_ASSERT( it < it_end );

		decl->type= ParseTypeName( error_messages, it, it_end );
	}

	do
	{
		decl->variables.emplace_back();
		VariablesDeclaration::VariableEntry& variable_entry= decl->variables.back();

		if( it->type == Lexem::Type::And )
		{
			variable_entry.reference_modifier= ReferenceModifier::Reference;
			++it;
			U_ASSERT( it < it_end );
		}

		if( it->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( error_messages, *it );
			return decl;
		}

		if( it->text == Keywords::mut_ )
		{
			variable_entry.mutability_modifier= MutabilityModifier::Mutable;
			++it;
			U_ASSERT( it < it_end );
		}
		else if( it->text == Keywords::imut_ )
		{
			variable_entry.mutability_modifier= MutabilityModifier::Immutable;
			++it;
			U_ASSERT( it < it_end );
		}

		if( it->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( error_messages, *it );
			return decl;
		}

		variable_entry.name= it->text;
		++it;
		U_ASSERT( it < it_end );

		if( it->type == Lexem::Type::Assignment )
		{
			++it;
			BinaryOperatorsChainPtr expression= ParseExpression( error_messages, it, it_end );
			variable_entry.initializer.reset( new ExpressionInitializer( it->file_pos, std::move(expression) ) );
		}
		else if(
			it->type == Lexem::Type::BracketLeft ||
			it->type == Lexem::Type::SquareBracketLeft ||
			it->type == Lexem::Type::BraceLeft )
		{
			variable_entry.initializer= ParseInitializer( error_messages, it, it_end, false );
		}

		if( it->type == Lexem::Type::Comma )
		{
			++it;
			U_ASSERT( it < it_end );
		}
		else if( it->type == Lexem::Type::Semicolon )
		{
			++it;
			U_ASSERT( it < it_end );
			break;
		}
		else
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}

	} while( it < it_end );

	return decl;
}

static IBlockElementPtr ParseReturnOperator(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::return_ );
	U_ASSERT( it < it_end );

	const FilePos& op_pos= it->file_pos;

	++it;
	U_ASSERT( it < it_end );

	if( it->type == Lexem::Type::Semicolon )
	{
		++it;
		return IBlockElementPtr( new ReturnOperator( op_pos, nullptr ) );
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

	return IBlockElementPtr( new ReturnOperator( op_pos, std::move( expression ) ) );
}

static IBlockElementPtr ParseWhileOperator(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::while_ );
	U_ASSERT( it < it_end );

	const FilePos& op_pos= it->file_pos;

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
				op_pos,
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

	const FilePos& op_pos= it->file_pos;

	++it;
	U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;

	return IBlockElementPtr( new BreakOperator( op_pos ) );
}

static IBlockElementPtr ParseContinueOperator(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_UNUSED( it_end );
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::continue_ );
	U_ASSERT( it < it_end );

	const FilePos& op_pos= it->file_pos;

	++it;
	U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}

	++it;

	return IBlockElementPtr( new ContinueOperator( op_pos ) );
}

static IBlockElementPtr ParseIfOperaotr(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::Identifier && it->text == Keywords::if_ );
	U_ASSERT( it < it_end );

	const FilePos& op_pos= it->file_pos;

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
				op_pos,
				std::move( branches ) ) );
}

static BlockPtr ParseBlock(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::BraceLeft );
	U_ASSERT( it < it_end );

	const FilePos& block_pos= it->file_pos;

	++it;

	BlockElements elements;

	while( it->type != Lexem::Type::EndOfFile )
	{
		if( it->type == Lexem::Type::BraceLeft )
			elements.emplace_back( ParseBlock( error_messages, it, it_end ) );

		else if( it->type == Lexem::Type::BraceRight )
			break;

		else if( it->type == Lexem::Type::Identifier && it->text == Keywords::let_ )
			elements.emplace_back( ParseVariablesDeclaration( error_messages, it, it_end ) );

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
						(it-2)->file_pos,
						std::move( l_expression ),
						std::move( r_expression ) ) );
			}
			else if( it->type == Lexem::Type::Semicolon )
			{
				++it;
				U_ASSERT( it < it_end );

				elements.emplace_back(
					new SingleExpressionOperator(
						(it-1)->file_pos,
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
			block_pos,
			std::move( elements ) ) );
}

static IProgramElementPtr ParseFunction(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->text == Keywords::fn_ );
	U_ASSERT( it < it_end );

	const FilePos& func_pos= it->file_pos;

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

	std::vector<FunctionArgumentDeclarationPtr> arguments;

	while(1)
	{
		if( it->type == Lexem::Type::BracketRight )
		{
			++it;
			break;
		}

		TypeName arg_type= ParseTypeName( error_messages, it, it_end );

		ReferenceModifier reference_modifier= ReferenceModifier::None;
		MutabilityModifier mutability_modifier= MutabilityModifier::Mutable;

		if( it->type == Lexem::Type::And )
		{
			reference_modifier= ReferenceModifier::Reference;
			++it;
			U_ASSERT( it < it_end );
		}

		if( it->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}

		if( it->text == Keywords::mut_ )
		{
			mutability_modifier= MutabilityModifier::Mutable;
			++it;
			U_ASSERT( it < it_end );
		}
		else if( it->text == Keywords::imut_ )
		{
			mutability_modifier= MutabilityModifier::Immutable;
			++it;
			U_ASSERT( it < it_end );
		}

		if( it->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}

		const FilePos& arg_file_pos= it->file_pos;
		const ProgramString& arg_name= it->text;
		++it;
		U_ASSERT( it < it_end );

		arguments.emplace_back(
			new FunctionArgumentDeclaration(
				arg_file_pos,
				arg_name,
				std::move(arg_type),
				mutability_modifier,
				reference_modifier ) );

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
	MutabilityModifier mutability_modifier;
	ReferenceModifier reference_modifier;

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

		if( it->type == Lexem::Type::And )
		{
			reference_modifier= ReferenceModifier::Reference;
			++it;
			U_ASSERT( it < it_end );
		}

		if( it->type == Lexem::Type::Identifier )
		{
			if( it->text == Keywords::mut_ )
			{
				mutability_modifier= MutabilityModifier::Mutable;
				++it;
				U_ASSERT( it < it_end );
			}
			else if( it->text == Keywords::imut_ )
			{
				mutability_modifier= MutabilityModifier::Immutable;
				++it;
				U_ASSERT( it < it_end );
			}
			else
				PushErrorMessage( error_messages, *it );
		}
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
			func_pos,
			std::move( fn_name ),
			return_type,
			mutability_modifier,
			reference_modifier,
			std::move( arguments ),
			std::move( block ) ) );
}

static std::unique_ptr<ClassDeclaration> ParseClass(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->text == Keywords::class_ );
	++it; U_ASSERT( it < it_end );

	std::unique_ptr<ClassDeclaration> result( new ClassDeclaration( it->file_pos ) );

	if( it->type != Lexem::Type::Identifier )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}
	result->name_= it->text;
	++it; U_ASSERT( it < it_end );

	if( it->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}
	++it; U_ASSERT( it < it_end );

	while( !(
		it->type == Lexem::Type::BraceRight ||
		it->type == Lexem::Type::EndOfFile ) )
	{
		if( it->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}

		result->fields_.emplace_back();
		ClassDeclaration::Field& field= result->fields_.back();
		field.file_pos= it->file_pos;

		field.name= it->text;
		++it; U_ASSERT( it < it_end );

		if( it->type != Lexem::Type::Colon )
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}
		++it; U_ASSERT( it < it_end );

		field.type= ParseTypeName( error_messages, it, it_end );

		if( it->type != Lexem::Type::Semicolon )
		{
			PushErrorMessage( error_messages, *it );
			return nullptr;
		}
		++it; U_ASSERT( it < it_end );
	}

	if( it->type != Lexem::Type::BraceRight )
	{
		PushErrorMessage( error_messages, *it );
		return nullptr;
	}
	++it;

	return result;
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
		else if( lexem.type == Lexem::Type::Identifier && lexem.text == Keywords::class_ )
		{
			if( IProgramElementPtr program_element= ParseClass( result.error_messages, it, it_end ) )
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

} // namespace U
