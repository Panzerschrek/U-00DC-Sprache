#include <cctype>
#include <cmath>

#include "assert.hpp"
#include "keywords.hpp"
#include "syntax_analyzer.hpp"

namespace U
{

static int GetBinaryOperatorPriority( const BinaryOperatorType binary_operator )
{
	#define PRIORITY ( - __LINE__ )

	switch( binary_operator )
	{
	case BinaryOperatorType::Div:
	case BinaryOperatorType::Mul:
	case BinaryOperatorType::Rem:
		return PRIORITY;
	case BinaryOperatorType::Add:
	case BinaryOperatorType::Sub:
		return PRIORITY;
	case BinaryOperatorType::ShiftLeft :
	case BinaryOperatorType::ShiftRight:
		return PRIORITY;
	case BinaryOperatorType::Equal:
	case BinaryOperatorType::NotEqual:
	case BinaryOperatorType::Less:
	case BinaryOperatorType::LessEqual:
	case BinaryOperatorType::Greater:
	case BinaryOperatorType::GreaterEqual:
		return PRIORITY;
	case BinaryOperatorType::And: return PRIORITY;
	case BinaryOperatorType::Or: return PRIORITY;
	case BinaryOperatorType::Xor: return PRIORITY;
	case BinaryOperatorType::LazyLogicalAnd: return PRIORITY;
	case BinaryOperatorType::LazyLogicalOr: return PRIORITY;
	};

	U_ASSERT(false);
	return PRIORITY;

	#undef PRIORITY
};

static bool IsBinaryOperator( const Lexem& lexem )
{
	return
		lexem.type == Lexem::Type::Plus ||
		lexem.type == Lexem::Type::Minus ||
		lexem.type == Lexem::Type::Star ||
		lexem.type == Lexem::Type::Slash ||
		lexem.type == Lexem::Type::Percent ||

		lexem.type == Lexem::Type::CompareEqual ||
		lexem.type == Lexem::Type::CompareNotEqual ||
		lexem.type == Lexem::Type::CompareLess ||
		lexem.type == Lexem::Type::CompareLessOrEqual ||
		lexem.type == Lexem::Type::CompareGreater ||
		lexem.type == Lexem::Type::CompareGreaterOrEqual ||

		lexem.type == Lexem::Type::And ||
		lexem.type == Lexem::Type::Or ||
		lexem.type == Lexem::Type::Xor ||

		lexem.type == Lexem::Type::ShiftLeft  ||
		lexem.type == Lexem::Type::ShiftRight ||

		lexem.type == Lexem::Type::Conjunction ||
		lexem.type == Lexem::Type::Disjunction;
}

static BinaryOperatorType LexemToBinaryOperator( const Lexem& lexem )
{
	switch( lexem.type )
	{
		case Lexem::Type::Plus: return BinaryOperatorType::Add;
		case Lexem::Type::Minus: return BinaryOperatorType::Sub;
		case Lexem::Type::Star: return BinaryOperatorType::Mul;
		case Lexem::Type::Slash: return BinaryOperatorType::Div;
		case Lexem::Type::Percent: return BinaryOperatorType::Rem;

		case Lexem::Type::CompareEqual: return BinaryOperatorType::Equal;
		case Lexem::Type::CompareNotEqual: return BinaryOperatorType::NotEqual;
		case Lexem::Type::CompareLess: return BinaryOperatorType::Less;
		case Lexem::Type::CompareLessOrEqual: return BinaryOperatorType::LessEqual;
		case Lexem::Type::CompareGreater: return BinaryOperatorType::Greater;
		case Lexem::Type::CompareGreaterOrEqual: return BinaryOperatorType::GreaterEqual;

		case Lexem::Type::And: return BinaryOperatorType::And;
		case Lexem::Type::Or: return BinaryOperatorType::Or;
		case Lexem::Type::Xor: return BinaryOperatorType::Xor;

		case Lexem::Type::ShiftLeft : return BinaryOperatorType::ShiftLeft ;
		case Lexem::Type::ShiftRight: return BinaryOperatorType::ShiftRight;

		case Lexem::Type::Conjunction: return BinaryOperatorType::LazyLogicalAnd;
		case Lexem::Type::Disjunction: return BinaryOperatorType::LazyLogicalOr;

		default:
		U_ASSERT(false);
		return BinaryOperatorType::Add;
	};
}

static BinaryOperatorType GetAdditiveAssignmentOperator( const Lexem& lexem )
{
	switch(lexem.type)
	{
		case Lexem::Type::AssignAdd: return BinaryOperatorType::Add;
		case Lexem::Type::AssignSub: return BinaryOperatorType::Sub;
		case Lexem::Type::AssignMul: return BinaryOperatorType::Mul;
		case Lexem::Type::AssignDiv: return BinaryOperatorType::Div;
		case Lexem::Type::AssignAnd: return BinaryOperatorType::And;
		case Lexem::Type::AssignRem: return BinaryOperatorType::Rem;
		case Lexem::Type::AssignOr : return BinaryOperatorType::Or;
		case Lexem::Type::AssignXor: return BinaryOperatorType::Xor;
		case Lexem::Type::AssignShiftLeft : return BinaryOperatorType::ShiftLeft;
		case Lexem::Type::AssignShiftRight: return BinaryOperatorType::ShiftRight;

		default:
		U_ASSERT(false);
		return BinaryOperatorType::Add;
	};
}

class SyntaxAnalyzer final
{
public:
	SyntaxAnalysisResult DoAnalyzis( const Lexems& lexems );

private:
	ProgramElements ParseNamespaceBody( Lexem::Type end_lexem );

	std::unique_ptr<NumericConstant> ParseNumericConstant();

	IExpressionComponentPtr ParseExpression();

	void ParseTypeName_r( TypeName& result );
	TypeName ParseTypeName();
	ComplexName ParseComplexName();

	IInitializerPtr ParseInitializer( bool parse_expression_initializer );
	std::unique_ptr<ArrayInitializer> ParseArrayInitializer();
	std::unique_ptr<StructNamedInitializer> ParseStructNamedInitializer();
	std::unique_ptr<ConstructorInitializer> ParseConstructorInitializer();
	std::unique_ptr<ExpressionInitializer> ParseExpressionInitializer();

