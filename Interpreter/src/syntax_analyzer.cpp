#include "assert.hpp"
#include "syntax_analyzer.hpp"
#include <iostream>
namespace Interpreter
{

namespace Keywords
{

const ProgramString fn= ToPorgramString( "fn" );
const ProgramString let= ToPorgramString( "let" );

}

static void PushErrorMessage(
	SyntaxErrorMessages& error_messages,
	const Lexem& lexem )
{
	error_messages.emplace_back(
		std::to_string(lexem.line) + ":" + std::to_string(lexem.pos_in_line) +
		" Syntax error - unexpected lexem: " + ToStdString(lexem.text) );
}

static Block ParseBlock(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->type == Lexem::Type::BraceLeft );
	U_ASSERT( it < it_end );

	++it;

	unsigned int level= 1;
	while( it < it_end && level > 0 )
	{
		if( it->type == Lexem::Type::BraceRight )
			level--;
		else if( it->type == Lexem::Type::BraceLeft )
			level++;

		++it;
	}

	U_ASSERT( it <= it_end );

	if( level > 0 )
		PushErrorMessage( error_messages, it == it_end ? *(it-1) : *it );

	return Block();
}

static IProgramElementPtr ParseFunction(
	SyntaxErrorMessages& error_messages,
	Lexems::const_iterator& it,
	const Lexems::const_iterator it_end )
{
	U_ASSERT( it->text == Keywords::fn );
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

	Block block;

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

		if( lexem.type == Lexem::Type::Identifier && lexem.text == Keywords::fn )
		{
			if( IProgramElementPtr program_element= ParseFunction( result.error_messages, it, it_end ) )
				result.program_elements.emplace_back( std::move( program_element ) );

			continue;
		}

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
