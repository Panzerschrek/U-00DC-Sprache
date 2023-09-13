#include "../../tests/tests_lib/funcs_registrator.hpp"
#include "../../tests/tests_lib/tests.hpp"
#include "../lex_synt_lib/lexical_analyzer.hpp"

namespace U
{

namespace
{

void TestLexResult( const char* const program_text, const Lexems& expected_result )
{
	LexicalAnalysisResult lex_result = LexicalAnalysis( program_text );
	if( !lex_result.lexems.empty() && lex_result.lexems.back().type == Lexem::Type::EndOfFile )
		lex_result.lexems.pop_back();

	U_TEST_ASSERT( lex_result.lexems.size() == expected_result.size() );
	for( size_t i= 0; i < expected_result.size(); ++i )
	{
		U_TEST_ASSERT( lex_result.lexems[i].type == expected_result[i].type );
		U_TEST_ASSERT( lex_result.lexems[i].src_loc == expected_result[i].src_loc );

		// Do not compare number text, because in number lexem text actually stored special struct.
		if( expected_result[i].type != Lexem::Type::Number )
		{
			U_TEST_ASSERT( lex_result.lexems[i].text == expected_result[i].text );
		}
	}
}

} // namespace

U_TEST( PosInLine_Test0 )
{
	static const char c_program_text[]=
	R"(
auto x= "str"; var i32 y= 0x666;
	)";

	const Lexems expected_result
	{
		{ "auto"  , SrcLoc( 0, 2,  0 ), Lexem::Type::Identifier },
		{ "x"     , SrcLoc( 0, 2,  5 ), Lexem::Type::Identifier },
		{ "="     , SrcLoc( 0, 2,  6 ), Lexem::Type::Assignment },
		{ "str"   , SrcLoc( 0, 2,  8 ), Lexem::Type::String     },
		{ ";"     , SrcLoc( 0, 2, 13 ), Lexem::Type::Semicolon  },
		{ "var"   , SrcLoc( 0, 2, 15 ), Lexem::Type::Identifier },
		{ "i32"   , SrcLoc( 0, 2, 19 ), Lexem::Type::Identifier },
		{ "y"     , SrcLoc( 0, 2, 23 ), Lexem::Type::Identifier },
		{ "="     , SrcLoc( 0, 2, 24 ), Lexem::Type::Assignment },
		{ "0x666" , SrcLoc( 0, 2, 26 ), Lexem::Type::Number     },
		{ ";"     , SrcLoc( 0, 2, 31 ), Lexem::Type::Semicolon  },
	};

	TestLexResult( c_program_text, expected_result );
}

U_TEST( PosInLine_Test1 )
{
	// Inline comment.

	static const char c_program_text[]=
	R"(
fn Foo(); /* some */ fn Bar();
	)";

	const Lexems expected_result
	{
		{ "fn"  , SrcLoc( 0, 2,  0 ), Lexem::Type::Identifier   },
		{ "Foo" , SrcLoc( 0, 2,  3 ), Lexem::Type::Identifier   },
		{ "("   , SrcLoc( 0, 2,  6 ), Lexem::Type::BracketLeft  },
		{ ")"   , SrcLoc( 0, 2,  7 ), Lexem::Type::BracketRight },
		{ ";"   , SrcLoc( 0, 2,  8 ), Lexem::Type::Semicolon    },
		{ "fn"  , SrcLoc( 0, 2, 21 ), Lexem::Type::Identifier   },
		{ "Bar" , SrcLoc( 0, 2, 24 ), Lexem::Type::Identifier   },
		{ "("   , SrcLoc( 0, 2, 27 ), Lexem::Type::BracketLeft  },
		{ ")"   , SrcLoc( 0, 2, 28 ), Lexem::Type::BracketRight },
		{ ";"   , SrcLoc( 0, 2, 29 ), Lexem::Type::Semicolon    },
	};

	TestLexResult( c_program_text, expected_result );
}