	VariablesDeclarationPtr ParseVariablesDeclaration();
	std::unique_ptr<AutoVariableDeclaration> ParseAutoVariableDeclaration();

	IBlockElementPtr ParseReturnOperator();
	IBlockElementPtr ParseWhileOperator();
	IBlockElementPtr ParseBreakOperator();
	IBlockElementPtr ParseContinueOperator();
	IBlockElementPtr ParseIfOperator();
	IBlockElementPtr ParseStaticAssert();

	BlockPtr ParseBlock();

	std::unique_ptr<FunctionDeclaration> ParseFunction();
	std::unique_ptr<ClassDeclaration> ParseClass();
	std::unique_ptr<ClassDeclaration> ParseClassBody();

	IProgramElementPtr ParseTemplate();

	void PushErrorMessage( const Lexem& lexem );

private:
	SyntaxErrorMessages error_messages_;
	Lexems::const_iterator it_;
	Lexems::const_iterator it_end_;
};

SyntaxAnalysisResult SyntaxAnalyzer::DoAnalyzis( const Lexems& lexems )
{
	SyntaxAnalysisResult result;

	it_= lexems.begin();
	it_end_= lexems.end();

	while( it_ < it_end_ )
	{
		result.program_elements= ParseNamespaceBody( Lexem::Type::EndOfFile );
		++it_;
	}

	result.error_messages.swap( error_messages_ );
	return result;
}

ProgramElements SyntaxAnalyzer::ParseNamespaceBody( const Lexem::Type end_lexem )
{
	ProgramElements program_elements;

	while( it_ < it_end_ )
	{
		if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::fn_ )
		{
			if( IProgramElementPtr program_element= ParseFunction() )
				program_elements.emplace_back( std::move( program_element ) );
		}
		else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::struct_ || it_->text == Keywords::class_ ) )
		{
			if( IProgramElementPtr program_element= ParseClass() )
				program_elements.emplace_back( std::move( program_element ) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ )
		{
			if( IProgramElementPtr program_element= ParseTemplate() )
				program_elements.emplace_back( std::move( program_element ) );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::namespace_ )
		{
			++it_; U_ASSERT( it_ < it_end_ );

			ProgramString name;
			if( it_->type != Lexem::Type::Identifier )
			{
				PushErrorMessage( *it_ );
				return program_elements;
			}
			name= it_->text;
			++it_; U_ASSERT( it_ < it_end_ );

			if( it_->type != Lexem::Type::BraceLeft )
			{
				PushErrorMessage( *it_ );
				return program_elements;
			}
			++it_; U_ASSERT( it_ < it_end_ );

			std::unique_ptr<Namespace> namespace_( new Namespace( (it_-2)->file_pos ) );
			namespace_->name_= std::move(name);
			namespace_->elements_= ParseNamespaceBody( Lexem::Type::BraceRight );
			program_elements.push_back( std::move( namespace_ ) );
		}
		else if( it_->type == end_lexem )
		{
			// End of namespace
			++it_;
			return program_elements;
		}
		else
		{
			PushErrorMessage( *it_ );
			return program_elements;
		}
	} // while not end of file

	return program_elements;
}

std::unique_ptr<NumericConstant> SyntaxAnalyzer::ParseNumericConstant()
{
	const ProgramString& text= it_->text;

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
						return c - 'a' + 10;
					else
					{
						U_ASSERT( c >= 'A' && c <= 'F' );
						return c - 'A' + 10;
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
				it_->file_pos,
				number,
				std::move( type_suffix ),
				has_fraction_point ) );
}

