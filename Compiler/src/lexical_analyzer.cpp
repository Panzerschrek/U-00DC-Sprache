#include <cctype>
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

typedef std::map<ProgramString, Lexem::Type> FixedLexemsMap;
static const size_t g_max_fixed_lexem_size= 3;

static const FixedLexemsMap g_fixed_lexems[ g_max_fixed_lexem_size + 1 ]=
{
	{ // Zero symbol lexems.
	},
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
	{ // Two symbol lexems.
		{ "</"_SpC, Lexem::Type::TemplateBracketLeft  },
		{ "/>"_SpC, Lexem::Type::TemplateBracketRight },

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

		{ "<-"_SpC, Lexem::Type::LeftArrow },
	},
	{ // Three symbol lexems.
		{ "<<="_SpC, Lexem::Type::AssignShiftLeft  },
		{ ">>="_SpC, Lexem::Type::AssignShiftRight },
	},
};

static bool IsWhitespace( sprache_char c )
{
	return std::isspace( c ) || std::iscntrl( c );
}

static bool IsNewline( sprache_char c )
{
	return c == '\n';
}

static bool IsNumberStartChar( sprache_char c )
{
	return std::isdigit(c);
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
		ProgramString::const_iterator& it,
		const ProgramString::const_iterator it_end,
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
	ProgramString::const_iterator& it,
	const ProgramString::const_iterator it_end )
{
	U_ASSERT( *it == '"' );
	++it;

	Lexem result;
	result.type= Lexem::Type::String;

	while(true)
	{
		if( it == it_end )
			break;

		if( *it == '"' )
		{
			++it;
			break;
		}

		if( *it == '\\' )
		{
			++it;
			if( it == it_end )
				break;
			if( *it != '"' )
				break;
			result.text.push_back(*it);
			++it;
		}
		else
		{
			result.text.push_back(*it);
			++it;
		}
	}

	return result;
}

static Lexem ParseNumber(
	ProgramString::const_iterator& it,
	const ProgramString::const_iterator it_end )
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
					return std::isxdigit(c);
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
				return std::isdigit(c);
			},
			true );
	}

	// Type suffix.
	while( it < it_end && IsIdentifierChar(*it) )
	{
		result.text.push_back(*it);
		++it;
	}

	return result;
}

static Lexem ParseIdentifier(
	ProgramString::const_iterator& it,
	const ProgramString::const_iterator it_end )
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

LexicalAnalysisResult LexicalAnalysis( const ProgramString& program_text )
{
	LexicalAnalysisResult result;

	ProgramString::const_iterator it= program_text.begin();
	const ProgramString::const_iterator it_end= program_text.end();

	unsigned int line= 1; // Count lines from "1", in human-readable format.
	ProgramString::const_iterator last_newline_it= program_text.begin();

	while( it < it_end )
	{
		const sprache_char c= *it;
		Lexem lexem;

		// line comment.
		if( c == '/' && it_end - it > 1 && *(it+1) == '/' )
		{
			while( it < it_end && !IsNewline(*it) ) ++it;
			if( it == it_end ) break;

			line++;
			last_newline_it= it;
			++it;

			continue;
		}
		else if( c == '"' )
			lexem= ParseString( it, it_end );

		else if( IsNumberStartChar(c) )
			lexem= ParseNumber( it, it_end );

		else if( IsIdentifierStartChar(c) )
			lexem= ParseIdentifier( it, it_end );

		else if( IsNewline(c) )
		{
			line++;
			last_newline_it= it;

			++it;
			continue;
		}
		else if( IsWhitespace(c) )
		{
			++it;
			continue;
		}
		else
		{
			// Try find fixed lexems.
			for( unsigned int s= g_max_fixed_lexem_size; s >= 1; --s )
			{
				if( it + s <= it_end )
				{
					const FixedLexemsMap& m= g_fixed_lexems[s];
					ProgramString str( &*it, &*it + s );

					const auto lexem_it= m.find( str );
					if( lexem_it != m.end() )
					{
						it+= s;

						lexem.type= lexem_it->second;
						lexem.text= std::move(str);
						goto push_lexem;
					}
				}
			}

			result.error_messages.emplace_back(
				std::to_string(line) + ":" + std::to_string(it - last_newline_it) +
				" Lexical error: unrecognized character: " + std::to_string(*it) );

			++it;
			continue;
		}

	push_lexem:
		lexem.file_pos.line= line;
		lexem.file_pos.pos_in_line= (unsigned int)( it - last_newline_it );

		result.lexems.emplace_back( std::move(lexem) );

	} // while not end

	Lexem eof_lexem;
	eof_lexem.type= Lexem::Type::EndOfFile;
	eof_lexem.text= "EOF"_SpC;
	eof_lexem.file_pos.line= line;
	eof_lexem.file_pos.pos_in_line= (unsigned int)( it - last_newline_it );

	result.lexems.emplace_back( std::move(eof_lexem) );

	return result;
}

} // namespace U
