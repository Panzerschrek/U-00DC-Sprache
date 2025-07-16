#include <algorithm>
#include <cmath>
#include <cstring>

#include "../../lex_synt_lib_common/assert.hpp"
#include "program_string.hpp"
#include "lexical_analyzer.hpp"

namespace U
{

namespace
{

using FixedLexemsMap= ProgramStringMap<Lexem::Type>;
const size_t g_max_fixed_lexem_size= 3;

const FixedLexemsMap g_fixed_lexems[ g_max_fixed_lexem_size + 1 ]=
{
	FixedLexemsMap
	{ // Zero symbol lexems.
	},
	FixedLexemsMap
	{ // One symbol lexems.
		{ "(", Lexem::Type::BracketLeft },
		{ ")", Lexem::Type::BracketRight },
		{ "[", Lexem::Type::SquareBracketLeft },
		{ "]", Lexem::Type::SquareBracketRight },
		{ "{", Lexem::Type::BraceLeft },
		{ "}", Lexem::Type::BraceRight },

		{ ",", Lexem::Type::Comma },
		{ ".", Lexem::Type::Dot },
		{ ":", Lexem::Type::Colon },
		{ ";", Lexem::Type::Semicolon },
		{ "?", Lexem::Type::Question },

		{ "=", Lexem::Type::Equal },
		{ "+", Lexem::Type::Plus },
		{ "-", Lexem::Type::Minus },
		{ "*", Lexem::Type::Asterisk },
		{ "/", Lexem::Type::Slash },
		{ "%", Lexem::Type::Percent },

		{ "<", Lexem::Type::Less },
		{ ">", Lexem::Type::Greater },

		{ "&", Lexem::Type::Ampersand },
		{ "|", Lexem::Type::Pipe },
		{ "^", Lexem::Type::Caret },
		{ "~", Lexem::Type::Tilde },
		{ "!", Lexem::Type::Exclamation },

		{ "@", Lexem::Type::At },

		{ "$", Lexem::Type::Dollar },
	},
	FixedLexemsMap
	{ // Two symbol lexems.
		{ "</", Lexem::Type::TemplateBracketLeft  },
		{ "/>", Lexem::Type::TemplateBracketRight },

		{ "<?", Lexem::Type::MacroBracketLeft  },
		{ "?>", Lexem::Type::MacroBracketRight },

		{ "::", Lexem::Type::DoubleColon },

		{ "++", Lexem::Type::DoublePlus },
		{ "--", Lexem::Type::DoubleMinus },

		{ "==", Lexem::Type::DoubleEqual },
		{ "!=", Lexem::Type::ExclamationEqual },
		{ "<=", Lexem::Type::LessEqual },
		{ ">=", Lexem::Type::GreaterEqual },

		{ "&&", Lexem::Type::DoubleAmpersand },
		{ "||", Lexem::Type::DoublePipe },

		{ "+=", Lexem::Type::PlusEqual },
		{ "-=", Lexem::Type::MinusEqual },
		{ "*=", Lexem::Type::AsteriskEqual },
		{ "/=", Lexem::Type::SlashEqual },
		{ "%=", Lexem::Type::PercentEqual },
		{ "&=", Lexem::Type::AmpersandEqual },
		{ "|=", Lexem::Type::PipeEqual },
		{ "^=", Lexem::Type::CaretEqual },

		{ "<<", Lexem::Type::DoubleLess },
		{ ">>", Lexem::Type::DoubleGreater },

		{ "->", Lexem::Type::MinusGreater },

		{ "$<", Lexem::Type::DollarLess },
		{ "$>", Lexem::Type::DollarGreater },
	},
	FixedLexemsMap
	{ // Three symbol lexems.
		{ "<=>", Lexem::Type::LessEqualGreater },
		{ "<<=", Lexem::Type::DoubleLessEqual },
		{ ">>=", Lexem::Type::DoubleGreaterEqual },
		{ "...", Lexem::Type::Ellipsis },
	},
};

using Iterator= const char*;

bool IsWhitespace( const sprache_char c )
{
	return
		c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v' ||
		c <= 0x1Fu || c == 0x7Fu;
}

bool IsNewline( const sprache_char c )
{
	// See https://en.wikipedia.org/wiki/Newline#Unicode.
	return
		c == '\n' || // line feed
		c == '\r' || // carriage return
		c == '\f' || // form feed
		c == '\v' || // vertical tab
		c == 0x0085 || // Next line
		c == 0x2028 || // line separator
		c == 0x2029 ;  // paragraph separator
}

bool IsNewlineSequence( const sprache_char c0, const sprache_char c1 )
{
	return c0 == '\r' && c1 == '\n';
}

bool IsNumberStartChar( const sprache_char c )
{
	return c >= '0' && c <= '9';
}

bool IsIdentifierStartChar( const sprache_char c )
{
	// HACK - manually define allowed "letters".
	// TODO - use something, like symbol category from unicode.
	return
		( c >= 'a' && c <= 'z' ) ||
		( c >= 'A' && c <= 'Z' ) ||
		( c >= 0x0400u && c <= 0x04FFu ) || // Cyrillic
		( c >= 0x0500u && c <= 0x0527u ) || // Extended cyrillic
		( c >= 0x00C0u && c <= 0x00D6u ) || // Additional latin symbols
		( c >= 0x00D8u && c <= 0x00F6u ) || // Additional latin symbols
		( c >= 0x00F8u && c <= 0x00FFu ) || // Additional latin symbols
		( c >= 0x0100u && c <= 0x017Fu ) || // Extended latin part A
		( c >= 0x0180u && c <= 0x024Fu ) ;  // Extended latin part B
}

bool IsIdentifierChar( const sprache_char c )
{
	return IsIdentifierStartChar(c) || IsNumberStartChar(c) || c == '_';
}

Lexem ParseCharLiteral( Iterator& it, const Iterator it_end, const SrcLoc& src_loc, LexSyntErrors& out_errors )
{
	U_ASSERT( *it == '\'' );
	++it;

	Lexem result;
	result.type= Lexem::Type::CharLiteral;

	if( it == it_end )
	{
		out_errors.emplace_back( "unexpected end of file after '", src_loc );
		return result;
	}

	const char c = *it;
	if( c == '\'' )
	{
		out_errors.emplace_back( "empty char literal", src_loc );
		return result;
	}
	else if( c == '\\' )
	{
		++it;
		const char escaped_c= *it;
		++it;
		switch( escaped_c )
		{
		case '"': result.text.push_back('"'); break;
		case '\'': result.text.push_back('\''); break;
		case '\\': result.text.push_back('\\'); break;
		case '/': result.text.push_back('/'); break;
		case 'b': result.text.push_back('\b'); break;
		case 'f': result.text.push_back('\f'); break;
		case 'n': result.text.push_back('\n'); break;
		case 'r': result.text.push_back('\r'); break;
		case 't': result.text.push_back('\t'); break;
		case '0': result.text.push_back('\0'); break;

		case 'u':
			{
				// Parse hex number.
				if( it_end - it < 4 )
				{
					out_errors.emplace_back( "expected 4 hex digits", src_loc );
					return result;
				}

				sprache_char char_code= 0u;
				for( size_t i= 0u; i < 4u; i++ )
				{
					sprache_char digit;
						 if( *it >= '0' && *it <= '9' ) digit= uint32_t( *it - '0' );
					else if( *it >= 'a' && *it <= 'f' ) digit= uint32_t( *it - 'a' + 10 );
					else if( *it >= 'A' && *it <= 'F' ) digit= uint32_t( *it - 'A' + 10 );
					else
					{
						out_errors.emplace_back( "expected hex number", src_loc );
						return result;
					}
					char_code|= digit << ( ( 3u - i ) * 4u );
					++it;
				}
				PushCharToUTF8String( char_code, result.text );
			}
			break;

		default:
			out_errors.emplace_back( std::string("invalid escape sequence: \\") + char(*it), src_loc );
			return result;
		};
	}
	else
		PushCharToUTF8String( ReadNextUTF8Char( it, it_end ), result.text );

	if( it == it_end )
	{
		out_errors.emplace_back( "unexpected end of file at char literal", src_loc );
		return result;
	}
	if( *it != '\'' )
	{
		out_errors.emplace_back( "expected ' at end of char literal", src_loc );
		return result;
	}
	++it;

	return result;
}

Lexem ParseStringImpl( Iterator& it, const Iterator it_end, const SrcLoc& src_loc, LexSyntErrors& out_errors )
{
	U_ASSERT( *it == '"' );
	++it;

	Lexem result;
	result.type= Lexem::Type::String;

	while(true)
	{
		if( it == it_end )
			break;
		else if( *it == '"' )
		{
			++it;
			break;
		}
		else if( ( /* *it >= 0x00u && */ sprache_char(*it) < 0x20u ) || *it == 0x7F ) // TODO - is this correct control character?
		{
			out_errors.emplace_back( "control character inside string", src_loc );
			return result;
		}
		else if( *it == '\\' )
		{
			++it;
			if( it == it_end )
				return result;
			switch( *it )
			{
			case '"':
			case '\'':
			case '\\':
			case '/':
				result.text.push_back(*it);
				++it;
				break;

			case 'b': result.text.push_back('\b'); ++it; break;
			case 'f': result.text.push_back('\f'); ++it; break;
			case 'n': result.text.push_back('\n'); ++it; break;
			case 'r': result.text.push_back('\r'); ++it; break;
			case 't': result.text.push_back('\t'); ++it; break;
			case '0': result.text.push_back('\0'); ++it; break;

			case 'u':
				{
					// Parse hex number.
					++it;
					if( it_end - it < 4 )
					{
						out_errors.emplace_back( "expected 4 hex digits", src_loc );
						return result;
					}

					sprache_char char_code= 0u;
					for( size_t i= 0u; i < 4u; i++ )
					{
						sprache_char digit;
							 if( *it >= '0' && *it <= '9' ) digit= uint32_t( *it - '0' );
						else if( *it >= 'a' && *it <= 'f' ) digit= uint32_t( *it - 'a' + 10 );
						else if( *it >= 'A' && *it <= 'F' ) digit= uint32_t( *it - 'A' + 10 );
						else
						{
							out_errors.emplace_back( "expected hex number", src_loc );
							return result;
						}
						char_code|= digit << ( ( 3u - i ) * 4u );
						++it;
					}
					PushCharToUTF8String( char_code, result.text );
				}
				break;

			default:
				out_errors.emplace_back( std::string("invalid escape sequence: \\") + char(*it), src_loc );
				return result;
			};
		}
		else
		{
			result.text.push_back(*it);
			++it;
		}
	} // while true

	return result;
}

Lexem ParseIdentifier( Iterator& it, const Iterator it_end )
{
	Lexem result;
	result.type= Lexem::Type::Identifier;

	while( it < it_end )
	{
		auto it_next= it;
		if( !IsIdentifierChar( ReadNextUTF8Char( it_next, it_end ) ) )
			break;

		result.text.insert( result.text.end(), it, it_next );
		it= it_next;
	}

	return result;
}

bool IsMacroIdentifierStartChar( const sprache_char c )
{
	return c == '?';
}

Lexem ParseMacroIdentifier( Iterator& it, const Iterator it_end )
{
	Lexem result;
	result.type= Lexem::Type::MacroIdentifier;

	U_ASSERT(IsMacroIdentifierStartChar(sprache_char(*it)));
	result.text.push_back(*it);
	++it;

	if( it < it_end && IsMacroIdentifierStartChar(sprache_char(*it)) )
	{
		result.type= Lexem::Type::MacroUniqueIdentifier;
		result.text.push_back(*it);
		++it;
	}

	while( it < it_end )
	{
		auto it_next= it;
		if( !IsIdentifierChar( ReadNextUTF8Char( it_next, it_end ) ) )
			break;

		result.text.insert( result.text.end(), it, it_next );
		it= it_next;
	}

	return result;
}

double PowI( const uint64_t base, const uint64_t pow )
{
	double res= 1.0, p= double(base);

	for( uint64_t i= 1; ; i <<= 1, p*= p )
	{
		if( (i & pow ) != 0 )
			res*= p;
		if( i >= pow )
			break;
	}

	return res;
}

template<uint32_t base>
uint32_t TryParseDigit( const char c )
{
	if constexpr( base == 2 )
	{
		if( c >= '0' && c <= '1' )
			return uint32_t(c - '0');
	}
	else if constexpr( base == 8 )
	{
		if( c >= '0' && c <= '7' )
			return uint32_t(c - '0');
	}
	else if constexpr( base == 10 )
	{
		if( c >= '0' && c <= '9' )
			return uint32_t(c - '0');
	}
	else if constexpr( base == 16 )
	{
		if( c >= '0' && c <= '9' )
			return uint32_t(c - '0');
		else if( c >= 'a' && c <= 'f' )
			return uint32_t(c - 'a' + 10);
		else if( c >= 'A' && c <= 'F' )
			return uint32_t(c - 'A' + 10);
	}

	return uint32_t(-1);
}

std::array<char, 8> TryParseNumericLexemTypeSuffix( Iterator& it, const Iterator it_end, SrcLoc src_loc, LexSyntErrors& out_errors )
{
	std::array<char, 8> res{};
	if( it != it_end && IsIdentifierStartChar( GetUTF8FirstChar( it, it_end ) ) )
	{
		const Lexem type_suffix= ParseIdentifier( it, it_end );
		if( type_suffix.text.size() >= sizeof(res) )
			out_errors.emplace_back( "Type suffix of numeric literal is too long", src_loc );

		std::memcpy( res.data(), type_suffix.text.data(), std::min( type_suffix.text.size(), sizeof(res.size()) ) );
	}

	return res;
}

Lexem ContinueParsingFloatingPointNumber( const double parsed_part, Iterator& it, const Iterator it_end, SrcLoc src_loc, LexSyntErrors& out_errors )
{
	double value= parsed_part;

	// Integer part.
	while( it < it_end )
	{
		const uint32_t num= TryParseDigit<10>( *it );
		if( num == uint32_t(-1) )
			break;

		value= std::fma( value, 10.0, double(num) );
		++it;

	}

	int32_t num_fractional_digits= 0;

	// Fractional part.
	if( it < it_end && *it == '.' )
	{
		++it;

		while( it < it_end )
		{
			const uint32_t num= TryParseDigit<10>( *it );
			if( num == uint32_t(-1) )
				break;

			++num_fractional_digits;
			value= std::fma( value, 10.0, double(num) );
			++it;
		}
	}

	int32_t exponent= 0;

	// Exponent
	if( it < it_end && *it == 'e' )
	{
		++it;

		bool is_negative= false;

		if( it < it_end && *it == '-' )
		{
			is_negative= true;
			++it;
		}
		else if( it < it_end && *it == '+' )
			++it;

		while( it < it_end )
		{
			const uint32_t num= TryParseDigit<10>( *it );
			if( num == uint32_t(-1) )
				break;

			exponent= exponent * 10 + int32_t(num);
			++it;

			if( exponent > 2048 )
			{
				// Do not allow too large exponents.
				out_errors.emplace_back( "Floating point number exponent overflow", src_loc );
				break;
			}
		}
		if( is_negative )
			exponent= -exponent;
	}

	FloatingPointNumberLexemData result;

	// TODO - check no precision lost happens here.

	if( exponent >= num_fractional_digits )
		result.value= value * PowI( 10u, uint64_t( exponent - num_fractional_digits ) );
	else
		result.value= value / PowI( 10u, uint64_t( num_fractional_digits - exponent ) );

	result.type_suffix= TryParseNumericLexemTypeSuffix( it, it_end, src_loc, out_errors );

	Lexem result_lexem;
	result_lexem.type= Lexem::Type::FloatingPointNumber;
	result_lexem.text.resize( sizeof(FloatingPointNumberLexemData) );
	std::memcpy( result_lexem.text.data(), &result, sizeof(FloatingPointNumberLexemData) );
	return result_lexem;
}

Lexem ParseDecimalNumber( Iterator& it, const Iterator it_end, SrcLoc src_loc, LexSyntErrors& out_errors )
{
	uint64_t value= 0u;

	while( it < it_end )
	{
		const uint32_t num= TryParseDigit<10>( *it );
		if( num == uint32_t(-1) )
			break;

		const uint64_t new_value= value * 10 + num;
		++it;

		if( new_value < value ) // Check overflow
		{
			// Continue parsing as float in case of overflow.
			// TODO - ensure no precision lost happens in this case.
			const double parsed_part= std::fma( double(value), 10.0, double(num) );
			return ContinueParsingFloatingPointNumber( parsed_part, it, it_end, src_loc, out_errors );
		}
		else
			value= new_value;
	}

	if( it < it_end )
	{
		// If we have decimal point or exponent - parse as float.
		if( *it == '.' || *it == 'e' )
			return ContinueParsingFloatingPointNumber( double(value), it, it_end, src_loc, out_errors );
	}

	IntegerNumberLexemData result;
	result.value= value;

	result.type_suffix= TryParseNumericLexemTypeSuffix( it, it_end, src_loc, out_errors );

	Lexem result_lexem;
	result_lexem.type= Lexem::Type::IntegerNumber;
	result_lexem.text.resize( sizeof(IntegerNumberLexemData) );
	std::memcpy( result_lexem.text.data(), &result, sizeof(IntegerNumberLexemData) );
	return result_lexem;
}

// Initial prefix should be skipped before this call.
template<uint32_t base>
Lexem ParseIntegerNumberImpl( Iterator& it, const Iterator it_end, SrcLoc src_loc, LexSyntErrors& out_errors )
{
	uint64_t value= 0u;

	// Require at least one digit.
	if( it == it_end )
		out_errors.emplace_back( "Unexpected end of number", src_loc );
	else
	{
		value= TryParseDigit<base>( *it );
		++it;
		if( value == uint64_t(-1) )
			out_errors.emplace_back( "Unexpected end of number", src_loc );
	}

	while( it < it_end )
	{
		const uint32_t num= TryParseDigit<base>( *it );
		if( num == uint32_t(-1) )
			break;

		const uint64_t new_value= value * base + num;
		++it;

		if( new_value < value ) // Check overflow
		{
			out_errors.emplace_back( "Integer part of numeric literal is too long", src_loc );
			break;
		}
		else
			value= new_value;
	}

	IntegerNumberLexemData result;
	result.value= value;

	result.type_suffix= TryParseNumericLexemTypeSuffix( it, it_end, src_loc, out_errors );

	Lexem result_lexem;
	result_lexem.type= Lexem::Type::IntegerNumber;
	result_lexem.text.resize( sizeof(IntegerNumberLexemData) );
	std::memcpy( result_lexem.text.data(), &result, sizeof(IntegerNumberLexemData) );
	return result_lexem;
}

Lexem ParseNumber( Iterator& it, const Iterator it_end, SrcLoc src_loc, LexSyntErrors& out_errors )
{
	if( *it == '0' && std::next(it) < it_end )
	{
		const char d= *std::next(it);
		switch(d)
		{
		case 'b':
			it+= 2;
			return ParseIntegerNumberImpl<2>( it, it_end, src_loc, out_errors );
		case 'o':
			it+= 2;
			return ParseIntegerNumberImpl<8>( it, it_end, src_loc, out_errors );
		case 'x':
			it+= 2;
			return ParseIntegerNumberImpl<16>( it, it_end, src_loc, out_errors );
		};
	}

	return ParseDecimalNumber( it, it_end, src_loc, out_errors );
}

} // namespace

LexicalAnalysisResult LexicalAnalysis( const std::string_view program_text )
{
	LexicalAnalysisResult result;

	Iterator it= program_text.data();
	const Iterator it_end= it + program_text.size();

	if( program_text.size() >= 3u && program_text.substr(0, 3) == "\xEF\xBB\xBF" )
		it+= 3; // Skip UTF-8 byte order mark.


	uint32_t line= 1; // Count lines from "1", in human-readable format.
	uint32_t column= 0u;
	uint32_t max_column= 0u;

	std::string fixed_lexem_str;
	while( it < it_end )
	{
		auto it_prev= it;
		const auto advance_column=
		[&]
		{
			while( it_prev < it )
			{
				// TODO - maybe count UTF-8 chars instead of code points?
				ReadNextUTF8Char( it_prev, it );
				++column;
				max_column= std::max( max_column, column );
			}
		};

		const sprache_char c= GetUTF8FirstChar( it, it_end );
		Lexem lexem;

		// line comment.
		if( c == '/' && it_end - it > 1 && *(it+1) == '/' )
		{
			// Read all until new line, but do not extract new line symbol itself.
			while( it < it_end )
			{
				auto it_copy= it;
				const sprache_char c= ReadNextUTF8Char( it_copy, it_end );
				if( IsNewline(c) )
					break;
				it= it_copy;
			}

			continue;
		}
		// Multiline comment.
		if( c == '/' && it + 1 < it_end && *std::next(it) == '*' )
		{
			int comments_depth= 1;
			it+= 2;
			column+= 2u;

			while( it < it_end )
			{
				if( it + 1 < it_end && *it == '*' && *std::next(it) == '/' )
				{
					it+= 2;
					column+= 2u;
					max_column= std::max( max_column, column );
					--comments_depth;
					if( comments_depth == 0 )
						break;
				}
				else if( it + 1 < it_end && *it == '/' && *std::next(it) == '*' )
				{
					it+= 2;
					column+= 2u;
					max_column= std::max( max_column, column );
					++comments_depth;
				}
				else
				{
					const sprache_char c= sprache_char(*it);
					if( IsNewline( c ) )
					{
						++line;
						column= 0;
						++it;
						// Handle case with two-symbol line ending.
						if( it < it_end )
						{
							auto it_copy= it;
							if( IsNewlineSequence( c, ReadNextUTF8Char( it_copy, it ) ) )
								it= it_copy;
						}
					}
					else
						ReadNextUTF8Char( it, it_end );
					++column;
					max_column= std::max( max_column, column );
				}
			}

			if( comments_depth != 0 )
				result.errors.emplace_back( "Lexical error: expected */", SrcLoc( 0u, line, column ) );
			continue;
		}
		else if( IsNewline(c) )
		{
			++line;
			column= 0u;

			ReadNextUTF8Char( it, it_end ); // Consume this line ending symbol.

			// Handle case with two-symbol line ending.
			if( it < it_end )
			{
				auto it_copy= it;
				const sprache_char next_c= ReadNextUTF8Char( it_copy, it_end );
				if( IsNewlineSequence( c, next_c ) )
					it= it_copy;
			}

			continue;
		}
		else if( IsWhitespace(c) )
		{
			++it;
			++column;
			continue;
		}
		else if( c == '"' )
		{
			lexem= ParseStringImpl( it, it_end, SrcLoc( 0u, line, column ), result.errors );
			if( IsIdentifierStartChar( GetUTF8FirstChar( it, it_end ) ) )
			{
				// Parse string suffix.
				lexem.src_loc= SrcLoc( 0u, line, column );

				advance_column();
				result.lexems.push_back( std::move(lexem) );

				lexem= ParseIdentifier( it, it_end );
				lexem.type= Lexem::Type::LiteralSuffix;
			}
		}
		else if( c == '\'' )
		{
			lexem= ParseCharLiteral( it, it_end, SrcLoc( 0u, line, column ), result.errors );
			if( IsIdentifierStartChar( GetUTF8FirstChar( it, it_end ) ) )
			{
				// Parse string suffix.
				lexem.src_loc= SrcLoc( 0u, line, column );

				advance_column();
				result.lexems.push_back( std::move(lexem) );

				lexem= ParseIdentifier( it, it_end );
				lexem.type= Lexem::Type::LiteralSuffix;
			}
		}
		else if( IsNumberStartChar(c) )
			lexem= ParseNumber( it, it_end, SrcLoc( 0u, line, column ), result.errors );
		else if( IsIdentifierStartChar(c) )
			lexem= ParseIdentifier( it, it_end );
		else if( IsMacroIdentifierStartChar(c) &&
				std::next(it) < it_end &&
				*std::next(it) != '>' &&
				( IsIdentifierChar(GetUTF8FirstChar(std::next(it), it_end)) || IsMacroIdentifierStartChar(GetUTF8FirstChar(std::next(it), it_end)) ) )
			lexem= ParseMacroIdentifier( it, it_end );
		else
		{
			size_t s= std::min( g_max_fixed_lexem_size, size_t( it_end - it ) );
			fixed_lexem_str.clear();
			fixed_lexem_str.insert( fixed_lexem_str.end(), it, it + s );
			for( ; s >= 1u; --s )
			{
				const FixedLexemsMap& m= g_fixed_lexems[s];

				const auto lexem_it= m.find( fixed_lexem_str );
				if( lexem_it != m.end() )
				{
					it+= s;
					lexem.type= lexem_it->second;
					// It's not expensive to store lexem text, since std::string (usually) has small string optimization - without heap usage.
					// So, store text even for fixed lexems in order to simplify working with parsed lexems.
					lexem.text= fixed_lexem_str;
					break;
				}
				fixed_lexem_str.pop_back();
			}

			if( lexem.type == Lexem::Type::None )
			{
				result.errors.emplace_back(
					"Lexical error: unrecognized character: " + std::to_string(c),
					SrcLoc( 0u, line, column ) );
				++it;
				continue;
			}
		}

		lexem.src_loc= SrcLoc( 0u, line, column );

		advance_column();

		result.lexems.push_back( std::move(lexem) );
	} // while not end

	Lexem eof_lexem;
	eof_lexem.type= Lexem::Type::EndOfFile;
	eof_lexem.text= "EOF";
	eof_lexem.src_loc= SrcLoc( 0u, line, column );

	result.lexems.emplace_back( std::move(eof_lexem) );

	if( line > SrcLoc::c_max_line )
	{
		result.errors.emplace_back(
			"Lexical error: line limit reached, max is " + std::to_string( SrcLoc::c_max_line ),
			SrcLoc( 0u, line, column ) );
	}
	if( max_column > SrcLoc::c_max_column )
	{
		result.errors.emplace_back(
			"Lexical error: column limit reached, max is " + std::to_string( SrcLoc::c_max_column ),
			SrcLoc( 0u, line, column ) );
	}

	return result;
}

bool IsValidIdentifier( const std::string_view text )
{
	Iterator it= text.data();
	const Iterator it_end= it + text.size();

	// Skip starting whitespaces.
	while( it < it_end )
	{
		const auto c= GetUTF8FirstChar( it, it_end );
		if( !IsWhitespace( c ) )
			break;
		ReadNextUTF8Char( it, it_end );
	}

	if( it == it_end )
		return false;

	// Read start.
	const auto start= GetUTF8FirstChar( it, it_end );
	if( !IsIdentifierStartChar( start ) )
		return false;

	// Try to parse identifier.
	ParseIdentifier( it, it_end );

	// Skip trailing whitespaces.
	while( it < it_end )
	{
		const auto c= GetUTF8FirstChar( it, it_end );
		if( !IsWhitespace( c ) )
			break;
		ReadNextUTF8Char( it, it_end );
	}

	// Is valid if nothing left.
	return it == it_end;
}

LineToLinearPositionIndex BuildLineToLinearPositionIndex( const std::string_view text )
{
	LineToLinearPositionIndex result;
	BuildLineToLinearPositionIndex( text, result );

	return result;
}

void BuildLineToLinearPositionIndex( std::string_view text, LineToLinearPositionIndex& out_index )
{
	out_index.clear();
	out_index.push_back(0);
	out_index.push_back(0);

	const char* const it_start= text.data();
	const char* const it_end= text.data() + text.size();
	const char* it= it_start;
	while( it < it_end )
	{
		const sprache_char c= ReadNextUTF8Char( it, it_end );
		if( IsNewline( c ) )
		{
			// Handle cases with two-symbol line ending.
			if( it < it_end )
			{
				auto it_copy= it;
				const sprache_char next_c= ReadNextUTF8Char( it_copy, it_end );
				if( IsNewlineSequence( c, next_c ) )
					it= it_copy;
			}

			out_index.push_back( TextLinearPosition(it - it_start) ); // Next line starts with next symbol.
		}
	}
}

uint32_t LinearPositionToLine( const LineToLinearPositionIndex& index, const TextLinearPosition position )
{
	U_ASSERT( index.size() >= 2 ); // Should contains at least dummy and first line.

	// Use binary search to find line number.
	const auto it= std::upper_bound( index.begin(), index.end(), position );

	if( it == index.begin() )
	{
		// WTF?
		return 1;
	}
	const auto prev_it= std::prev(it);

	return  uint32_t( size_t( prev_it - index.begin() ) );
}

std::optional<TextLinearPosition> GetIdentifierStartForPosition( const std::string_view text, const TextLinearPosition position )
{
	if( position >= text.size() )
		return std::nullopt;

	if( !IsIdentifierChar( GetUTF8FirstChar( text.data() + position, text.data() + text.size() ) ) )
		return std::nullopt; // Not an identifier.

	// Go backward until find non-identifier char.
	TextLinearPosition current_position= position;
	while( current_position > 0 )
	{
		const char c= text[ current_position - 1 ];
		if( ( c & 0b10000000 ) == 0 )
		{
			if( !IsIdentifierChar( sprache_char(c) ) )
				break;
			--current_position;
		}
		else
		{
			// Read multibyte sequence backwards.
			TextLinearPosition code_point_start_position= current_position - 1;
			while(true)
			{
				if( ( text[ code_point_start_position ] & 0b11000000 ) == 0b10000000 )
				{
					// Auxilarity byte.
					if( code_point_start_position == 0 )
						return std::nullopt; // Broken UTF-8.
					--code_point_start_position;
				}
				else
					break;
			}
			if( !IsIdentifierChar( GetUTF8FirstChar( text.data() + code_point_start_position, text.data() + text.size() ) ) )
				break;
			current_position= code_point_start_position;
		}
	}

	return current_position;
}

std::optional<TextLinearPosition> GetIdentifierEndForPosition( const std::string_view text, const TextLinearPosition position )
{
	if( position >= text.size() )
		return std::nullopt;

	const char* s= text.data() + position;
	const char* const s_end= text.data() + text.size();
	while( s < s_end )
	{
		const char* s_copy= s;
		const sprache_char c= ReadNextUTF8Char( s_copy, s_end );
		if( !IsIdentifierChar(c) )
			break;
		s= s_copy;
	}

	const TextLinearPosition end_position= TextLinearPosition(s - text.data());
	if( end_position == position )
		return std::nullopt; // Not an identifier.

	return end_position;
}

std::optional<std::string> ParseStringLiteral( const std::string_view text )
{
	if( text.empty() || text.front() != '"' )
		return std::nullopt;

	const auto it_end= text.data() + text.size();
	auto it= text.data();

	LexSyntErrors errors;
	Lexem string_lexem= ParseStringImpl( it, it_end, SrcLoc(), errors );
	if( !errors.empty() )
		return std::nullopt;
	return std::move( string_lexem.text );
}

} // namespace U