IExpressionComponentPtr SyntaxAnalyzer::ParseExpression()
{
	IExpressionComponentPtr root;

	while(1)
	{
		PrefixOperators prefix_operators;

		// Prefix operators.
		while(1)
		{
			switch( it_->type )
			{
			case Lexem::Type::Identifier:
			case Lexem::Type::Scope:
			case Lexem::Type::Number:
			case Lexem::Type::BracketLeft:
				goto parse_operand;

			case Lexem::Type::Plus:
				 prefix_operators.emplace_back( new UnaryPlus( it_->file_pos ) );
				++it_;
				break;

			case Lexem::Type::Minus:
				prefix_operators.emplace_back( new UnaryMinus( it_->file_pos ) );
				++it_;
				break;

			case Lexem::Type::Not:
				prefix_operators.emplace_back( new LogicalNot( it_->file_pos ) );
				++it_;
				break;

			case Lexem::Type::Tilda:
				prefix_operators.emplace_back( new BitwiseNot( it_->file_pos ) );
				++it_;
				break;

			default:
				if( prefix_operators.empty() )
					return root;
				else
				{
					PushErrorMessage( *it_ );
					return nullptr;
				}
			};
		}

	parse_operand:
		ExpressionComponentWithUnaryOperatorsPtr current_node;

		// Operand.
		if( it_->type == Lexem::Type::Identifier )
		{
			if( it_->text == Keywords::true_ )
			{
				current_node.reset( new BooleanConstant( it_->file_pos, true ) );
				++it_;
			}
			else if( it_->text == Keywords::false_ )
			{
				current_node.reset( new BooleanConstant( it_->file_pos, false ) );
				++it_;
			}
			else
				current_node.reset( new NamedOperand( it_->file_pos, ParseComplexName() ) );
		}
		else if( it_->type == Lexem::Type::Scope )
		{
			current_node.reset( new NamedOperand( it_->file_pos, ParseComplexName() ) );
		}
		else if( it_->type == Lexem::Type::Number )
		{
			current_node= ParseNumericConstant();
			++it_;
		}
		else if( it_->type == Lexem::Type::BracketLeft )
		{
			++it_;

			current_node.reset(
				new BracketExpression(
					(it_-1)->file_pos,
					ParseExpression() ) );

			U_ASSERT( it_ < it_end_ );
			if( it_->type != Lexem::Type::BracketRight )
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}
			++it_;
		}
		else
		{
			U_ASSERT(false);
		}

		current_node->prefix_operators_= std::move( prefix_operators );

		bool is_binary_operator= false;
		// Postfix operators.
		bool end_postfix_operators= false;
		while( !end_postfix_operators )
		{
			switch( it_->type )
			{
			case Lexem::Type::SquareBracketLeft:
				{
					++it_;
					U_ASSERT( it_ < it_end_ );

					current_node->postfix_operators_.emplace_back(
						new IndexationOperator(
							(it_-1)->file_pos,
							ParseExpression() ) );

					U_ASSERT( it_ < it_end_ );
					if( it_->type != Lexem::Type::SquareBracketRight )
					{
						PushErrorMessage( *it_ );
						return nullptr;
					}
					++it_;
				}
				break;

			case Lexem::Type::BracketLeft:
				{
					const FilePos& call_operator_pos= it_->file_pos;

					++it_;
					U_ASSERT( it_ < it_end_ );

					std::vector<IExpressionComponentPtr> arguments;
					while(1)
					{
						if( it_->type == Lexem::Type::BracketRight )
						{
							++it_;
							break;
						}

						arguments.emplace_back( ParseExpression() );

						if( it_->type == Lexem::Type::Comma )
						{
							++it_;
							if( it_->type == Lexem::Type::BracketRight )
							{
								PushErrorMessage( *it_ );
								return nullptr;
							}
						}
						else if( it_->type == Lexem::Type::BracketRight )
						{
							++it_;
							break;
						}
					}

					current_node->postfix_operators_.emplace_back(
						new CallOperator(
							call_operator_pos,
							std::move( arguments ) ) );

				} break;

			case Lexem::Type::Dot:
				{
					++it_; U_ASSERT( it_ < it_end_ );

					if( it_->type != Lexem::Type::Identifier )
					{
						PushErrorMessage( *it_ );
						return nullptr;
					}

					current_node->postfix_operators_.emplace_back(
						new MemberAccessOperator(
								(it_-1)->file_pos,
								it_->text ) );

					++it_; U_ASSERT( it_ < it_end_ );
				} break;

			default:
				is_binary_operator= IsBinaryOperator( *it_ );
				end_postfix_operators= true;
				break;
			};

			if( end_postfix_operators )
				break;
		}

		if( root == nullptr )
		{
			root= std::move( current_node );
		}
		else
		{
			BinaryOperator* const root_as_binary_operator= dynamic_cast<BinaryOperator*>( root.get() );
			U_ASSERT( root_as_binary_operator != nullptr );

			// Place to existent tree last component.
			BinaryOperator* most_right_with_null= root_as_binary_operator;
			while( most_right_with_null->right_ != nullptr )
			{
				BinaryOperator* const right_as_binary_operator= dynamic_cast<BinaryOperator*>( most_right_with_null->right_.get() );
				U_ASSERT( right_as_binary_operator != nullptr );
				most_right_with_null= right_as_binary_operator;
			}
			most_right_with_null->right_= std::move( current_node );
		}

		if( is_binary_operator )
		{
			const BinaryOperatorType binary_operator_type= LexemToBinaryOperator( *it_ );
			std::unique_ptr<BinaryOperator> binary_operator( new BinaryOperator( it_->file_pos ) );
			binary_operator->operator_type_= binary_operator_type;
			++it_;
			U_ASSERT( it_ < it_end_ );

			if( BinaryOperator* const root_as_binary_operator= dynamic_cast<BinaryOperator*>( root.get() ) )
			{
				BinaryOperator* node_to_replace_parent= nullptr;
				BinaryOperator* node_to_replace= root_as_binary_operator;
				while( GetBinaryOperatorPriority( binary_operator->operator_type_ ) > GetBinaryOperatorPriority( node_to_replace->operator_type_ ) )
				{
					node_to_replace_parent= node_to_replace;
					BinaryOperator* const right_as_binary_operator= dynamic_cast<BinaryOperator*>( node_to_replace->right_.get() );
					if( right_as_binary_operator == nullptr )
						break;
					node_to_replace= right_as_binary_operator;
				}

				if( node_to_replace_parent != nullptr )
				{
					binary_operator->left_= std::move( node_to_replace_parent->right_ );
					node_to_replace_parent->right_= std::move( binary_operator );
				}
				else
				{
					binary_operator->left_= std::move( root );
					root= std::move( binary_operator );
				}
			}
			else
			{
				binary_operator->left_= std::move( root );
				root= std::move( binary_operator );
			}
		}
		else
			break;
	}

	return root;
}

void SyntaxAnalyzer::ParseTypeName_r( TypeName& result )
{
	if( it_->type == Lexem::Type::SquareBracketLeft )
	{
		++it_;
		ParseTypeName_r( result );

		if( it_->type != Lexem::Type::Comma )
		{
			PushErrorMessage( *it_ );
			return;
		}
		++it_;

		result.array_sizes.emplace_back( ParseExpression() );

		if( it_->type != Lexem::Type::SquareBracketRight )
		{
			PushErrorMessage( *it_ );
			return;
		}

		++it_;
	}
	else
		result.name= ParseComplexName();
}

TypeName SyntaxAnalyzer::ParseTypeName()
{
	TypeName result;
	ParseTypeName_r( result );
	return result;
}

ComplexName SyntaxAnalyzer::ParseComplexName()
{
	ComplexName complex_name;

	if( !( it_->type == Lexem::Type::Identifier || it_->type == Lexem::Type::Scope ) )
	{
		PushErrorMessage( *it_ );
		return complex_name;
	}

	if( it_->type == Lexem::Type::Scope )
	{
		complex_name.components.emplace_back();
		++it_; U_ASSERT( it_ < it_end_ );
	}

	do
	{
		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( *it_ );
			return complex_name;
		}
		complex_name.components.emplace_back();
		complex_name.components.back().name= it_->text;
		++it_; U_ASSERT( it_ < it_end_ );

		if( it_->type == Lexem::Type::TemplateBracketLeft )
		{
			complex_name.components.back().have_template_parameters= true;

			++it_; U_ASSERT( it_ < it_end_ );
			while(true)
			{
				if( it_->type == Lexem::Type::TemplateBracketRight )
				{
					++it_; U_ASSERT( it_ < it_end_ );
					break;
				}

				complex_name.components.back().template_parameters.push_back( ParseExpression() );

				if( it_->type == Lexem::Type::Comma )
				{
					++it_; U_ASSERT( it_ < it_end_ );
					if( it_->type == Lexem::Type::TemplateBracketRight )
					{
						PushErrorMessage( *it_ );
						return complex_name;
					}
				}
			}
		}

		if( it_->type == Lexem::Type::Scope )
		{
			++it_; U_ASSERT( it_ < it_end_ );
		}
		else
			break;

	} while(true);

	return complex_name;
}

