#include <algorithm>
#include <cstring>

#include "assert.hpp"
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

		{ "=", Lexem::Type::Assignment },
		{ "+", Lexem::Type::Plus },
		{ "-", Lexem::Type::Minus },
		{ "*", Lexem::Type::Star },
		{ "/", Lexem::Type::Slash },
		{ "%", Lexem::Type::Percent },

		{ "<", Lexem::Type::CompareLess },
		{ ">", Lexem::Type::CompareGreater },

		{ "&", Lexem::Type::And },
		{ "|", Lexem::Type::Or },
		{ "^", Lexem::Type::Xor },
		{ "~", Lexem::Type::Tilda },
		{ "!", Lexem::Type::Not },

		{ "'", Lexem::Type::Apostrophe },
	},
	FixedLexemsMap
	{ // Two symbol lexems.
		{ "</", Lexem::Type::TemplateBracketLeft  },
		{ "/>", Lexem::Type::TemplateBracketRight },

		{ "<?", Lexem::Type::MacroBracketLeft  },
		{ "?>", Lexem::Type::MacroBracketRight },

		{ "::", Lexem::Type::Scope },

		{ "++", Lexem::Type::Increment },
		{ "--", Lexem::Type::Decrement },

		{ "==", Lexem::Type::CompareEqual },
		{ "!=", Lexem::Type::CompareNotEqual },
		{ "<=", Lexem::Type::CompareLessOrEqual },
		{ ">=", Lexem::Type::CompareGreaterOrEqual },

		{ "&&", Lexem::Type::Conjunction },
		{ "||", Lexem::Type::Disjunction },

		{ "+=", Lexem::Type::AssignAdd },
		{ "-=", Lexem::Type::AssignSub },
		{ "*=", Lexem::Type::AssignMul },
		{ "/=", Lexem::Type::AssignDiv },
		{ "%=", Lexem::Type::AssignRem },
		{ "&=", Lexem::Type::AssignAnd },
		{ "|=", Lexem::Type::AssignOr  },
		{ "^=", Lexem::Type::AssignXor },

		{ "<<", Lexem::Type::ShiftLeft  },
		{ ">>", Lexem::Type::ShiftRight },

		{ "<-", Lexem::Type::LeftArrow  },
		{ "->", Lexem::Type::RightArrow },
	},
	FixedLexemsMap
	{ // Three symbol lexems.
		{ "<<=", Lexem::Type::AssignShiftLeft  },
		{ ">>=", Lexem::Type::AssignShiftRight },
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
	return c == '\n';
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

Lexem ParseString( Iterator& it, const Iterator it_end, const FilePos& file_pos, LexSyntErrors& out_errors )
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
			out_errors.emplace_back( "control character inside string", file_pos );
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
						out_errors.emplace_back( "expected 4 hex digits", file_pos );
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
							out_errors.emplace_back( "expected hex number", file_pos );
							return result;
						}
						char_code|= digit << ( ( 3u - i ) * 4u );
						++it;
					}
					PushCharToUTF8String( char_code, result.text );
				}
				break;

			default:
				out_errors.emplace_back( std::string("invalid escape sequence: \\") + char(*it), file_pos );
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
	if( pow == 0u )
		return 1.0;
	if( pow == 1u )
		return double(base);
	if( pow == 2u )
		return double(base * base);

	const uint64_t half_pow= pow / 2u;
	double res= PowI( base, half_pow );
	res= res * res;
	if( half_pow * 2u != pow )
		res*= double(base);
	return res;
}

