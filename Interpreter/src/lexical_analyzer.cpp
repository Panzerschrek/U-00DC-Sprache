#include <cctype>
#include <map>

#include "lexical_analyzer.hpp"

namespace Interpreter
{

typedef std::map<ProgramString, Lexem::Type> FixedLexemsMap;
static const size_t g_max_fixed_lexem_size= 3;

static const FixedLexemsMap g_fixed_lexems[ g_max_fixed_lexem_size + 1 ]=
{
	{ // Zero symbol lexems.
	},
	{ // One symbol lexems.
		{ ToProgramString( "(" ), Lexem::Type::BracketLeft },
		{ ToProgramString( ")" ), Lexem::Type::BracketRight },
		{ ToProgramString( "[" ), Lexem::Type::SquareBracketLeft },
		{ ToProgramString( "]" ), Lexem::Type::SquareBracketRight },
		{ ToProgramString( "{" ), Lexem::Type::BraceLeft },
		{ ToProgramString( "}" ), Lexem::Type::BraceRight },

		{ ToProgramString( "," ), Lexem::Type::Comma },
		{ ToProgramString( "." ), Lexem::Type::Dot },
		{ ToProgramString( ":" ), Lexem::Type::Colon },
		{ ToProgramString( ";" ), Lexem::Type::Semicolon },

		{ ToProgramString( "=" ), Lexem::Type::Assignment },
		{ ToProgramString( "+" ), Lexem::Type::Plus },
		{ ToProgramString( "-" ), Lexem::Type::Minus },
		{ ToProgramString( "*" ), Lexem::Type::Star },
		{ ToProgramString( "/" ), Lexem::Type::Slash },

		{ ToProgramString( "<" ), Lexem::Type::CompareLess },
		{ ToProgramString( ">" ), Lexem::Type::CommpareGreater },

		{ ToProgramString( "|" ), Lexem::Type::Or },
		{ ToProgramString( "^" ), Lexem::Type::Xor },
		{ ToProgramString( "~" ), Lexem::Type::Tilda },
		{ ToProgramString( "!" ), Lexem::Type::Not },
	},
	{ // Two symbol lexems.
		{ ToProgramString( "++" ), Lexem::Type::Increment },
		{ ToProgramString( "--" ), Lexem::Type::Decrement },

		{ ToProgramString( "==" ), Lexem::Type::CompareEqual },
		{ ToProgramString( "!=" ), Lexem::Type::CompareNotEqual },
		{ ToProgramString( "<=" ), Lexem::Type::CompareLessOrEqual },
		{ ToProgramString( ">=" ), Lexem::Type::CompareGreaterOrEqual },

		{ ToProgramString( "&&" ), Lexem::Type::Conjunction },
		{ ToProgramString( "||" ), Lexem::Type::Disjunction },
		{ ToProgramString( "||" ), Lexem::Type::Disjunction },
	},
	{
	},
};

ProgramString ToProgramString( const char* str )
{
	ProgramString result;

	while( *str != 0 )
	{
		result.push_back( *str );
		++str;
	}

	return result;
}

std::string ToStdString( const ProgramString& str )
{
	std::string result( str.size(), '\0' );

	for( unsigned int i= 0; i < str.size(); i++ )
		result[i]= str[i];

	return result;
}

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

static bool IsIdentifierChar( sprache_char c )
{
	return std::isalnum(c) || c == '_';
}

static bool IsIdentifierStartChar( sprache_char c )
{
	return std::isalpha(c);
}

static Lexem ParseNumber(
	ProgramString::const_iterator& it,
	const ProgramString::const_iterator it_end )
{
	// TODO - parse floating point constants (such 1.0f, 70e27), hex constants (such 0xDEAD ).

	Lexem result;
	result.type= Lexem::Type::Number;

	while( it < it_end && std::isdigit(*it) )
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

		if( IsNumberStartChar(c) )
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
		lexem.line= line;
		lexem.pos_in_line= (unsigned int)( it - last_newline_it );

		result.lexems.emplace_back( std::move(lexem) );

	} // while not end

	Lexem eof_lexem;
	eof_lexem.type= Lexem::Type::EndOfFile;
	eof_lexem.text= ToProgramString( "EOF" );
	eof_lexem.line= line;
	eof_lexem.pos_in_line= (unsigned int)( it - last_newline_it );

	result.lexems.emplace_back( std::move(eof_lexem) );

	return result;
}

} // namespace Interpreter