IInitializerPtr SyntaxAnalyzer::ParseInitializer( const bool parse_expression_initializer )
{
	U_ASSERT( it_ < it_end_ );

	if( it_->type == Lexem::Type::SquareBracketLeft )
	{
		return ParseArrayInitializer();
	}
	else if( it_->type == Lexem::Type::BracketLeft )
	{
		return ParseConstructorInitializer();
	}
	else if( it_->type == Lexem::Type::BraceLeft )
	{
		return ParseStructNamedInitializer();
	}
	else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::zero_init_ )
	{
		const auto prev_it= it_;
		++it_;
		return IInitializerPtr( new ZeroInitializer( prev_it->file_pos ) );
	}
	else if( parse_expression_initializer )
	{
		// In some cases usage of expression in initializer is forbidden.
		return ParseExpressionInitializer();
	}
	else
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}
}

std::unique_ptr<ArrayInitializer> SyntaxAnalyzer::ParseArrayInitializer()
{
	U_ASSERT( it_ < it_end_ );
	U_ASSERT( it_->type == Lexem::Type::SquareBracketLeft );

	std::unique_ptr<ArrayInitializer> result( new ArrayInitializer( it_->file_pos ) );
	++it_;
	U_ASSERT( it_ < it_end_ );

	while( it_ < it_end_ && it_->type != Lexem::Type::SquareBracketRight )
	{
		result->initializers.push_back( ParseInitializer( true ) );
		U_ASSERT( it_ < it_end_ );
		if( it_->type == Lexem::Type::Comma )
			++it_;
		else
			break;
		// TODO - parse continious flag here
	}
	if( it_ == it_end_ || it_->type != Lexem::Type::SquareBracketRight )
	{
		PushErrorMessage( *it_ );
		return result;
	}

	++it_;

	return result;
}

std::unique_ptr<StructNamedInitializer> SyntaxAnalyzer::ParseStructNamedInitializer()
{
	U_ASSERT( it_ < it_end_ );
	U_ASSERT( it_->type == Lexem::Type::BraceLeft );

	std::unique_ptr<StructNamedInitializer> result( new StructNamedInitializer( it_->file_pos ) );
	++it_;
	U_ASSERT( it_ < it_end_ );

	while( it_ < it_end_ && it_->type != Lexem::Type::BraceRight )
	{
		U_ASSERT( it_ < it_end_ );
		if( it_->type == Lexem::Type::Dot )
			++it_;
		else
		{
			PushErrorMessage( *it_ );
			return result;
		}

		U_ASSERT( it_ < it_end_ );
		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( *it_ );
			return result;
		}
		ProgramString name= it_->text;
		++it_;

		IInitializerPtr initializer;
		if( it_->type == Lexem::Type::Assignment )
		{
			++it_;
			U_ASSERT( it_ < it_end_ );
			if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::zero_init_ )
			{
				initializer.reset( new ZeroInitializer( it_->file_pos ) );
				++it_;
			}
			else
			{
				initializer.reset( new ExpressionInitializer( it_->file_pos, ParseExpression() ) );
			}
		}
		else if(
			it_->type == Lexem::Type::BracketLeft ||
			it_->type == Lexem::Type::SquareBracketLeft ||
			it_->type == Lexem::Type::BraceLeft )
		{
			initializer= ParseInitializer( false );
		}

		result->members_initializers.emplace_back();
		result->members_initializers.back().name= std::move(name);
		result->members_initializers.back().initializer= std::move(initializer);

		if( it_->type == Lexem::Type::Comma )
			++it_;
	}
	if( it_ == it_end_ || it_->type != Lexem::Type::BraceRight )
	{
		PushErrorMessage( *it_ );
		return result;
	}

	++it_;

	return result;
}

std::unique_ptr<ConstructorInitializer> SyntaxAnalyzer::ParseConstructorInitializer()
{
	U_ASSERT( it_ < it_end_ );
	U_ASSERT( it_->type == Lexem::Type::BracketLeft );

	++it_;
	U_ASSERT( it_ < it_end_ );

	std::vector<IExpressionComponentPtr> args;
	while( it_ < it_end_ && it_->type != Lexem::Type::BracketRight )
	{
		args.push_back( ParseExpression() );
		U_ASSERT( it_ < it_end_ );
		if( it_->type == Lexem::Type::Comma )
		{
			++it_;
			U_ASSERT( it_ < it_end_ );
			// Disallow comma after closing bracket
			if( it_->type == Lexem::Type::BracketRight )
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}
		}
		else
			break;
	}
	if( it_ == it_end_ || it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}
	++it_;

	std::unique_ptr<ConstructorInitializer> result(
		new ConstructorInitializer( it_->file_pos, std::move(args) ) );

	return result;
}

std::unique_ptr<ExpressionInitializer> SyntaxAnalyzer::ParseExpressionInitializer()
{
	std::unique_ptr<ExpressionInitializer> result(
		new ExpressionInitializer(
			it_->file_pos,
			ParseExpression() ) );

	return result;
}

