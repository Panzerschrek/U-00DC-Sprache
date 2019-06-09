#include <map>

#include "assert.hpp"

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

using FixedLexemsMap= ProgramStringMap<Lexem::Type>;
static const size_t g_max_fixed_lexem_size= 3;

static const FixedLexemsMap g_fixed_lexems[ g_max_fixed_lexem_size + 1 ]=
{
	FixedLexemsMap
	{ // Zero symbol lexems.
	},
	FixedLexemsMap
	{ // One symbol lexems.
		{ "("_SpC, Lexem::Type::BracketLeft },
		{ ")"_SpC, Lexem::Type::BracketRight },
		{ "["_SpC, Lexem::Type::SquareBracketLeft },
		{ "]"_SpC, Lexem::Type::SquareBracketRight },
		{ "{"_SpC, Lexem::Type::BraceLeft },
		{ "}"_SpC, Lexem::Type::BraceRight },

		{ ","_SpC, Lexem::Type::Comma },
		{ "."_SpC, Lexem::Type::Dot },
		{ ":"_SpC, Lexem::Type::Colon },
		{ ";"_SpC, Lexem::Type::Semicolon },

		{ "="_SpC, Lexem::Type::Assignment },
		{ "+"_SpC, Lexem::Type::Plus },
		{ "-"_SpC, Lexem::Type::Minus },
		{ "*"_SpC, Lexem::Type::Star },
		{ "/"_SpC, Lexem::Type::Slash },
		{ "%"_SpC, Lexem::Type::Percent },

		{ "<"_SpC, Lexem::Type::CompareLess },
		{ ">"_SpC, Lexem::Type::CompareGreater },

		{ "&"_SpC, Lexem::Type::And },
		{ "|"_SpC, Lexem::Type::Or },
		{ "^"_SpC, Lexem::Type::Xor },
		{ "~"_SpC, Lexem::Type::Tilda },
		{ "!"_SpC, Lexem::Type::Not },

		{ "'"_SpC, Lexem::Type::Apostrophe },
	},
	FixedLexemsMap
	{ // Two symbol lexems.
		{ "</"_SpC, Lexem::Type::TemplateBracketLeft  },
		{ "/>"_SpC, Lexem::Type::TemplateBracketRight },

		{ "<?"_SpC, Lexem::Type::MacroBracketLeft  },
		{ "?>"_SpC, Lexem::Type::MacroBracketRight },

		{ "::"_SpC, Lexem::Type::Scope },

		{ "++"_SpC, Lexem::Type::Increment },
		{ "--"_SpC, Lexem::Type::Decrement },

		{ "=="_SpC, Lexem::Type::CompareEqual },
		{ "!="_SpC, Lexem::Type::CompareNotEqual },
		{ "<="_SpC, Lexem::Type::CompareLessOrEqual },
		{ ">="_SpC, Lexem::Type::CompareGreaterOrEqual },

		{ "&&"_SpC, Lexem::Type::Conjunction },
		{ "||"_SpC, Lexem::Type::Disjunction },

		{ "+="_SpC, Lexem::Type::AssignAdd },
		{ "-="_SpC, Lexem::Type::AssignSub },
		{ "*="_SpC, Lexem::Type::AssignMul },
		{ "/="_SpC, Lexem::Type::AssignDiv },
		{ "%="_SpC, Lexem::Type::AssignRem },
		{ "&="_SpC, Lexem::Type::AssignAnd },
		{ "|="_SpC, Lexem::Type::AssignOr  },
		{ "^="_SpC, Lexem::Type::AssignXor },

		{ "<<"_SpC, Lexem::Type::ShiftLeft  },
		{ ">>"_SpC, Lexem::Type::ShiftRight },

		{ "<-"_SpC, Lexem::Type::LeftArrow  },
		{ "->"_SpC, Lexem::Type::RightArrow },
	},
	FixedLexemsMap
	{ // Three symbol lexems.
		{ "<<="_SpC, Lexem::Type::AssignShiftLeft  },
		{ ">>="_SpC, Lexem::Type::AssignShiftRight },
		{ "..."_SpC, Lexem::Type::Ellipsis },
	},
};

using Iterator= const sprache_char*;

static bool IsWhitespace( sprache_char c )
{
	return
		c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v' ||
		c <= 0x1Fu || c == 0x7Fu;
}

static bool IsNewline( sprache_char c )
{
	return c == '\n';
}

static bool IsNumberStartChar( sprache_char c )
{
	return c >= '0' && c <= '9';
}

static bool IsIdentifierStartChar( sprache_char c )
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

static bool IsIdentifierChar( sprache_char c )
{
	return IsIdentifierStartChar(c) || IsNumberStartChar(c) || c == '_';
}

static void ParseNumberImpl(
		Iterator& it,
		const Iterator it_end,
		Lexem& result,
		bool (*is_digit_func)(sprache_char c),
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

		while( it < it_end && is_digit_func(*it) )
		{
			result.text.push_back(*it);
			++it;
		}
	}
}