U_TEST( PosInLine_Test2 )
{
	// Symbols with multibyte utf-8 representation.

	static const char c_program_text[]=
	R"(
auto& str= "киррилическая строка"; fn Große_Lüge();
	)";

	const Lexems expected_result
	{
		{ "auto"                , SrcLoc( 0, 2,  0 ), Lexem::Type::Identifier   },
		{ "&"                   , SrcLoc( 0, 2,  4 ), Lexem::Type::And          },
		{ "str"                 , SrcLoc( 0, 2,  6 ), Lexem::Type::Identifier   },
		{ "="                   , SrcLoc( 0, 2,  9 ), Lexem::Type::Assignment   },
		{ "киррилическая строка", SrcLoc( 0, 2, 11 ), Lexem::Type::String       },
		{ ";"                   , SrcLoc( 0, 2, 33 ), Lexem::Type::Semicolon    },
		{ "fn"                  , SrcLoc( 0, 2, 35 ), Lexem::Type::Identifier   },
		{ "Große_Lüge"          , SrcLoc( 0, 2, 38 ), Lexem::Type::Identifier   },
		{ "("                   , SrcLoc( 0, 2, 48 ), Lexem::Type::BracketLeft  },
		{ ")"                   , SrcLoc( 0, 2, 49 ), Lexem::Type::BracketRight },
		{ ";"                   , SrcLoc( 0, 2, 50 ), Lexem::Type::Semicolon    },
	};

	TestLexResult( c_program_text, expected_result );
}

U_TEST( PosInLine_Test3 )
{
	// String literal suffix.

	static const char c_program_text[]=
	R"(
auto& str= "str"u16; static_assert(true);
	)";

	const Lexems expected_result
	{
		{ "auto"            , SrcLoc( 0, 2,  0 ), Lexem::Type::Identifier    },
		{ "&"               , SrcLoc( 0, 2,  4 ), Lexem::Type::And           },
		{ "str"             , SrcLoc( 0, 2,  6 ), Lexem::Type::Identifier    },
		{ "="               , SrcLoc( 0, 2,  9 ), Lexem::Type::Assignment    },
		{ "str"             , SrcLoc( 0, 2, 11 ), Lexem::Type::String        },
		{ "u16"             , SrcLoc( 0, 2, 16 ), Lexem::Type::LiteralSuffix },
		{ ";"               , SrcLoc( 0, 2, 19 ), Lexem::Type::Semicolon     },
		{ "static_assert"   , SrcLoc( 0, 2, 21 ), Lexem::Type::Identifier    },
		{ "("               , SrcLoc( 0, 2, 34 ), Lexem::Type::BracketLeft   },
		{ "true"            , SrcLoc( 0, 2, 35 ), Lexem::Type::Identifier    },
		{ ")"               , SrcLoc( 0, 2, 39 ), Lexem::Type::BracketRight  },
		{ ";"               , SrcLoc( 0, 2, 40 ), Lexem::Type::Semicolon     },
	};

	TestLexResult( c_program_text, expected_result );
}

U_TEST( LineNumberTest0 )
{
	static const char c_program_text[]=
	R"(
fn
Foo // scarry
(
// wtf
);
	)";

	const Lexems expected_result
	{
		{ "fn"  , SrcLoc( 0, 2,  0 ), Lexem::Type::Identifier   },
		{ "Foo" , SrcLoc( 0, 3,  0 ), Lexem::Type::Identifier   },
		{ "("   , SrcLoc( 0, 4,  0 ), Lexem::Type::BracketLeft  },
		{ ")"   , SrcLoc( 0, 6,  0 ), Lexem::Type::BracketRight },
		{ ";"   , SrcLoc( 0, 6,  1 ), Lexem::Type::Semicolon    },
	};

	TestLexResult( c_program_text, expected_result );
}

U_TEST( ValidIdentifierTest )
{
	U_TEST_ASSERT(  IsValidIdentifier( "foo" ) );
	U_TEST_ASSERT(  IsValidIdentifier( "Foo" ) );
	U_TEST_ASSERT(  IsValidIdentifier( "F00" ) );
	U_TEST_ASSERT(  IsValidIdentifier( "q12345" ) );
	U_TEST_ASSERT(  IsValidIdentifier( "ЭтоТожеВерныйИдентефикатор" ) );
	U_TEST_ASSERT(  IsValidIdentifier( "   Foo   " ) );
	U_TEST_ASSERT(  IsValidIdentifier( "   Фуу   " ) );
	U_TEST_ASSERT(  IsValidIdentifier( " \n\r\t   швуте  \r\t\n " ) );
	U_TEST_ASSERT( !IsValidIdentifier( "@foo" ) );
	U_TEST_ASSERT( !IsValidIdentifier( "@ foo" ) );
	U_TEST_ASSERT( !IsValidIdentifier( "foo%" ) );
	U_TEST_ASSERT( !IsValidIdentifier( "1foo" ) );
	U_TEST_ASSERT( !IsValidIdentifier( "foo  *" ) );
	U_TEST_ASSERT( !IsValidIdentifier( "foo foo" ) );
	U_TEST_ASSERT( !IsValidIdentifier( "\"foo\"" ) );
	U_TEST_ASSERT( !IsValidIdentifier( "++" ) );
	U_TEST_ASSERT( !IsValidIdentifier( "()" ) );
}