VariablesDeclarationPtr SyntaxAnalyzer::ParseVariablesDeclaration()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ );
	U_ASSERT( it_ < it_end_ );

	++it_;
	U_ASSERT( it_ < it_end_ );

	VariablesDeclarationPtr decl( new VariablesDeclaration( (it_-1)->file_pos ) );

	decl->type= ParseTypeName();

	do
	{
		decl->variables.emplace_back();
		VariablesDeclaration::VariableEntry& variable_entry= decl->variables.back();

		if( it_->type == Lexem::Type::And )
		{
			variable_entry.reference_modifier= ReferenceModifier::Reference;
			++it_;
			U_ASSERT( it_ < it_end_ );
		}

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( *it_ );
			return decl;
		}

		if( it_->text == Keywords::mut_ )
		{
			variable_entry.mutability_modifier= MutabilityModifier::Mutable;
			++it_;
			U_ASSERT( it_ < it_end_ );
		}
		else if( it_->text == Keywords::imut_ )
		{
			variable_entry.mutability_modifier= MutabilityModifier::Immutable;
			++it_;
			U_ASSERT( it_ < it_end_ );
		}
		else if( it_->text == Keywords::constexpr_ )
		{
			variable_entry.mutability_modifier= MutabilityModifier::Constexpr;
			++it_;
			U_ASSERT( it_ < it_end_ );
		}

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( *it_ );
			return decl;
		}

		variable_entry.name= it_->text;
		++it_;
		U_ASSERT( it_ < it_end_ );

		if( it_->type == Lexem::Type::Assignment )
		{
			++it_;
			U_ASSERT( it_ < it_end_ );
			if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::zero_init_ )
			{
				variable_entry.initializer.reset( new ZeroInitializer( it_->file_pos ) );
				++it_;
			}
			else
			{
				variable_entry.initializer.reset( new ExpressionInitializer( it_->file_pos, ParseExpression() ) );
			}
		}
		else if(
			it_->type == Lexem::Type::BracketLeft ||
			it_->type == Lexem::Type::SquareBracketLeft ||
			it_->type == Lexem::Type::BraceLeft )
		{
			variable_entry.initializer= ParseInitializer( false );
		}

		if( it_->type == Lexem::Type::Comma )
		{
			++it_;
			U_ASSERT( it_ < it_end_ );
		}
		else if( it_->type == Lexem::Type::Semicolon )
		{
			++it_;
			U_ASSERT( it_ < it_end_ );
			break;
		}
		else
		{
			PushErrorMessage( *it_ );
			return nullptr;
		}

	} while( it_ < it_end_ );

	return decl;
}

std::unique_ptr<AutoVariableDeclaration> SyntaxAnalyzer::ParseAutoVariableDeclaration()
{
	U_ASSERT( it_ < it_end_ );
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ );
	++it_;

	std::unique_ptr<AutoVariableDeclaration> result( new AutoVariableDeclaration( it_->file_pos ) );

	if( it_->type == Lexem::Type::And )
	{
		result->reference_modifier= ReferenceModifier::Reference;
		++it_; U_ASSERT( it_ < it_end_ );
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage( *it_ );
		return result;
	}

	if( it_->text == Keywords::mut_ )
	{
		result->mutability_modifier= MutabilityModifier::Mutable;
		++it_; U_ASSERT( it_ < it_end_ );
	}
	else if( it_->text == Keywords::imut_ )
	{
		result->mutability_modifier= MutabilityModifier::Immutable;
		++it_; U_ASSERT( it_ < it_end_ );
	}
	else if( it_->text == Keywords::constexpr_ )
	{
		result->mutability_modifier= MutabilityModifier::Constexpr;
		++it_; U_ASSERT( it_ < it_end_ );
	}

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage( *it_ );
		return result;
	}

	result->name= it_->text;
	++it_; U_ASSERT( it_ < it_end_ );

	if( it_->type != Lexem::Type::Assignment )
	{
		PushErrorMessage( *it_ );
		return result;
	}
	++it_; U_ASSERT( it_ < it_end_ );

	result->initializer_expression = ParseExpression();

	U_ASSERT( it_ < it_end_ );
	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage( *it_ );
		return result;
	}
	++it_;

	return result;
}

IBlockElementPtr SyntaxAnalyzer::ParseReturnOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::return_ );
	U_ASSERT( it_ < it_end_ );

	const FilePos& op_pos= it_->file_pos;

	++it_;
	U_ASSERT( it_ < it_end_ );

	if( it_->type == Lexem::Type::Semicolon )
	{
		++it_;
		return IBlockElementPtr( new ReturnOperator( op_pos, nullptr ) );
	}

	IExpressionComponentPtr expression= ParseExpression();

	U_ASSERT( it_ < it_end_ );
	if( it_->type != Lexem::Type::Semicolon  )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	++it_;

	return IBlockElementPtr( new ReturnOperator( op_pos, std::move( expression ) ) );
}

IBlockElementPtr SyntaxAnalyzer::ParseWhileOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::while_ );
	U_ASSERT( it_ < it_end_ );

	const FilePos& op_pos= it_->file_pos;

	++it_;
	U_ASSERT( it_ < it_end_ );
	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	++it_;
	U_ASSERT( it_ < it_end_ );

	IExpressionComponentPtr condition= ParseExpression();

	if( it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	++it_;
	U_ASSERT( it_ < it_end_ );

	if( it_->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	BlockPtr block= ParseBlock();

	return
		IBlockElementPtr(
			new WhileOperator(
				op_pos,
				std::move( condition ),
				std::move( block ) ) );
}

IBlockElementPtr SyntaxAnalyzer::ParseBreakOperator()
{
	U_UNUSED( it_end_ );

	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::break_ );
	U_ASSERT( it_ < it_end_ );

	const FilePos& op_pos= it_->file_pos;

	++it_;
	U_ASSERT( it_ < it_end_ );

	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	++it_;

	return IBlockElementPtr( new BreakOperator( op_pos ) );
}

IBlockElementPtr SyntaxAnalyzer::ParseContinueOperator()
{
	U_UNUSED( it_end_ );
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::continue_ );
	U_ASSERT( it_ < it_end_ );

	const FilePos& op_pos= it_->file_pos;

	++it_;
	U_ASSERT( it_ < it_end_ );

	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	++it_;

	return IBlockElementPtr( new ContinueOperator( op_pos ) );
}

IBlockElementPtr SyntaxAnalyzer::ParseIfOperator()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ );
	U_ASSERT( it_ < it_end_ );

	const FilePos& op_pos= it_->file_pos;

	++it_;
	U_ASSERT( it_ < it_end_ );

	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	++it_;
	U_ASSERT( it_ < it_end_ );

	std::vector<IfOperator::Branch> branches;
	branches.emplace_back();

	branches.back().condition= ParseExpression();

	if( it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	++it_;
	U_ASSERT( it_ < it_end_ );

	if( it_->type != Lexem::Type::BraceLeft )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}
	branches.back().block= ParseBlock();

	while(1)
	{
		if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::else_ )
		{
			branches.emplace_back();

			++it_;
			U_ASSERT( it_ < it_end_ );

			// Optional if.
			if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ )
			{
				++it_;
				U_ASSERT( it_ < it_end_ );

				if( it_->type != Lexem::Type::BracketLeft )
				{
					PushErrorMessage( *it_ );
					return nullptr;
				}

				++it_;
				U_ASSERT( it_ < it_end_ );

				branches.back().condition= ParseExpression();

				if( it_->type != Lexem::Type::BracketRight )
				{
					PushErrorMessage( *it_ );
					return nullptr;
				}

				++it_;
				U_ASSERT( it_ < it_end_ );
			}
			// Block - common for "else" and "else if".
			if( it_->type != Lexem::Type::BraceLeft )
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}

			branches.back().block= ParseBlock();

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