static Lexem ParseString(
	Iterator& it,
	const Iterator it_end,
	LexicalErrorMessages& out_errors )
{
	// Ü-string fromat is simular to JSON string format.

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
		else if( ( /* *it >= 0x00u && */ *it < 0x20u ) || *it == 0x7Fu ) // TODO - is this correct control character?
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

			case 'u':
				{
					// Parse hex number.
					++it;
					if( it_end - it < 4 )
					{
						out_errors.push_back( "expected 4 hex digits" );
						return result;
					}

					uint32_t char_code= 0u;
					for( size_t i= 0u; i < 4u; i++ )
					{
						uint32_t digit;
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

					result.text.push_back( sprache_char(char_code) ); // TODO - maybe convert to UTF-16?
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

static Lexem ParseNumber(
	Iterator& it,
	const Iterator it_end )
{
	Lexem result;
	result.type= Lexem::Type::Number;

	if( it_end - it >= 2 && *it == '0' )
	{
		sprache_char d= *(it+1);
		switch(d)
		{
		case 'b':
			result.text.append( it, it + 2 );
			it+= 2;
			ParseNumberImpl(
				it, it_end, result,
				[]( sprache_char c ) -> bool
				{
					return c == '0' || c == '1';
				} );

			break;

		case 'o':
			result.text.append( it, it + 2 );
			it+= 2;
			ParseNumberImpl(
				it, it_end, result,
				[]( sprache_char c ) -> bool
				{
					return c >= '0' && c <= '7';
				} );

			break;

		case 'x':
			result.text.append( it, it + 2 );
			it+= 2;
			ParseNumberImpl(
				it, it_end, result,
				[]( sprache_char c ) -> bool
				{
					return IsNumberStartChar(c) || ( c >= 'a' && c <= 'f' ) || ( c >= 'A' && c <= 'F' );
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
			[]( sprache_char c ) -> bool
			{
				return IsNumberStartChar(c);
			},
			true );
	}

	// TODO - produce separate lexem for it.
	// Type suffix.
	while( it < it_end && IsIdentifierChar(*it) )
	{
		result.text.push_back(*it);
		++it;
	}

	return result;
}

static Lexem ParseIdentifier(
	Iterator& it,
	const Iterator it_end )
{
	Lexem result;
	result.type= Lexem::Type::Identifier;

	while( it < it_end && IsIdentifierChar(*it) )
	{
		result.text.push_back(*it);
		++it;
	}

	return result;
}

static bool IsMacroIdentifierStartChar( const sprache_char c )
{
	return c == '?';
}

static Lexem ParseMacroIdentifier(
	Iterator& it,
	const Iterator it_end )
{
	Lexem result;
	result.type= Lexem::Type::MacroIdentifier;

	U_ASSERT(IsMacroIdentifierStartChar(*it));
	result.text.push_back(*it);
	++it;

	while( it < it_end && IsIdentifierChar(*it) )
	{
		result.text.push_back(*it);
		++it;
	}

	return result;
}

LexicalAnalysisResult LexicalAnalysis( const ProgramString& program_text, const bool collect_comments )
{
	return LexicalAnalysis( program_text.data(), program_text.size(), collect_comments );
}

LexicalAnalysisResult LexicalAnalysis( const sprache_char* const program_text_data, const size_t program_text_size, const bool collect_comments )
{
	LexicalAnalysisResult result;

	Iterator it= program_text_data;
	const Iterator it_end= program_text_data + program_text_size;

	unsigned int line= 1; // Count lines from "1", in human-readable format.
	Iterator last_newline_it= program_text_data;

	ProgramString fixed_lexem_str;
	while( it < it_end )
	{
		const sprache_char c= *it;
		Lexem lexem;
		lexem.file_pos.file_index= 0;
		auto pos_in_line= static_cast<unsigned short>( it - last_newline_it );

		// line comment.
		if( c == '/' && it_end - it > 1 && *(it+1) == '/' )
		{
			if( collect_comments )
			{
				Lexem comment_lexem;
				comment_lexem.file_pos.line= static_cast<unsigned short>(line);
				comment_lexem.file_pos.pos_in_line= pos_in_line;
				comment_lexem.type= Lexem::Type::Comment;

				while( it < it_end && !IsNewline(*it) )
				{
					comment_lexem.text.push_back(*it);
					++it;
				}
				result.lexems.emplace_back( std::move(comment_lexem) );
			}
			else
				while( it < it_end && !IsNewline(*it) ) ++it;

			if( it == it_end ) break;

			line++;
			++it;
			last_newline_it= it;
			pos_in_line= 0;

			continue;
		}
		else if( IsNewline(c) )
		{
			line++;
			++it;
			last_newline_it= it;
			pos_in_line= 0;

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
			if( it < it_end && IsIdentifierStartChar( *it ) )
			{
				// Parse string suffix.
				lexem.file_pos.line= static_cast<unsigned short>(line);
				lexem.file_pos.pos_in_line= pos_in_line;
				result.lexems.emplace_back( std::move(lexem) );

				lexem= ParseIdentifier( it, it_end );
				lexem.type= Lexem::Type::LiteralSuffix;
			}
		}

		else if( IsNumberStartChar(c) )
			lexem= ParseNumber( it, it_end );

		else if( IsIdentifierStartChar(c) )
			lexem= ParseIdentifier( it, it_end );
		else if( IsMacroIdentifierStartChar(c) && std::next(it) < it_end && *std::next(it) != '>' )
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
					goto push_lexem;
				}
				fixed_lexem_str.pop_back();
			}

			result.error_messages.emplace_back(
				std::to_string(line) + ":" + std::to_string(it - last_newline_it) +
				" Lexical error: unrecognized character: " + std::to_string(*it) );

			++it;
			continue;
		}

	push_lexem:
		lexem.file_pos.line= static_cast<unsigned short>(line);
		lexem.file_pos.pos_in_line= static_cast<unsigned short>( pos_in_line );
		lexem.file_pos.file_index= 0;

		result.lexems.emplace_back( std::move(lexem) );

	} // while not end

	Lexem eof_lexem;
	eof_lexem.type= Lexem::Type::EndOfFile;
	eof_lexem.text= "EOF"_SpC;
	eof_lexem.file_pos.line= static_cast<unsigned short>(line);
	eof_lexem.file_pos.pos_in_line= static_cast<unsigned short>( it - last_newline_it );
	eof_lexem.file_pos.file_index= 0;

	result.lexems.emplace_back( std::move(eof_lexem) );

	return result;
}

} // namespace U