Lexem ParseNumber( Iterator& it, const Iterator it_end, FilePos file_pos, LexSyntErrors& out_errors )
{
	uint64_t base= 10u;
	// Returns -1 for non-numbers
	uint64_t (*number_func)(char) =
		[]( const char c ) -> uint64_t
		{
			if( c >= '0' && c <= '9' )
				return uint64_t(c - '0');
			return uint64_t(-1);
		};

	if( *it == '0' && std::next(it) < it_end )
	{
		const char d= *std::next(it);
		switch(d)
		{
		case 'b':
			it+= 2;
			base= 2;
			number_func=
				[]( const char c ) -> uint64_t
				{
					if( c >= '0' && c <= '1' )
						return uint64_t(c - '0');
					return uint64_t(-1);
				};
			break;

		case 'o':
			it+= 2;
			base= 8;
			number_func=
				[]( const char c ) -> uint64_t
				{
					if( c >= '0' && c <= '7' )
						return uint64_t(c - '0');
					return uint64_t(-1);
				};
			break;

		case 'x':
			it+= 2;
			base= 16;
			number_func=
				[]( const char c ) -> uint64_t
				{
					if( c >= '0' && c <= '9' )
						return uint64_t(c - '0');
					else if( c >= 'a' && c <= 'f' )
						return uint64_t(c - 'a' + 10);
					else if( c >= 'A' && c <= 'F' )
						return uint64_t(c - 'A' + 10);
					else
						return uint64_t(-1);
				};
			break;

		default:
			break;
		};
	}

	uint64_t integer_part= 0, fractional_part= 0;
	int fractional_part_digits= 0, exponent= 0;
	bool has_fraction_point= false;

	while( it < it_end )
	{
		const uint64_t num= number_func( *it );
		if( num == uint64_t(-1) )
			break;

		const uint64_t integer_part_before= integer_part;
		integer_part= integer_part * base + num;
		++it;

		if( integer_part < integer_part_before ) // Check overflow
		{
			out_errors.emplace_back( "Integer part of numeric literal is too long", file_pos );
			break;
		}
	}

	if( it < it_end && *it == '.' )
	{
		++it;
		has_fraction_point= true;

		while( it < it_end )
		{
			const uint64_t num= number_func( *it );
			if( num == uint64_t(-1) )
				break;

			const uint64_t fractional_part_before= fractional_part;
			fractional_part= fractional_part * base + num;
			++fractional_part_digits;
			++it;

			if( fractional_part < fractional_part_before ) // Check overflow
			{
				out_errors.emplace_back( "Fractional part of numeric literal is too long", file_pos );
				break;
			}
		}
	}

	// Exponent
	if( base == 10u && it < it_end && *it == 'e' )
	{
		++it;

		U_ASSERT( base == 10 );
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
			const uint64_t num= number_func( *it );
			if( num == uint64_t(-1) )
				break;

			exponent= exponent * int(base) + int(num);
			++it;
		}
		if( is_negative )
			exponent= -exponent;
	}

	NumberLexemData result;

	// For double calculate only powers > 0, because pow( base, positive ) is always integer and have exact double representation.
	// pow( base, negative ) may have not exact double representation (1/10 for example).
	// Example:
	// 3 / 10 - right
	// 3 * (1/10) - wrong
	if( exponent >= 0 )
		result.value_double= double(integer_part) * PowI( base, uint64_t(exponent) );
	else
		result.value_double= double(integer_part) / PowI( base, uint64_t(-exponent) );
	if( exponent >= fractional_part_digits )
		result.value_double+= double(fractional_part) * PowI( base, uint64_t( exponent - fractional_part_digits ) );
	else
		result.value_double+= double(fractional_part) / PowI( base, uint64_t( fractional_part_digits - exponent ) );

	result.value_int= integer_part;
	for( int i= 0; i < exponent; ++i )
		result.value_int*= base;
	for( int i= 0; i < -exponent; ++i )
		result.value_int/= base;

	uint64_t fractional_part_corrected= fractional_part;
	for( int i= 0; i < exponent - fractional_part_digits; ++i )
		fractional_part_corrected*= base;
	for( int i= 0; i < fractional_part_digits - exponent; ++i )
		fractional_part_corrected/= base;
	result.value_int+= fractional_part_corrected;

	result.has_fractional_point= has_fraction_point;

	if( IsIdentifierStartChar( GetUTF8FirstChar( it, it_end ) ) )
	{
		const Lexem type_suffix= ParseIdentifier( it, it_end );
		if( type_suffix.text.size() >= sizeof(result.type_suffix) )
			out_errors.emplace_back( "Type suffix of numeric literal is too long", file_pos );

		std::memcpy( result.type_suffix.data(), type_suffix.text.data(), std::min( type_suffix.text.size(), sizeof(result.type_suffix) ) );
	}

	Lexem result_lexem;
	result_lexem.type= Lexem::Type::Number;
	result_lexem.text.resize( sizeof(NumberLexemData) );
	std::memcpy( result_lexem.text.data(), &result, sizeof(NumberLexemData) );
	return result_lexem;
}

} // namespace

bool operator==(const Lexem& l, const Lexem& r )
{
	return l.text == r.text && l.file_pos == r.file_pos && l.type == r.type;
}

bool operator!=(const Lexem& l, const Lexem& r )
{
	return !(l == r );
}

LexicalAnalysisResult LexicalAnalysis( const std::string& program_text, const bool collect_comments )
{
	return LexicalAnalysis( program_text.data(), program_text.size(), collect_comments );
}