IBlockElementPtr SyntaxAnalyzer::ParseStaticAssert()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ );

	std::unique_ptr<StaticAssert> result( new StaticAssert( it_->file_pos ) );

	++it_; U_ASSERT( it_ < it_end_ );

	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}
	++it_; U_ASSERT( it_ < it_end_ );

	result->expression= ParseExpression();

	if( it_->type != Lexem::Type::BracketRight )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}
	++it_; U_ASSERT( it_ < it_end_ );

	if( it_->type != Lexem::Type::Semicolon )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}
	++it_; U_ASSERT( it_ < it_end_ );

	return std::move(result);
}

BlockPtr SyntaxAnalyzer::ParseBlock()
{
	U_ASSERT( it_->type == Lexem::Type::BraceLeft );
	U_ASSERT( it_ < it_end_ );

	const FilePos& block_pos= it_->file_pos;

	++it_;

	BlockElements elements;

	while( it_->type != Lexem::Type::EndOfFile )
	{
		if( it_->type == Lexem::Type::BraceLeft )
			elements.emplace_back( ParseBlock() );

		else if( it_->type == Lexem::Type::BraceRight )
			break;

		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::var_ )
			elements.emplace_back( ParseVariablesDeclaration() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::auto_ )
			elements.emplace_back( ParseAutoVariableDeclaration() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::return_ )
			elements.emplace_back( ParseReturnOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::while_ )
			elements.emplace_back( ParseWhileOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::break_ )
			elements.emplace_back( ParseBreakOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::continue_ )
			elements.emplace_back( ParseContinueOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::if_ )
			elements.emplace_back( ParseIfOperator() );
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::static_assert_ )
			elements.emplace_back( ParseStaticAssert() );

		else if( it_->type == Lexem::Type::Increment )
		{
			std::unique_ptr<IncrementOperator> op( new IncrementOperator( it_->file_pos ) );
			++it_; U_ASSERT( it_ < it_end_ );

			op->expression= ParseExpression();
			elements.push_back( std::move(op) );

			if( it_->type != Lexem::Type::Semicolon )
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}
			++it_; U_ASSERT( it_ < it_end_ );
		}
		else if( it_->type == Lexem::Type::Decrement )
		{
			std::unique_ptr<DecrementOperator> op( new DecrementOperator( it_->file_pos ) );
			++it_; U_ASSERT( it_ < it_end_ );

			op->expression= ParseExpression();
			elements.push_back( std::move(op) );

			if( it_->type != Lexem::Type::Semicolon )
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}
			++it_; U_ASSERT( it_ < it_end_ );
		}

		else
		{
			IExpressionComponentPtr l_expression= ParseExpression();

			U_ASSERT( it_ < it_end_ );
			if( it_->type == Lexem::Type::Assignment )
			{
				++it_;
				U_ASSERT( it_ < it_end_ );

				IExpressionComponentPtr r_expression= ParseExpression();

				if( it_->type != Lexem::Type::Semicolon )
				{
					PushErrorMessage( *it_ );
					return nullptr;
				}
				++it_;
				U_ASSERT( it_ < it_end_ );

				elements.emplace_back(
					new AssignmentOperator(
						(it_-2)->file_pos,
						std::move( l_expression ),
						std::move( r_expression ) ) );
			}
			else if(
				it_->type == Lexem::Type::AssignAdd ||
				it_->type == Lexem::Type::AssignSub ||
				it_->type == Lexem::Type::AssignMul ||
				it_->type == Lexem::Type::AssignDiv ||
				it_->type == Lexem::Type::AssignRem ||
				it_->type == Lexem::Type::AssignAnd ||
				it_->type == Lexem::Type::AssignOr  ||
				it_->type == Lexem::Type::AssignXor ||
				it_->type == Lexem::Type::AssignShiftLeft  ||
				it_->type == Lexem::Type::AssignShiftRight )
			{
				std::unique_ptr<AdditiveAssignmentOperator> op( new AdditiveAssignmentOperator( it_->file_pos ) );

				op->additive_operation_= GetAdditiveAssignmentOperator( *it_ );
				++it_; U_ASSERT( it_ < it_end_ );

				op->l_value_= std::move(l_expression);
				op->r_value_= ParseExpression();

				elements.push_back( std::move(op) );

				if( it_->type != Lexem::Type::Semicolon )
				{
					PushErrorMessage( *it_ );
					return nullptr;
				}
				++it_; U_ASSERT( it_ < it_end_ );
			}
			else if( it_->type == Lexem::Type::Semicolon )
			{
				++it_;
				U_ASSERT( it_ < it_end_ );

				elements.emplace_back(
					new SingleExpressionOperator(
						(it_-1)->file_pos,
						std::move( l_expression ) ) );
			}
			else
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}
		}
	}

	if( it_->type == Lexem::Type::BraceRight )
		++it_;
	else
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	return BlockPtr(
		new Block(
			block_pos,
			std::move( elements ) ) );
}

