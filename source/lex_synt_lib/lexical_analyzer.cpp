#include <algorithm>

#include "assert.hpp"
#include "program_string.hpp"
#include "lexical_analyzer.hpp"

namespace U
{

bool operator==( const FilePos& l, const FilePos& r )
{
	return l.file_index == r.file_index && l.line == r.line && r.pos_in_line == l.pos_in_line;
}

bool operator!=( const FilePos& l, const FilePos& r )
{
	return !( l == r );
}

bool operator< ( const FilePos& l, const FilePos& r )
{
	if( l.file_index != r.file_index )
		return l.file_index < r.file_index;
	if( l.line != r.line )
		return l.line < r.line;
	return l.pos_in_line < r.pos_in_line;
}

bool operator<=( const FilePos& l, const FilePos& r )
{
	return l < r || l == r;
}

bool operator==(const Lexem& l, const Lexem& r )
{
	return l.text == r.text && l.file_pos == r.file_pos && l.type == r.type;
}

bool operator!=(const Lexem& l, const Lexem& r )
{
	return !(l == r );
}

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

void ParseNumberImpl(
	Iterator& it,
	const Iterator it_end,
	Lexem& result,
	bool (*is_digit_func)(char c),
	bool exponent_allowed= false )
{
	// Integer part
	while( it < it_end && is_digit_func(*it) )
	{
		result.text.push_back(*it);
		++it;
	}

	// Fractional part
	if( it < it_end && *it == '.' )
	{
		result.text.push_back(*it);
		++it;

		while( it < it_end && is_digit_func(*it) )
		{
			result.text.push_back(*it);
			++it;
		}
	}

	// Exponent
	if( exponent_allowed && it < it_end && *it == 'e' )
	{
		result.text.push_back(*it);
		++it;

		if( it < it_end && *it == '-' )
		{
			result.text.push_back(*it);
			++it;
		}
		else if( it < it_end && *it == '+' )
			++it;

		while( it < it_end && is_digit_func(*it) )
		{
			result.text.push_back(*it);
			++it;
		}
	}
}

Lexem ParseString( Iterator& it, const Iterator it_end, LexicalErrorMessages& out_errors )
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
			out_errors.push_back( "control character inside string" );
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
						out_errors.push_back( "expected 4 hex digits" );
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
							out_errors.push_back( "expected hex number" );
							return result;
						}
						char_code|= digit << ( ( 3u - i ) * 4u );
						++it;
					}
					PushCharToUTF8String( char_code, result.text );
				}
				break;

			default:
				out_errors.push_back( std::string("invalid escape sequence: \\") + char(*it) );
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

