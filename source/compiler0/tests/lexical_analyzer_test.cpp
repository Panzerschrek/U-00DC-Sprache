#include "../../tests/cpp_tests/tests.hpp"
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

} // namespace U