std::unique_ptr<FunctionDeclaration> SyntaxAnalyzer::ParseFunction()
{
	U_ASSERT( it_->text == Keywords::fn_ );
	U_ASSERT( it_ < it_end_ );

	const FilePos& func_pos= it_->file_pos;
	++it_; U_ASSERT( it_ < it_end_ );

	ComplexName fn_name= ParseComplexName();

	if( it_->type != Lexem::Type::BracketLeft )
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	++it_;
	U_ASSERT( it_ < it_end_ );

	std::vector<FunctionArgumentDeclarationPtr> arguments;

	// Try parse "this"
	if( it_->type == Lexem::Type::Identifier )
	{
		bool is_this= false;
		MutabilityModifier mutability_modifier= MutabilityModifier::None;
		if( it_->text == Keywords::mut_ )
		{
			++it_; U_ASSERT( it_ < it_end_ );
			if( !( it_->type == Lexem::Type::Identifier && it_->text == Keywords::this_ ) )
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}
			++it_; U_ASSERT( it_ < it_end_ );

			is_this= true;
			mutability_modifier= MutabilityModifier::Mutable;
		}
		else if( it_->text == Keywords::imut_ )
		{
			++it_; U_ASSERT( it_ < it_end_ );
			if( !( it_->type == Lexem::Type::Identifier && it_->text == Keywords::this_ ) )
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}
			++it_; U_ASSERT( it_ < it_end_ );

			is_this= true;
			mutability_modifier= MutabilityModifier::Immutable;
		}
		else if( it_->text == Keywords::this_ )
		{
			is_this= true;
			++it_; U_ASSERT( it_ < it_end_ );
		}

		if( is_this )
		{
			arguments.emplace_back(
				new FunctionArgumentDeclaration(
					it_->file_pos,
					Keyword( Keywords::this_ ),
					TypeName(),
					mutability_modifier,
					ReferenceModifier::Reference ) );

			if( it_->type == Lexem::Type::Comma )
			{
				++it_;
				// Disallov constructions, like "fn f( mut this, ){}"
				if( it_->type == Lexem::Type::BracketRight )
				{
					PushErrorMessage( *it_ );
					return nullptr;
				}
			}
		}
	}

	while(1)
	{
		if( it_->type == Lexem::Type::BracketRight )
		{
			++it_;
			break;
		}

		TypeName arg_type= ParseTypeName();

		ReferenceModifier reference_modifier= ReferenceModifier::None;
		MutabilityModifier mutability_modifier= MutabilityModifier::Mutable;

		if( it_->type == Lexem::Type::And )
		{
			reference_modifier= ReferenceModifier::Reference;
			++it_;
			U_ASSERT( it_ < it_end_ );
		}

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( *it_ );
			return nullptr;
		}

		if( it_->text == Keywords::mut_ )
		{
			mutability_modifier= MutabilityModifier::Mutable;
			++it_;
			U_ASSERT( it_ < it_end_ );
		}
		else if( it_->text == Keywords::imut_ )
		{
			mutability_modifier= MutabilityModifier::Immutable;
			++it_;
			U_ASSERT( it_ < it_end_ );
		}

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( *it_ );
			return nullptr;
		}

		const FilePos& arg_file_pos= it_->file_pos;
		const ProgramString& arg_name= it_->text;
		++it_;
		U_ASSERT( it_ < it_end_ );

		arguments.emplace_back(
			new FunctionArgumentDeclaration(
				arg_file_pos,
				arg_name,
				std::move(arg_type),
				mutability_modifier,
				reference_modifier ) );

		if( it_->type == Lexem::Type::Comma )
		{
			++it_;
			// Disallov constructions, like "fn f( a : int, ){}"
			if( it_->type == Lexem::Type::BracketRight )
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}
		}
		else if( it_->type == Lexem::Type::BracketRight )
		{}
		else
		{
			PushErrorMessage( *it_ );
			return nullptr;
		}
	}

	TypeName return_type;
	MutabilityModifier mutability_modifier= MutabilityModifier::None;
	ReferenceModifier reference_modifier= ReferenceModifier::None;

	if( it_->type == Lexem::Type::Colon )
	{
		++it_;
		U_ASSERT( it_ < it_end_ );

		return_type= ParseTypeName();

		if( it_->type == Lexem::Type::And )
		{
			reference_modifier= ReferenceModifier::Reference;
			++it_;
			U_ASSERT( it_ < it_end_ );
		}

		if( it_->type == Lexem::Type::Identifier )
		{
			if( it_->text == Keywords::mut_ )
			{
				mutability_modifier= MutabilityModifier::Mutable;
				++it_;
				U_ASSERT( it_ < it_end_ );
			}
			else if( it_->text == Keywords::imut_ )
			{
				mutability_modifier= MutabilityModifier::Immutable;
				++it_;
				U_ASSERT( it_ < it_end_ );
			}
			else
				PushErrorMessage( *it_ );
		}
	}

	std::unique_ptr<StructNamedInitializer> constructor_initialization_list;
	BlockPtr block;

	if( it_->type == Lexem::Type::Semicolon )
	{
		// function prototype
		++it_; U_ASSERT( it_ < it_end_ );
	}
	else
	{
		if( it_->type == Lexem::Type::BracketLeft )
		{
			constructor_initialization_list.reset( new StructNamedInitializer( it_->file_pos ) );
			++it_; U_ASSERT( it_ < it_end_ );

			while( true )
			{
				if( it_->type == Lexem::Type::BracketRight )
				{
					++it_; U_ASSERT( it_ < it_end_ );
					break;
				}

				if( it_->type != Lexem::Type::Identifier )
				{
					PushErrorMessage( *it_ );
					return nullptr;
				}
				constructor_initialization_list->members_initializers.emplace_back();
				constructor_initialization_list->members_initializers.back().name= it_->text;
				IInitializerPtr& initializer= constructor_initialization_list->members_initializers.back().initializer;

				++it_; U_ASSERT( it_ < it_end_ );

				if( it_->type == Lexem::Type::Assignment )
				{
					++it_;
					U_ASSERT( it_ < it_end_ );
					if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::zero_init_ )
					{
						initializer.reset( new ZeroInitializer( it_->file_pos ) );
						++it_;
					}
					else
						initializer.reset( new ExpressionInitializer( it_->file_pos, ParseExpression() ) );
				}
				else
					initializer= ParseInitializer( false );

				if( it_->type == Lexem::Type::Comma )
					++it_; U_ASSERT( it_ < it_end_ );
			}
		}

		if( it_->type == Lexem::Type::BraceLeft )
			block= ParseBlock();
		else
		{
			PushErrorMessage( *it_ );
			return nullptr;
		}
	}

	return std::unique_ptr<FunctionDeclaration>(
		new FunctionDeclaration(
			func_pos,
			std::move( fn_name ),
			std::move(return_type),
			mutability_modifier,
			reference_modifier,
			std::move( arguments ),
			std::move( constructor_initialization_list ),
			std::move( block ) ) );
}