Lexem ParseNumber( Iterator& it, const Iterator it_end )
{
	Lexem result;
	result.type= Lexem::Type::Number;

	if( it_end - it >= 2 && *it == '0' )
	{
		const char d= *(it+1);
		switch(d)
		{
		case 'b':
			result.text.append( it, it + 2 );
			it+= 2;
			ParseNumberImpl(
				it, it_end, result,
				[]( char c ) -> bool
				{
					return c == '0' || c == '1';
				} );

			break;

		case 'o':
			result.text.append( it, it + 2 );
			it+= 2;
			ParseNumberImpl(
				it, it_end, result,
				[]( char c ) -> bool
				{
					return c >= '0' && c <= '7';
				} );

			break;

		case 'x':
			result.text.append( it, it + 2 );
			it+= 2;
			ParseNumberImpl(
				it, it_end, result,
				[]( char c ) -> bool
				{
					return ( c >= '0' && c <= '9' ) || ( c >= 'a' && c <= 'f' ) || ( c >= 'A' && c <= 'F' );
				} );

			break;

		default:
			goto parse_decimal;
		};
	}
	else
	{
	parse_decimal:
		ParseNumberImpl(
			it, it_end, result,
			[]( char c ) -> bool
			{
				return ( c >= '0' && c <= '9' );
			},
			true );
	}

	// TODO - produce separate lexem for it.
	// Type suffix.
	while( it < it_end && IsIdentifierChar(sprache_char(*it)) )
	{
		result.text.push_back(*it);
		++it;
	}

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

	U_ASSERT(IsMacroIdentifierStartChar(*it));
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

} // namespace

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

	unsigned short line= 1; // Count lines from "1", in human-readable format.
	unsigned int pos_in_line= 0u;

	std::string fixed_lexem_str;
	while( it < it_end )
	{
		auto it_prev= it;
		const auto advance_pos_in_line=
		[&]
		{
			while( it_prev < it )
			{
				ReadNextUTF8Char( it_prev, it );
				++pos_in_line;
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
				comment_lexem.file_pos.line= line;
				comment_lexem.file_pos.pos_in_line= static_cast<unsigned short>(pos_in_line);
				comment_lexem.type= Lexem::Type::Comment;

				while( it < it_end && !IsNewline(sprache_char(*it)) )
				{
					comment_lexem.text.push_back(*it);
					++it;
				}
				advance_pos_in_line();
				result.lexems.emplace_back( std::move(comment_lexem) );
			}
			else
				while( it < it_end && !IsNewline(sprache_char(*it)) ) ++it;

			if( it == it_end ) break;

			++line;
			++it;
			pos_in_line= 0u;
			continue;
		}
		if( c == '/' && it_end - it > 1 && *std::next(it) == '*' )
		{
			++comments_depth;
			if( collect_comments )
			{
				Lexem comment_lexem;
				comment_lexem.file_pos.line= line;
				comment_lexem.file_pos.pos_in_line= static_cast<unsigned short>(pos_in_line);
				comment_lexem.type= Lexem::Type::Comment;
				comment_lexem.text= "/*";
				advance_pos_in_line();
				result.lexems.emplace_back( std::move(comment_lexem) );
			}
			it+= 2;
			pos_in_line+= 2u;
			continue;
		}
		if( c == '*' && it_end - it > 1 && *(it+1) == '/' )
		{
			--comments_depth;
			if( collect_comments )
			{
				Lexem comment_lexem;
				lexem.file_pos.file_index= 0u;
				comment_lexem.file_pos.line= line;
				comment_lexem.file_pos.pos_in_line= static_cast<unsigned short>(pos_in_line);
				comment_lexem.type= Lexem::Type::Comment;
				comment_lexem.text= "*/";
				advance_pos_in_line();
				result.lexems.push_back( std::move(comment_lexem) );
			}
			else if( comments_depth < 0 )
				result.error_messages.emplace_back(
					std::to_string(line) + ":" + std::to_string(pos_in_line) +
					" Lexical error: unexpected */" );
			it+= 2;
			pos_in_line+= 2u;
			continue;
		}
		else if( IsNewline(c) )
		{
			++line;
			++it;
			pos_in_line= 0u;
			continue;
		}
		else if( IsWhitespace(c) )
		{
			++it;
			++pos_in_line;
			continue;
		}
		else if( c == '"' )
		{
			lexem= ParseString( it, it_end, result.error_messages );
			if( IsIdentifierStartChar( GetUTF8FirstChar( it, it_end ) ) )
			{
				// Parse string suffix.
				lexem.file_pos.file_index= 0u;
				lexem.file_pos.line= line;
				lexem.file_pos.pos_in_line= static_cast<unsigned short>(pos_in_line);
				advance_pos_in_line();
				if( comments_depth == 0 || collect_comments )
					result.lexems.push_back( std::move(lexem) );

				lexem= ParseIdentifier( it, it_end );
				lexem.type= Lexem::Type::LiteralSuffix;
			}
		}
		else if( IsNumberStartChar(c) )
			lexem= ParseNumber( it, it_end );
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
				result.error_messages.emplace_back(
					std::to_string(line) + ":" + std::to_string(pos_in_line) +
					" Lexical error: unrecognized character: " + std::to_string(*it) );
			++it;
			continue;
		}

	push_lexem:
		lexem.file_pos.file_index= 0u;
		lexem.file_pos.line= line;
		lexem.file_pos.pos_in_line= static_cast<unsigned short>(pos_in_line);

		advance_pos_in_line();

		if( !( comments_depth != 0 && !collect_comments ) )
			result.lexems.push_back( std::move(lexem) );
	} // while not end

	if( !collect_comments )
		for( int i= 0; i < comments_depth; ++i )
			result.error_messages.emplace_back(
				std::to_string(line) + ":" + std::to_string(pos_in_line) +
				" Lexical error: expected */" );

	Lexem eof_lexem;
	eof_lexem.type= Lexem::Type::EndOfFile;
	eof_lexem.text= "EOF";
	eof_lexem.file_pos.file_index= 0;
	eof_lexem.file_pos.line= static_cast<unsigned short>(line);
	eof_lexem.file_pos.pos_in_line= static_cast<unsigned short>(pos_in_line);

	result.lexems.emplace_back( std::move(eof_lexem) );

	return result;
}

} // namespace U
