#include "tests.hpp"

namespace U
{

namespace
{

void TestLexResult( const char* const program_text, const Lexems& expected_result )
{
	LexicalAnalysisResult lex_result = LexicalAnalysis( program_text );
	if( !lex_result.lexems.empty() && lex_result.lexems.back().type == Lexem::Type::EndOfFile )
		lex_result.lexems.pop_back();
	U_TEST_ASSERT( lex_result.lexems == expected_result );
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
		{ "auto"  , FilePos{ 0, 2,  0 }, Lexem::Type::Identifier },
		{ "x"     , FilePos{ 0, 2,  5 }, Lexem::Type::Identifier },
		{ "="     , FilePos{ 0, 2,  6 }, Lexem::Type::Assignment },
		{ "str"   , FilePos{ 0, 2,  8 }, Lexem::Type::String     },
		{ ";"     , FilePos{ 0, 2, 13 }, Lexem::Type::Semicolon  },
		{ "var"   , FilePos{ 0, 2, 15 }, Lexem::Type::Identifier },
		{ "i32"   , FilePos{ 0, 2, 19 }, Lexem::Type::Identifier },
		{ "y"     , FilePos{ 0, 2, 23 }, Lexem::Type::Identifier },
		{ "="     , FilePos{ 0, 2, 24 }, Lexem::Type::Assignment },
		{ "0x666" , FilePos{ 0, 2, 26 }, Lexem::Type::Number     },
		{ ";"     , FilePos{ 0, 2, 31 }, Lexem::Type::Semicolon  },
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
		{ "fn"  , FilePos{ 0, 2,  0 }, Lexem::Type::Identifier   },
		{ "Foo" , FilePos{ 0, 2,  3 }, Lexem::Type::Identifier   },
		{ "("   , FilePos{ 0, 2,  6 }, Lexem::Type::BracketLeft  },
		{ ")"   , FilePos{ 0, 2,  7 }, Lexem::Type::BracketRight },
		{ ";"   , FilePos{ 0, 2,  8 }, Lexem::Type::Semicolon    },
		{ "fn"  , FilePos{ 0, 2, 21 }, Lexem::Type::Identifier   },
		{ "Bar" , FilePos{ 0, 2, 24 }, Lexem::Type::Identifier   },
		{ "("   , FilePos{ 0, 2, 27 }, Lexem::Type::BracketLeft  },
		{ ")"   , FilePos{ 0, 2, 28 }, Lexem::Type::BracketRight },
		{ ";"   , FilePos{ 0, 2, 29 }, Lexem::Type::Semicolon    },
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
		{ "auto"                , FilePos{ 0, 2,  0 }, Lexem::Type::Identifier },
		{ "&"                   , FilePos{ 0, 2,  4 }, Lexem::Type::And        },
		{ "str"                 , FilePos{ 0, 2,  6 }, Lexem::Type::Identifier },
		{ "="                   , FilePos{ 0, 2,  9 }, Lexem::Type::Assignment },
		{ "киррилическая строка", FilePos{ 0, 2, 11 }, Lexem::Type::String     },
		{ ";"                   , FilePos{ 0, 2, 33 }, Lexem::Type::Semicolon  },
		{ "fn"                  , FilePos{ 0, 2, 35 }, Lexem::Type::Identifier   },
		{ "Große_Lüge"          , FilePos{ 0, 2, 38 }, Lexem::Type::Identifier   },
		{ "("                   , FilePos{ 0, 2, 48 }, Lexem::Type::BracketLeft  },
		{ ")"                   , FilePos{ 0, 2, 49 }, Lexem::Type::BracketRight },
		{ ";"                   , FilePos{ 0, 2, 50 }, Lexem::Type::Semicolon    },
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
		{ "auto"            , FilePos{ 0, 2,  0 }, Lexem::Type::Identifier    },
		{ "&"               , FilePos{ 0, 2,  4 }, Lexem::Type::And           },
		{ "str"             , FilePos{ 0, 2,  6 }, Lexem::Type::Identifier    },
		{ "="               , FilePos{ 0, 2,  9 }, Lexem::Type::Assignment    },
		{ "str"             , FilePos{ 0, 2, 11 }, Lexem::Type::String        },
		{ "u16"             , FilePos{ 0, 2, 16 }, Lexem::Type::LiteralSuffix },
		{ ";"               , FilePos{ 0, 2, 19 }, Lexem::Type::Semicolon     },
		{ "static_assert"   , FilePos{ 0, 2, 21 }, Lexem::Type::Identifier    },
		{ "("               , FilePos{ 0, 2, 34 }, Lexem::Type::BracketLeft   },
		{ "true"            , FilePos{ 0, 2, 35 }, Lexem::Type::Identifier    },
		{ ")"               , FilePos{ 0, 2, 39 }, Lexem::Type::BracketRight  },
		{ ";"               , FilePos{ 0, 2, 40 }, Lexem::Type::Semicolon     },
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
		{ "fn"  , FilePos{ 0, 2,  0 }, Lexem::Type::Identifier   },
		{ "Foo" , FilePos{ 0, 3,  0 }, Lexem::Type::Identifier   },
		{ "("   , FilePos{ 0, 4,  0 }, Lexem::Type::BracketLeft  },
		{ ")"   , FilePos{ 0, 6,  0 }, Lexem::Type::BracketRight },
		{ ";"   , FilePos{ 0, 6,  1 }, Lexem::Type::Semicolon    },
	};

	TestLexResult( c_program_text, expected_result );
}

} // namespace U