std::unique_ptr<ClassDeclaration> SyntaxAnalyzer::ParseClass()
{
	U_ASSERT( it_->text == Keywords::struct_ || it_->text == Keywords::class_ );
	++it_; U_ASSERT( it_ < it_end_ );

	ComplexName name= ParseComplexName();

	std::unique_ptr<ClassDeclaration> result= ParseClassBody();
	if( result != nullptr )
		result->name_= std::move(name);

	return result;
}

std::unique_ptr<ClassDeclaration> SyntaxAnalyzer::ParseClassBody()
{
	std::unique_ptr<ClassDeclaration> result( new ClassDeclaration( it_->file_pos ) );

	if( it_->type == Lexem::Type::Semicolon )
	{
		++it_; U_ASSERT( it_ < it_end_ );
		result->is_forward_declaration_= true;
		return result;
	}
	else if( it_->type == Lexem::Type::BraceLeft )
	{
		++it_; U_ASSERT( it_ < it_end_ );
	}
	else
	{
		PushErrorMessage( *it_ );
		return nullptr;
	}

	while( !(
		it_->type == Lexem::Type::BraceRight ||
		it_->type == Lexem::Type::EndOfFile ) )
	{
		// SPRACHE_TODO - try parse here, subclasses, typedefs, etc.
		if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::fn_ )
		{
			result->members_.push_back( ParseFunction() );
		}
		else if( it_->type == Lexem::Type::Identifier && ( it_->text == Keywords::struct_ || it_->text == Keywords::class_ ) )
		{
			result->members_.push_back( ParseClass() );
		}
		else if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ )
		{
			// TODO
			//result->members_.push_back( ParseTemplate() );
		}
		else
		{
			ClassDeclaration::Field field;
			field.file_pos= it_->file_pos;

			field.type= ParseTypeName();

			U_ASSERT( it_ < it_end_ );
			if( it_->type != Lexem::Type::Identifier )
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}
			field.name= it_->text;
			++it_;

			if( it_->type != Lexem::Type::Semicolon )
			{
				PushErrorMessage( *it_ );
				return nullptr;
			}
			++it_;U_ASSERT( it_ < it_end_ );

			result->members_.push_back( std::move( field ) );
		}
	}

	if( it_->type != Lexem::Type::BraceRight )
	{
		PushErrorMessage(*it_ );
		return nullptr;
	}
	++it_;

	return result;
}

IProgramElementPtr SyntaxAnalyzer::ParseTemplate()
{
	U_ASSERT( it_->type == Lexem::Type::Identifier && it_->text == Keywords::template_ );
	++it_; U_ASSERT( it_ < it_end_ );

	std::unique_ptr<ClassTemplateDeclaration> result( new ClassTemplateDeclaration( it_->file_pos ) );

	if( it_->type != Lexem::Type::TemplateBracketLeft )
	{
		PushErrorMessage( *it_ );
		return std::move(result);
	}
	++it_; U_ASSERT( it_ < it_end_ );

	while( true )
	{
		if( it_->type == Lexem::Type::TemplateBracketRight )
		{
			++it_; U_ASSERT( it_ < it_end_ );
			break;
		}

		result->args_.emplace_back();

		if( it_->type == Lexem::Type::Identifier && it_->text == Keywords::type_ )
		{
			++it_; U_ASSERT( it_ < it_end_ );
		}
		else
			result->args_.back().arg_type= ParseComplexName();

		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( *it_ );
			return std::move(result);
		}
		result->args_.back().name= it_->text;
		++it_; U_ASSERT( it_ < it_end_ );

		if( it_->type == Lexem::Type::Comma )
		{
			++it_; U_ASSERT( it_ < it_end_ );
			if( it_->type == Lexem::Type::TemplateBracketRight )
			{
				PushErrorMessage( *it_ );
				return std::move(result);
			}
		}
	} // for arg parameters

	if( it_->type != Lexem::Type::Identifier )
	{
		PushErrorMessage( *it_ );
		return std::move(result);
	}

	ComplexName name;
	if( it_->text == Keywords::struct_ || it_->text == Keywords::class_ )
	{
		++it_; U_ASSERT( it_ < it_end_ );

		//name= ParseComplexName();
		if( it_->type != Lexem::Type::Identifier )
		{
			PushErrorMessage( *it_ );
			return std::move(result);
		}
		name.components.emplace_back();
		name.components.back().name= it_->text;
		++it_; U_ASSERT( it_ < it_end_ );
	}
	else
	{
		// TODO - parse functions templates here
		PushErrorMessage( *it_ );
		return std::move(result);
	}

	if( it_->type != Lexem::Type::TemplateBracketLeft )
	{
		PushErrorMessage( *it_ );
		return std::move(result);
	}
	++it_; U_ASSERT( it_ < it_end_ );

	// Parse signature args
	while( true )
	{
		if( it_->type == Lexem::Type::TemplateBracketRight )
		{
			++it_; U_ASSERT( it_ < it_end_ );
			break;
		}

		result->signature_args_.emplace_back();
		result->signature_args_.back().name= ParseComplexName();

		if( it_->type == Lexem::Type::Comma )
		{
			++it_; U_ASSERT( it_ < it_end_ );
			if( it_->type == Lexem::Type::TemplateBracketRight )
			{
				PushErrorMessage( *it_ );
				return std::move(result);
			}
		}
	} // for signature args

	result->class_= ParseClassBody();
	if( result->class_ != nullptr )
		result->class_->name_= std::move(name);

	return std::move(result);
}

void SyntaxAnalyzer::PushErrorMessage( const Lexem& lexem )
{
	error_messages_.emplace_back(
		std::to_string(lexem.file_pos.line) + ":" + std::to_string(lexem.file_pos.pos_in_line) +
		" Syntax error - unexpected lexem: " + ToStdString(lexem.text) );
}

SyntaxAnalysisResult SyntaxAnalysis( const Lexems& lexems )
{
	return SyntaxAnalyzer().DoAnalyzis( lexems );
}

} // namespace U