U_TEST( LineToLinearPositionIndex_Test0 )
{
	static const char c_program_text[] = "";
	U_TEST_ASSERT( BuildLineToLinearPositionIndex( c_program_text ) == LineToLinearPositionIndex({ 0, 0 }) );
}

U_TEST( LineToLinearPositionIndex_Test1 )
{
	static const char c_program_text[] = "foo";
	U_TEST_ASSERT( BuildLineToLinearPositionIndex( c_program_text ) == LineToLinearPositionIndex({ 0, 0 }) );
}

U_TEST( LineToLinearPositionIndex_Test2 )
{
	static const char c_program_text[] = "foo\nbarw";
	U_TEST_ASSERT( BuildLineToLinearPositionIndex( c_program_text ) == LineToLinearPositionIndex({ 0, 0, 4 }) );
}

U_TEST( LineToLinearPositionIndex_Test3 )
{
	static const char c_program_text[] = "foo\n\nbarw";
	U_TEST_ASSERT( BuildLineToLinearPositionIndex( c_program_text ) == LineToLinearPositionIndex({ 0, 0, 4, 5 }) );
}

U_TEST( LineToLinearPositionIndex_Test4 )
{
	static const char c_program_text[] = "\n";
	U_TEST_ASSERT( BuildLineToLinearPositionIndex( c_program_text ) == LineToLinearPositionIndex({ 0, 0, 1 }) );
}

U_TEST( LineToLinearPositionIndex_Test5 )
{
	static const char c_program_text[] = "\n ";
	U_TEST_ASSERT( BuildLineToLinearPositionIndex( c_program_text ) == LineToLinearPositionIndex({ 0, 0, 1 }) );
}

U_TEST( LineToLinearPositionIndex_Test6 )
{
	static const char c_program_text[] = "foo\n";
	U_TEST_ASSERT( BuildLineToLinearPositionIndex( c_program_text ) == LineToLinearPositionIndex({ 0, 0, 4 }) );
}

U_TEST( LineToLinearPositionIndex_Test7 )
{
	static const char c_program_text[] = "foo\n\n";
	U_TEST_ASSERT( BuildLineToLinearPositionIndex( c_program_text ) == LineToLinearPositionIndex({ 0, 0, 4, 5 }) );
}

U_TEST( LinearPositionToLine_Test0 )
{
	static const char c_program_text[] = "";
	const LineToLinearPositionIndex index= BuildLineToLinearPositionIndex( c_program_text );
	U_TEST_ASSERT( LinearPositionToLine( index, 0 ) == 1 );
	U_TEST_ASSERT( LinearPositionToLine( index, 1 ) == 1 );
	U_TEST_ASSERT( LinearPositionToLine( index, 2 ) == 1 );
	U_TEST_ASSERT( LinearPositionToLine( index, 100 ) == 1 );
}

U_TEST( LinearPositionToLine_Test1 )
{
	static const char c_program_text[] = "fn foo()\n{\n\tbar();\n}\n";
	const LineToLinearPositionIndex index= BuildLineToLinearPositionIndex( c_program_text );
	U_TEST_ASSERT( LinearPositionToLine( index,  0 ) == 1 );
	U_TEST_ASSERT( LinearPositionToLine( index,  6 ) == 1 );
	U_TEST_ASSERT( LinearPositionToLine( index,  8 ) == 1 ); // '\n' counts as last symbol in the line.
	U_TEST_ASSERT( LinearPositionToLine( index,  9 ) == 2 );
	U_TEST_ASSERT( LinearPositionToLine( index, 10 ) == 2 ); // '\n' counts as last symbol in the line.
	U_TEST_ASSERT( LinearPositionToLine( index, 11 ) == 3 );
	U_TEST_ASSERT( LinearPositionToLine( index, 12 ) == 3 );
	U_TEST_ASSERT( LinearPositionToLine( index, 13 ) == 3 );
	U_TEST_ASSERT( LinearPositionToLine( index, 18 ) == 3 );  // '\n' counts as last symbol in the line.
	U_TEST_ASSERT( LinearPositionToLine( index, 19 ) == 4 );
	U_TEST_ASSERT( LinearPositionToLine( index, 20 ) == 4 );
	U_TEST_ASSERT( LinearPositionToLine( index, 21 ) == 5 );
	U_TEST_ASSERT( LinearPositionToLine( index, 22 ) == 5 );
	U_TEST_ASSERT( LinearPositionToLine( index, 23 ) == 5 );
	U_TEST_ASSERT( LinearPositionToLine( index, 53 ) == 5 );
}

