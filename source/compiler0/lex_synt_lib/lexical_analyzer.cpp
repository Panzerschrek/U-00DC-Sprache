#include <algorithm>
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
		{ "@", Lexem::Type::At },

		{ "$", Lexem::Type::PointerTypeMark },
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

		{ "->", Lexem::Type::RightArrow },

		{ "$<", Lexem::Type::ReferenceToPointer },
		{ "$>", Lexem::Type::PointerToReference },
	},
	FixedLexemsMap
	{ // Three symbol lexems.
		{ "<=>", Lexem::Type::CompareOrder },
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

Lexem ParseNumber( Iterator& it, const Iterator it_end, SrcLoc src_loc, LexSyntErrors& out_errors )
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
			out_errors.emplace_back( "Integer part of numeric literal is too long", src_loc );
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
				out_errors.emplace_back( "Fractional part of numeric literal is too long", src_loc );
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

	// For double calculate only powers > 0, because pow( base, positive ) is always integer and has exact double representation.
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

	if( it != it_end && IsIdentifierStartChar( GetUTF8FirstChar( it, it_end ) ) )
	{
		const Lexem type_suffix= ParseIdentifier( it, it_end );
		if( type_suffix.text.size() >= sizeof(result.type_suffix) )
			out_errors.emplace_back( "Type suffix of numeric literal is too long", src_loc );

		std::memcpy( result.type_suffix.data(), type_suffix.text.data(), std::min( type_suffix.text.size(), sizeof(result.type_suffix) ) );
	}

	Lexem result_lexem;
	result_lexem.type= Lexem::Type::Number;
	result_lexem.text.resize( sizeof(NumberLexemData) );
	std::memcpy( result_lexem.text.data(), &result, sizeof(NumberLexemData) );
	return result_lexem;
}

} // namespace

LexicalAnalysisResult LexicalAnalysis( const std::string_view program_text, const bool collect_comments )
{
	LexicalAnalysisResult result;

	Iterator it= program_text.data();
	const Iterator it_end= it + program_text.size();

	if( program_text.size() >= 3u && program_text.substr(0, 3) == "\xEF\xBB\xBF" )
		it+= 3; // Skip UTF-8 byte order mark.

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
			const auto comment_start_it= it;

			// Read all until new line, but do not extract new line symbol itself.
			while( it < it_end )
			{
				auto it_copy= it;
				const sprache_char c= ReadNextUTF8Char( it_copy, it_end );
				if( IsNewline(c) )
					break;
				it= it_copy;
			}

			if( collect_comments )
			{
				Lexem comment_lexem;
				comment_lexem.src_loc= SrcLoc( 0u, line, column );
				comment_lexem.type= Lexem::Type::Comment;
				comment_lexem.text.insert( comment_lexem.text.end(), comment_start_it, it );
				result.lexems.emplace_back( std::move(comment_lexem) );
			}

			continue;
		}
		if( c == '/' && it_end - it > 1 && *std::next(it) == '*' )
		{
			++comments_depth;
			if( collect_comments )
			{
				Lexem comment_lexem;
				comment_lexem.src_loc= SrcLoc( 0u, line, column );
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
				comment_lexem.src_loc= SrcLoc( 0u, line, column );
				comment_lexem.type= Lexem::Type::Comment;
				comment_lexem.text= "*/";
				advance_column();
				result.lexems.push_back( std::move(comment_lexem) );
			}
			else if( comments_depth < 0 )
				result.errors.emplace_back( "Lexical error: unexpected */", SrcLoc( 0u, line, column ) );
			it+= 2;
			column+= 2u;
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
				if( comments_depth == 0 || collect_comments )
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
					goto push_lexem;
				}
				fixed_lexem_str.pop_back();
			}

			if( comments_depth == 0 )
				result.errors.emplace_back(
					"Lexical error: unrecognized character: " + std::to_string(c),
					SrcLoc( 0u, line, column ) );
			++it;
			continue;
		}

	push_lexem:
		lexem.src_loc= SrcLoc( 0u, line, column );

		advance_column();

		if( !( comments_depth != 0 && !collect_comments ) )
			result.lexems.push_back( std::move(lexem) );
	} // while not end

	if( !collect_comments )
		for( int i= 0; i < comments_depth; ++i )
			result.errors.emplace_back( "Lexical error: expected */", SrcLoc( 0u, line, column ) );

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