LexicalAnalysisResult LexicalAnalysis( const char* const program_text_data, const size_t program_text_size, const bool collect_comments )
{
	LexicalAnalysisResult result;

	Iterator it= program_text_data;
	const Iterator it_end= program_text_data + program_text_size;

	int comments_depth= 0;

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
				ReadNextUTF8Char( it_prev, it );
				++column;
				max_column= std::max( max_column, column );
			}
		};

		const uint32_t c= GetUTF8FirstChar( it, it_end );
		Lexem lexem;

		// line comment.
		if( c == '/' && it_end - it > 1 && *(it+1) == '/' )
		{
			if( collect_comments )
			{
				Lexem comment_lexem;
				comment_lexem.file_pos= FilePos( 0u, line, column );
				comment_lexem.type= Lexem::Type::Comment;

				while( it < it_end && !IsNewline(sprache_char(*it)) )
				{
					comment_lexem.text.push_back(*it);
					++it;
				}
				advance_column();
				result.lexems.emplace_back( std::move(comment_lexem) );
			}
			else
				while( it < it_end && !IsNewline(sprache_char(*it)) ) ++it;

			if( it == it_end ) break;

			++line;
			++it;
			column= 0u;
			continue;
		}
		if( c == '/' && it_end - it > 1 && *std::next(it) == '*' )
		{
			++comments_depth;
			if( collect_comments )
			{
				Lexem comment_lexem;
				comment_lexem.file_pos= FilePos( 0u, line, column );
				comment_lexem.type= Lexem::Type::Comment;
				comment_lexem.text= "/*";
				advance_column();
				result.lexems.emplace_back( std::move(comment_lexem) );
			}
			it+= 2;
			column+= 2u;
			continue;
		}
		if( c == '*' && it_end - it > 1 && *(it+1) == '/' )
		{
			--comments_depth;
			if( collect_comments )
			{
				Lexem comment_lexem;
				comment_lexem.file_pos= FilePos( 0u, line, column );
				comment_lexem.type= Lexem::Type::Comment;
				comment_lexem.text= "*/";
				advance_column();
				result.lexems.push_back( std::move(comment_lexem) );
			}
			else if( comments_depth < 0 )
				result.errors.emplace_back( "Lexical error: unexpected */", FilePos( 0u, line, column ) );
			it+= 2;
			column+= 2u;
			continue;
		}
		else if( IsNewline(c) )
		{
			++line;
			++it;
			column= 0u;
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
			lexem= ParseString( it, it_end, FilePos( 0u, line, column ), result.errors );
			if( IsIdentifierStartChar( GetUTF8FirstChar( it, it_end ) ) )
			{
				// Parse string suffix.
				lexem.file_pos= FilePos( 0u, line, column );

				advance_column();
				if( comments_depth == 0 || collect_comments )
					result.lexems.push_back( std::move(lexem) );

				lexem= ParseIdentifier( it, it_end );
				lexem.type= Lexem::Type::LiteralSuffix;
			}
		}
		else if( IsNumberStartChar(c) )
			lexem= ParseNumber( it, it_end, FilePos( 0u, line, column ), result.errors );
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
					lexem.text= fixed_lexem_str;
					goto push_lexem;
				}
				fixed_lexem_str.pop_back();
			}

			if( comments_depth == 0 )
				result.errors.emplace_back(
					"Lexical error: unrecognized character: " + std::to_string(c),
					FilePos( 0u, line, column ) );
			++it;
			continue;
		}

	push_lexem:
		lexem.file_pos= FilePos( 0u, line, column );

		advance_column();

		if( !( comments_depth != 0 && !collect_comments ) )
			result.lexems.push_back( std::move(lexem) );
	} // while not end

	if( !collect_comments )
		for( int i= 0; i < comments_depth; ++i )
			result.errors.emplace_back( "Lexical error: expected */", FilePos( 0u, line, column ) );

	Lexem eof_lexem;
	eof_lexem.type= Lexem::Type::EndOfFile;
	eof_lexem.text= "EOF";
	eof_lexem.file_pos= FilePos( 0u, line, column );

	result.lexems.emplace_back( std::move(eof_lexem) );

	if( line > FilePos::c_max_line )
	{
		result.errors.emplace_back(
			"Lexical error: line limit reached, max is " + std::to_string( FilePos::c_max_line ),
			FilePos( 0u, line, column ) );
	}
	if( max_column > FilePos::c_max_column )
	{
		result.errors.emplace_back(
			"Lexical error: column limit reached, max is " + std::to_string( FilePos::c_max_column ),
			FilePos( 0u, line, column ) );
	}

	return result;
}

} // namespace U