U_TEST( IdentifierStartEndPosition_Test0 )
{
	static const char c_program_text[] = "foo  bar {baz} (qerty) ++*= []%%%";

	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  0 ) == TextLinearPosition( 0) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  1 ) == TextLinearPosition( 0) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  2 ) == TextLinearPosition( 0) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  3 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  4 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  5 ) == TextLinearPosition( 5) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  6 ) == TextLinearPosition( 5) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  7 ) == TextLinearPosition( 5) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  8 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  9 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 10 ) == TextLinearPosition(10) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 11 ) == TextLinearPosition(10) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 12 ) == TextLinearPosition(10) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 13 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 14 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 15 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 16 ) == TextLinearPosition(16) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 17 ) == TextLinearPosition(16) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 18 ) == TextLinearPosition(16) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 19 ) == TextLinearPosition(16) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 20 ) == TextLinearPosition(16) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 21 ) == std::nullopt );

	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 24 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 25 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 29 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 30 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 31 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 32 ) == std::nullopt );

	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  0 ) == TextLinearPosition( 3) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  1 ) == TextLinearPosition( 3) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  2 ) == TextLinearPosition( 3) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  3 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  4 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  5 ) == TextLinearPosition( 8) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  6 ) == TextLinearPosition( 8) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  7 ) == TextLinearPosition( 8) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  8 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  9 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 10 ) == TextLinearPosition(13) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 11 ) == TextLinearPosition(13) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 12 ) == TextLinearPosition(13) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 13 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 14 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 15 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 16 ) == TextLinearPosition(21) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 17 ) == TextLinearPosition(21) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 18 ) == TextLinearPosition(21) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 19 ) == TextLinearPosition(21) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 20 ) == TextLinearPosition(21) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 21 ) == std::nullopt );

	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 24 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 25 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 29 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 30 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 31 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 32 ) == std::nullopt );
}

U_TEST( IdentifierStartEndPosition_Test1 )
{
	static const char c_program_text[] = "хлеб für  Путин>";

	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  0 ) == TextLinearPosition( 0) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  2 ) == TextLinearPosition( 0) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  4 ) == TextLinearPosition( 0) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  6 ) == TextLinearPosition( 0) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  8 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text,  9 ) == TextLinearPosition( 9) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 10 ) == TextLinearPosition( 9) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 12 ) == TextLinearPosition( 9) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 13 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 14 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 15 ) == TextLinearPosition(15) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 17 ) == TextLinearPosition(15) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 19 ) == TextLinearPosition(15) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 21 ) == TextLinearPosition(15) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 23 ) == TextLinearPosition(15) );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 25 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierStartForPosition( c_program_text, 26 ) == std::nullopt );

	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  0 ) == TextLinearPosition( 8) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  2 ) == TextLinearPosition( 8) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  4 ) == TextLinearPosition( 8) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  6 ) == TextLinearPosition( 8) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  8 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text,  9 ) == TextLinearPosition(13) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 10 ) == TextLinearPosition(13) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 12 ) == TextLinearPosition(13) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 13 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 14 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 15 ) == TextLinearPosition(25) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 17 ) == TextLinearPosition(25) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 19 ) == TextLinearPosition(25) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 21 ) == TextLinearPosition(25) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 23 ) == TextLinearPosition(25) );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 25 ) == std::nullopt );
	U_TEST_ASSERT( GetIdentifierEndForPosition( c_program_text, 26 ) == std::nullopt );
}

} // namespace U
